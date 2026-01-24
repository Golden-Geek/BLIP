#!/usr/bin/env python3
"""Decode ESP32/ESP32-C6 backtraces using the matching PlatformIO build ELF.

This is a practical fallback when the PlatformIO serial monitor doesn't decode
exceptions automatically (e.g., missing monitor filter).

Typical input looks like:
  Backtrace: 0x4200abcd:0x3fc9e2a0 0x42001234:0x3fc9e2c0

We extract the program-counter (PC) portion (left side of each "PC:SP" pair)
then run addr2line to resolve function + file:line.

Usage:
  python tools/decode_backtrace.py -e creatorsball "Backtrace: 0x...:0x... 0x...:0x..."

Or pipe from a file/stdin:
  type crash.txt | python tools/decode_backtrace.py -e creatorsball

Notes:
- You must decode with the exact matching `.pio/build/<env>/firmware.elf`.
- Works for Xtensa (ESP32) and RISC-V (ESP32-C3/C6/etc) by auto-selecting the
  correct addr2line tool.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import struct
from pathlib import Path
from typing import List, Optional, Tuple


_HEX_RE = re.compile(r"0x[0-9a-fA-F]+")


def _hex_to_int(addr: str) -> Optional[int]:
    try:
        return int(addr, 16)
    except Exception:
        return None


def _extract_named_register_addrs(text: str) -> List[str]:
    """Extract MEPC/RA/PC-like addresses from a Guru Meditation register dump."""
    named: List[str] = []
    # Examples:
    # MEPC    : 0x4002145e  RA      : 0x400213f4
    patterns = [
        r"\bMEPC\b\s*:\s*(0x[0-9a-fA-F]+)",
        r"\bRA\b\s*:\s*(0x[0-9a-fA-F]+)",
        r"\bPC\b\s*:\s*(0x[0-9a-fA-F]+)",
        r"\bEPC\b\s*:\s*(0x[0-9a-fA-F]+)",
    ]
    for pat in patterns:
        for m in re.finditer(pat, text):
            named.append(m.group(1))

    # Preserve order / unique
    seen = set()
    out: List[str] = []
    for a in named:
        if a in seen:
            continue
        seen.add(a)
        out.append(a)
    return out


def _is_probable_code_addr(value: int) -> bool:
    """Heuristic: keep only addresses likely to be executable code.

    Covers common ESP32 Xtensa and ESP32-C3/C6 RISC-V code ranges.
    """
    # ESP32-C3/C6 app flash mapping commonly in 0x4200_0000...
    if 0x42000000 <= value <= 0x42FFFFFF:
        return True

    # ESP32 (Xtensa) flash (IROM) commonly in 0x400d_0000...
    if 0x400D0000 <= value <= 0x400DFFFF:
        return True

    # Xtensa IRAM code often in 0x4010_0000...
    if 0x40100000 <= value <= 0x401FFFFF:
        return True

    # ROM-ish (kept for context; may not resolve without ROM ELF)
    if 0x40000000 <= value <= 0x400FFFFF:
        return True

    return False


def _project_root() -> Path:
    # tools/ -> project root
    return Path(__file__).resolve().parents[1]


def _elf_path(env_name: str) -> Path:
    return _project_root() / ".pio" / "build" / env_name / "firmware.elf"


def _platformio_packages_dir() -> Path:
    return Path(os.environ.get("USERPROFILE", "")) / ".platformio" / "packages"


def _candidate_addr2line_paths() -> List[Path]:
    pkg = _platformio_packages_dir()
    # Common toolchains for this repo (ESP32 Xtensa, ESP32-C6 RISC-V)
    candidates = [
        pkg / "toolchain-xtensa-esp32" / "bin" / "xtensa-esp32-elf-addr2line.exe",
        pkg / "toolchain-xtensa-esp32" / "bin" / "xtensa-esp32-elf-addr2line",
        pkg / "toolchain-riscv32-esp" / "bin" / "riscv32-esp-elf-addr2line.exe",
        pkg / "toolchain-riscv32-esp" / "bin" / "riscv32-esp-elf-addr2line",
    ]
    # Also consider PATH if user has toolchains elsewhere
    candidates.extend([Path("xtensa-esp32-elf-addr2line"), Path("riscv32-esp-elf-addr2line")])
    return candidates


def _elf_machine(elf: Path) -> Optional[int]:
    """Return ELF e_machine or None if unknown."""
    try:
        data = elf.read_bytes()
    except Exception:
        return None

    if len(data) < 20 or data[0:4] != b"\x7fELF":
        return None

    elf_class = data[4]  # 1=32-bit, 2=64-bit
    elf_data = data[5]  # 1=little endian, 2=big endian
    if elf_data != 1:
        return None

    # e_machine is at offset 18 for both 32/64-bit ELF headers.
    try:
        (e_machine,) = struct.unpack_from("<H", data, 18)
        return int(e_machine)
    except Exception:
        return None


def _try_addr2line(tool: Path, elf: Path, addresses: List[str]) -> Tuple[bool, str]:
    # -p: pretty, -f: function, -i: inlines, -a: print address, -C: demangle C++
    cmd = [str(tool), "-pfiaC", "-e", str(elf), *addresses]
    try:
        completed = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
    except FileNotFoundError:
        return False, ""

    out = (completed.stdout or "").strip()
    err = (completed.stderr or "").strip()

    # Heuristics: if tool/ELF mismatch, addr2line usually errors like:
    # "...: file format not recognized" or "can't open ...".
    if completed.returncode != 0 and not out:
        return False, f"{out}\n{err}".strip()

    if "file format not recognized" in err.lower():
        return False, err

    return True, f"{out}\n{err}".strip()


def _select_addr2line(elf: Path, sample_addr: str) -> Path:
    # Prefer the toolchain that matches the ELF arch.
    # e_machine values: Xtensa=94, RISC-V=243
    machine = _elf_machine(elf)
    preferred: List[Path] = []
    others: List[Path] = []
    for tool in _candidate_addr2line_paths():
        name = tool.name.lower()
        if machine == 243 and "riscv32-esp-elf-addr2line" in name:
            preferred.append(tool)
        elif machine == 94 and "xtensa-esp32-elf-addr2line" in name:
            preferred.append(tool)
        else:
            others.append(tool)

    candidates = preferred + others

    # We try toolchains; whichever successfully resolves is used.
    last_error: Optional[str] = None
    for tool in candidates:
        ok, output = _try_addr2line(tool, elf, [sample_addr])
        if ok:
            return tool
        if output:
            last_error = output

    details = f"\nLast error:\n{last_error}\n" if last_error else ""
    raise RuntimeError(
        "Could not find a working addr2line tool. "
        "Make sure PlatformIO toolchains are installed (build once), "
        "or that addr2line is on PATH." + details
    )


def _extract_pc_addresses(text: str, *, max_addrs: int = 48) -> List[str]:
    """Extract a small, high-signal set of addresses to decode.

    Preference order:
    1) Explicit backtrace pairs (PC:SP)
    2) Named registers (MEPC/RA/PC/EPC)
    3) Other hex addresses filtered to probable code ranges
    """
    pcs: List[str] = []

    # Split by whitespace; keep things like 0x1234:0x5678
    for token in re.split(r"\s+", text.strip()):
        if not token:
            continue
        if ":" in token:
            left, _right = token.split(":", 1)
            if _HEX_RE.fullmatch(left):
                pcs.append(left)
                continue

    # Common abort/panic formats:
    #   abort() was called at PC 0x4210....
    #   Saved PC:0x400....
    for m in re.finditer(r"\b(?:PC\s*|Saved\s+PC:)\s*(0x[0-9a-fA-F]+)", text):
        pcs.append(m.group(1))

    # If it's a Guru Meditation dump, pull MEPC/RA too.
    pcs.extend(_extract_named_register_addrs(text))

    # Also scan the whole blob for likely code addresses (stack dumps often contain
    # the real call chain as 0x42xxxxxx values).
    for h in _HEX_RE.findall(text):
        v = _hex_to_int(h)
        if v is None:
            continue
        if _is_probable_code_addr(v):
            pcs.append(h)

    # If we still found nothing, keep any hex addresses at all.
    if not pcs:
        pcs = _HEX_RE.findall(text)

    # De-duplicate but preserve order
    seen = set()
    ordered: List[str] = []
    for pc in pcs:
        if pc in seen:
            continue
        seen.add(pc)
        ordered.append(pc)

    # Limit output so we don't print hundreds of ??? lines.
    if len(ordered) > max_addrs:
        ordered = ordered[:max_addrs]
    return ordered


def _read_input_text(arg: Optional[str]) -> str:
    if arg and arg.strip():
        return arg
    return sys.stdin.read()


def main() -> int:
    parser = argparse.ArgumentParser(description="Decode ESP32 backtraces using PlatformIO firmware.elf")
    parser.add_argument("-e", "--env", required=True, help="PlatformIO environment name (e.g., creatorsball)")
    parser.add_argument("--max", type=int, default=48, help="Maximum number of addresses to decode (default: 48)")
    parser.add_argument("text", nargs="?", help="Backtrace / crash text. If omitted, reads stdin.")
    args = parser.parse_args()

    elf = _elf_path(args.env)
    if not elf.exists():
        print(f"ELF not found: {elf}", file=sys.stderr)
        print("Build first with: platformio run -e " + args.env, file=sys.stderr)
        return 2

    raw = _read_input_text(args.text)
    pcs = _extract_pc_addresses(raw, max_addrs=max(1, args.max))
    if not pcs:
        print("No addresses found. Paste the full 'Backtrace:' line (or panic output).", file=sys.stderr)
        return 2

    tool = _select_addr2line(elf, pcs[0])

    ok, output = _try_addr2line(tool, elf, pcs)
    if not ok:
        print("addr2line failed.", file=sys.stderr)
        print(output, file=sys.stderr)
        return 1

    print(f"ELF:  {elf}")
    print(f"Tool: {tool}")
    print("\nDecoded PCs:\n")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
