#!/usr/bin/env python3
"""Utility to export a minified build of the BLIP web dashboard assets."""
from __future__ import annotations

import argparse
import gzip
import re
import shutil
from pathlib import Path
from typing import Callable, Dict, List, Tuple

TextMinifier = Callable[[str], str]
VERSION_REGEX = re.compile(r"<!--\s*Version\s+([^\s]+)\s*-->", re.IGNORECASE)


def format_bytes(value: int) -> str:
    """Return a short human readable size string."""
    units = ["B", "KB", "MB", "GB"]
    size = float(value)
    for unit in units:
        if size < 1024.0 or unit == units[-1]:
            return f"{size:.1f}{unit}" if unit != "B" else f"{int(size)}B"
        size /= 1024.0
    return f"{size:.1f}GB"


def minify_css(css: str) -> str:
    css = re.sub(r"/\*.*?\*/", "", css, flags=re.S)
    css = re.sub(r"\s+", " ", css)
    css = re.sub(r"\s*([{}:;,>~])\s*", r"\1", css)
    css = re.sub(r";}", "}", css)
    return css.strip()


def _strip_js_comments(js: str) -> str:
    result: list[str] = []
    in_single_comment = False
    in_multi_comment = False
    in_string: str | None = None
    in_template = False
    escape = False
    i = 0
    length = len(js)
    prev_char = ""

    while i < length:
        ch = js[i]
        nxt = js[i + 1] if i + 1 < length else ""

        if in_single_comment:
            if ch == "\n":
                in_single_comment = False
                result.append(ch)
            i += 1
            prev_char = ch
            continue

        if in_multi_comment:
            if ch == "*" and nxt == "/":
                in_multi_comment = False
                i += 2
            else:
                i += 1
            prev_char = ch
            continue

        if in_string:
            result.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == in_string:
                in_string = None
            i += 1
            prev_char = ch
            continue

        if in_template:
            result.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == "`":
                in_template = False
            i += 1
            prev_char = ch
            continue

        if ch == "/" and nxt == "/" and prev_char != "\\":
            in_single_comment = True
            i += 2
            prev_char = ""
            continue

        if ch == "/" and nxt == "*":
            in_multi_comment = True
            i += 2
            prev_char = ""
            continue

        if ch in "'\"":
            in_string = ch
            result.append(ch)
            i += 1
            prev_char = ch
            continue

        if ch == "`":
            in_template = True
            result.append(ch)
            i += 1
            prev_char = ch
            continue

        result.append(ch)
        prev_char = ch
        i += 1

    return "".join(result)


def minify_js(js: str) -> str:
    stripped = _strip_js_comments(js)
    result: list[str] = []
    in_string: str | None = None
    in_template = False
    escape = False
    prev_char = ""
    i = 0
    length = len(stripped)

    def next_non_space(index: int) -> str:
        for j in range(index + 1, length):
            if stripped[j] not in " \t\r\n":
                return stripped[j]
        return ""

    def needs_space(left: str, right: str) -> bool:
        if not left or not right:
            return False
        if left == ";":
            return False
        left_is_word = left.isalnum() or left in "_$"
        right_is_word = right.isalnum() or right in "_$"
        return left_is_word and right_is_word

    while i < length:
        ch = stripped[i]

        if in_string:
            result.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == in_string:
                in_string = None
            i += 1
            continue

        if in_template:
            result.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == "`":
                in_template = False
            i += 1
            continue

        if ch in "'\"":
            in_string = ch
            result.append(ch)
            prev_char = ch
            i += 1
            continue

        if ch == "`":
            in_template = True
            result.append(ch)
            prev_char = ch
            i += 1
            continue

        if ch in " \t\r\n":
            nxt = next_non_space(i)
            if needs_space(prev_char, nxt):
                if result and result[-1] != " ":
                    result.append(" ")
                    prev_char = " "
            i += 1
            continue

        result.append(ch)
        prev_char = ch
        i += 1

    return "".join(result).strip()


def minify_html(html: str) -> str:
    version_prefix = ""
    match = VERSION_REGEX.search(html)
    if match:
        version_prefix = f"<!-- Version {match.group(1)} -->\n"

    html = re.sub(r"<!--(?!\s*\[if).*?-->", "", html, flags=re.S)
    html = re.sub(r">\s+<", "><", html)
    html = re.sub(r"\s+", " ", html)
    minified = html.strip()

    return version_prefix + minified if version_prefix else minified


