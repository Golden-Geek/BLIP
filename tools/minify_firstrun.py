#!/usr/bin/env python3
"""Minify and gzip firstrun.html, then regenerate FirstrunPage.h."""

from __future__ import annotations

import argparse
import gzip
import re
import sys
from pathlib import Path
from textwrap import dedent

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_HTML = REPO_ROOT / "data" / "server-files" / "firstrun.html"
DEFAULT_MINIFIED = REPO_ROOT / "data" / "server-files" / "firstrun.min.html"
DEFAULT_GZIP = REPO_ROOT / "data" / "server-files" / "firstrun.min.html.gz"
DEFAULT_HEADER = REPO_ROOT / "src" / "Component" / "components" / "communication" / "server" / "FirstrunPage.h"


def minify_html(source: str) -> str:
    """Collapse whitespace and strip comments to keep JS/CSS intact."""
    without_comments = re.sub(r"<!--.*?-->", "", source, flags=re.S)
    collapsed = re.sub(r"\s+", " ", without_comments)
    tightened = re.sub(r">\s+<", "><", collapsed)
    return tightened.strip()


def format_bytes(data: bytes, columns: int = 16) -> str:
    lines = []
    for idx in range(0, len(data), columns):
        chunk = ", ".join(f"0x{byte:02x}" for byte in data[idx : idx + columns])
        lines.append("        " + chunk)
    return ",\n".join(lines)


def write_header(path: Path, data: bytes) -> None:
    body = format_bytes(data)
    header = dedent(
        f"""
        #pragma once
        
        #include <cstddef>
        #include <pgmspace.h>
        
        namespace BlipServerPages
        {{
            constexpr unsigned char FIRSTRUN_PAGE_GZ[] PROGMEM = {{
        {body}
            }};
            constexpr size_t FIRSTRUN_PAGE_GZ_LEN = {len(data)};
        }}
        """
    ).strip() + "\n"
    path.write_text(header, encoding="utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--html", default=str(DEFAULT_HTML), help="Path to the editable firstrun.html")
    parser.add_argument("--minified", default=str(DEFAULT_MINIFIED), help="Where to write the minified HTML copy")
    parser.add_argument("--gzip", default=str(DEFAULT_GZIP), help="Where to write the gzipped payload")
    parser.add_argument("--header", default=str(DEFAULT_HEADER), help="FirstrunPage.h output path")
    args = parser.parse_args(argv)

    html_path = Path(args.html)
    if not html_path.exists():
        raise SystemExit(f"HTML input not found: {html_path}")

    original = html_path.read_text(encoding="utf-8")
    minified = minify_html(original)
    min_path = Path(args.minified)
    min_path.write_text(minified, encoding="utf-8")

    gz_bytes = gzip.compress(minified.encode("utf-8"), compresslevel=9)
    gz_path = Path(args.gzip)
    gz_path.write_bytes(gz_bytes)

    write_header(Path(args.header), gz_bytes)

    ratio = len(gz_bytes) / len(original.encode("utf-8")) if original else 0
    print(f"Minified length: {len(minified)} chars")
    print(f"Gzip size: {len(gz_bytes)} bytes ({ratio:.2%} of original)")
    print(f"Updated header: {args.header}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
