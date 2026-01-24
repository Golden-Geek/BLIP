#!/usr/bin/env python3
"""Serial monitor that auto-decodes ESP32 backtraces using PlatformIO ELF.

Why this exists
--------------
PlatformIO's built-in `esp32_exception_decoder` monitor filter can be missing,
limited, or mismatch ROM symbols across different ESP32 variants.

This script:
- Streams serial output like a normal monitor
- Detects `Backtrace:` blocks
- Automatically resolves addresses to function + file:line using:
  `.pio/build/<env>/firmware.elf`

Usage
-----
  python tools/monitor_decode.py -e creatorsball -p COM20

Tips
----
- Decode works only if the ELF matches the flashed firmware.
- For best decode detail, keep debug symbols (we already use `-g1` in blip.ini).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional


def _project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _elf_path(env_name: str) -> Path:
    return _project_root() / ".pio" / "build" / env_name / "firmware.elf"


def _platformio_python() -> Optional[Path]:
    # Prefer PlatformIO's venv python if present so dependencies match.
    userprofile = os.environ.get("USERPROFILE")
    if not userprofile:
        return None
    candidate = Path(userprofile) / ".platformio" / "penv" / "Scripts" / "python.exe"
    return candidate if candidate.exists() else None


def _decode_backtrace(env_name: str, text: str) -> str:
    py = _platformio_python() or Path(sys.executable)
    script = _project_root() / "tools" / "decode_backtrace.py"

    cmd = [str(py), str(script), "-e", env_name, text]
    completed = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return (completed.stdout or "").strip()


def _looks_like_backtrace_line(line: str) -> bool:
    return "Backtrace:" in line or "backtrace:" in line


def _looks_like_guru_meditation(line: str) -> bool:
    return "Guru Meditation Error" in line


def _looks_like_abort_pc(line: str) -> bool:
    return "abort() was called at PC" in line


def _contains_hex_addr(line: str) -> bool:
    # Lightweight heuristic to keep collecting wrapped backtrace lines
    return "0x" in line


def main() -> int:
    parser = argparse.ArgumentParser(description="Serial monitor with automatic backtrace decoding")
    parser.add_argument("-e", "--env", required=True, help="PlatformIO environment name (e.g., creatorsball)")
    parser.add_argument("-p", "--port", required=True, help="Serial port (e.g., COM20)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--reconnect", action="store_true", help="Auto-reconnect on disconnect")
    args = parser.parse_args()

    elf = _elf_path(args.env)
    if not elf.exists():
        print(f"ELF not found: {elf}", file=sys.stderr)
        print(f"Build first: platformio run -e {args.env}", file=sys.stderr)
        return 2

    try:
        import serial  # type: ignore
    except Exception as exc:  # pragma: no cover
        print("Missing dependency: pyserial", file=sys.stderr)
        print(f"Error: {exc}", file=sys.stderr)
        print("Fix: use PlatformIO's python (recommended) or `pip install pyserial`", file=sys.stderr)
        return 2

    def open_serial() -> "serial.Serial":
        # timeout keeps the loop responsive
        return serial.Serial(args.port, args.baud, timeout=0.2)

    print(f"--- BLIP decoded monitor | env={args.env} | port={args.port} | baud={args.baud}")
    print(f"--- ELF: {elf}")
    print("--- Quit: Ctrl+C")

    ser: Optional["serial.Serial"] = None
    last_error: Optional[str] = None
    last_error_at = 0.0

    while True:
        try:
            if ser is None:
                ser = open_serial()

            line_bytes = ser.readline()
            if not line_bytes:
                continue

            # Decode with replacement to avoid blowing up on binary garbage
            line = line_bytes.decode("utf-8", errors="replace").rstrip("\r\n")
            print(line)

            if _looks_like_backtrace_line(line):
                # Collect this and any continuation lines that still contain addresses.
                collected = [line]
                start = time.time()

                # ESP sometimes wraps backtrace across multiple lines; capture briefly.
                while time.time() - start < 1.0:
                    nxt = ser.readline()
                    if not nxt:
                        continue
                    nxt_s = nxt.decode("utf-8", errors="replace").rstrip("\r\n")
                    print(nxt_s)
                    if _contains_hex_addr(nxt_s):
                        collected.append(nxt_s)
                        start = time.time()  # extend window while addresses keep coming
                    else:
                        break

                blob = "\n".join(collected)
                decoded = _decode_backtrace(args.env, blob)
                print("\n=== Decoded backtrace ===")
                print(decoded)
                print("=== End decoded backtrace ===\n")

            if _looks_like_abort_pc(line) or _looks_like_guru_meditation(line):
                # Collect a larger panic block (register dump + a bit of stack).
                collected = [line]
                start = time.time()

                # Capture for a short period, extending while data keeps coming.
                # This avoids hanging forever if the device keeps logging.
                while time.time() - start < 2.0:
                    nxt = ser.readline()
                    if not nxt:
                        continue
                    nxt_s = nxt.decode("utf-8", errors="replace").rstrip("\r\n")
                    print(nxt_s)
                    collected.append(nxt_s)

                    # Extend window while we're still in the panic block.
                    if "Stack memory" in nxt_s or "register dump" in nxt_s or "MEPC" in nxt_s or "0x" in nxt_s:
                        start = time.time()

                    # Heuristic stop if it looks like we're back to normal logging.
                    if nxt_s.startswith("[") and "]" in nxt_s and "0x" not in nxt_s:
                        break

                blob = "\n".join(collected)
                decoded = _decode_backtrace(args.env, blob)
                print("\n=== Decoded panic (best-effort) ===")
                print(decoded)
                print("=== End decoded panic ===\n")

        except KeyboardInterrupt:
            return 0
        except Exception as exc:
            # Serial disconnect or access issue
            msg = str(exc)
            now = time.time()
            # Don't spam the console when we're in reconnect loops.
            if msg != last_error or (now - last_error_at) > 5.0:
                print(f"[monitor_decode] error: {exc}", file=sys.stderr)
                if "Access is denied" in msg or "PermissionError" in msg:
                    print(
                        "[monitor_decode] Hint: COM port is busy. Close other serial monitors (PlatformIO/Arduino/putty) and retry.",
                        file=sys.stderr,
                    )
                last_error = msg
                last_error_at = now
            try:
                if ser is not None:
                    ser.close()
            except Exception:
                pass
            ser = None

            if not args.reconnect:
                return 1

            time.sleep(0.5)


if __name__ == "__main__":
    raise SystemExit(main())