FILES_TO_MINIFY: Dict[str, TextMinifier] = {
    "edit.html": minify_html,
    "edit.css": minify_css,
    "edit.js": minify_js,
    "osc-mini.js": minify_js,
}


def export_assets(src: Path, dst: Path, gzip_output: bool) -> List[Tuple[str, int, int]]:
    if not src.exists():
        raise FileNotFoundError(f"Source directory '{src}' does not exist")

    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True)

    summary: list[Tuple[str, int, int]] = []

    for entry in src.iterdir():
        target = dst / entry.name
        if entry.is_dir():
            shutil.copytree(entry, target)
            continue

        if entry.name in FILES_TO_MINIFY:
            original_text = entry.read_text(encoding="utf-8")
            minified_text = FILES_TO_MINIFY[entry.name](original_text)
            target.write_text(minified_text, encoding="utf-8")
            orig_size = len(original_text.encode("utf-8"))
            new_size = len(minified_text.encode("utf-8"))
            summary.append((entry.name, orig_size, new_size))

            if gzip_output:
                gz_path = target.with_suffix(target.suffix + ".gz")
                with gzip.open(gz_path, "wb", compresslevel=9) as gz_file:
                    gz_file.write(minified_text.encode("utf-8"))
        else:
            shutil.copy2(entry, target)
            file_size = entry.stat().st_size
            summary.append((entry.name, file_size, file_size))

    return summary


def compress_assets(src: Path, dst: Path) -> List[Tuple[str, int, int]]:
    if not src.exists():
        raise FileNotFoundError(f"Source directory '{src}' does not exist")

    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True)

    summary: List[Tuple[str, int, int]] = []

    for entry in src.rglob("*"):
        if entry.is_dir():
            (dst / entry.relative_to(src)).mkdir(parents=True, exist_ok=True)
            continue

        rel_path = entry.relative_to(src)
        target_base = dst / rel_path
        target_base.parent.mkdir(parents=True, exist_ok=True)

        if target_base.suffix:
            gz_path = target_base.with_suffix(target_base.suffix + ".gz")
        else:
            gz_path = target_base.with_suffix(".gz")

        file_bytes = entry.read_bytes()
        with gzip.open(gz_path, "wb", compresslevel=9) as gz_file:
            gz_file.write(file_bytes)

        summary.append((str(rel_path), len(file_bytes), gz_path.stat().st_size))

    return summary


def main() -> None:
    parser = argparse.ArgumentParser(description="Export minified BLIP dashboard assets.")
    parser.add_argument("--src", default="data/server", help="Source directory of dashboard assets.")
    parser.add_argument(
        "--prod-out",
        default="data/server-prod",
        help="Directory for minified but uncompressed assets.",
    )
    parser.add_argument(
        "--compressed-out",
        default="data/server-compressed",
        help="Directory containing gzipped versions of the minified assets.",
    )

    args = parser.parse_args()
    src_dir = Path(args.src).resolve()
    prod_dir = Path(args.prod_out).resolve()
    compressed_dir = Path(args.compressed_out).resolve()

    summary = export_assets(src_dir, prod_dir, gzip_output=False)
    gzip_summary = compress_assets(prod_dir, compressed_dir)

    print(f"Exported optimized assets from {src_dir} to {prod_dir}")
    print(f"Compressed assets written to {compressed_dir}\n")

    print(f"{'File':30} {'Original':>12} {'Optimized':>12} {'Δ':>8}")
    print("-" * 70)
    for name, orig, new in summary:
        delta = new - orig
        print(f"{name:30} {format_bytes(orig):>12} {format_bytes(new):>12} {delta:>8}")

    print("\nCompressed (.gz) sizes")
    print(f"{'File':30} {'Minified':>12} {'Gzip':>12} {'Δ':>8}")
    print("-" * 70)
    for name, orig, new in gzip_summary:
        delta = new - orig
        print(f"{name+'.gz':30} {format_bytes(orig):>12} {format_bytes(new):>12} {delta:>8}")


if __name__ == "__main__":
    main()
