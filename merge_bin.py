from SCons.Script import Import
import os
import shutil
import json
from pathlib import Path

Import("env")

def clean_define(value):
    """Remove surrounding escaped quotes and spaces"""
    if isinstance(value, str):
        return value.strip().strip('\\"')
    return value

def extract_define(defines, name, default="unknown"):
    """Extracts a define like -DNAME="value" from CPPDEFINES"""
    for item in defines:
        if isinstance(item, tuple) and item[0] == name:
            return clean_define(item[1])
    return default

def parse_vidpid(value, fallback):
    """Parses comma-separated VID/PID string into list"""
    if not value or value == "unknown":
        return fallback
    return [v.strip() for v in value.split(",")]

def camel_case(name):
    """Convert name to UpperCamelCase (remove spaces, capitalize words)"""
    import re
    return ''.join(word.capitalize() for word in re.split(r'\W+', name) if word)

def merge_bin(source, target, env):
    chip = env.BoardConfig().get("build.mcu", "esp32")
    build_dir = Path(env.subst("$BUILD_DIR"))

    # Extract defines
    cpp_defines = env.get("CPPDEFINES", [])
    device_type = extract_define(cpp_defines, "DEVICE_TYPE", env.subst("$PIOENV"))
    blip_version = extract_define(cpp_defines, "BLIP_VERSION", "0.0.0")
    vids_str = extract_define(cpp_defines, "VIDS", "")
    pids_str = extract_define(cpp_defines, "PIDS", "")

    vids = parse_vidpid(vids_str, ["0x303A"])
    pids = parse_vidpid(pids_str, ["0x1001"])

    print(f"[Merge] DEVICE_TYPE='{device_type}', VERSION='{blip_version}', VIDS={vids}, PIDS={pids}")

    # Merge firmware
    merged_firmware = build_dir / "merge_firmware.bin"
    
    framework_dir = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))

    # Set correct sections based on chip
    if chip == "esp32":
        sections = [
            (0x1000, build_dir / "bootloader.bin"),
            (0x8000, build_dir / "partitions.bin"),
            (0xe000, framework_dir / "tools/partitions/boot_app0.bin"),
            (0x10000, build_dir / "firmware.bin"),
        ]
    elif chip == "esp32c6":
        sections = [
            (0x0, build_dir / "bootloader.bin"),
            (0x8000, build_dir / "partitions.bin"),
            (0xe000, framework_dir / "tools/partitions/boot_app0.bin"),
            (0x10000, build_dir / "firmware.bin"),
        ]
    else:
        print(f"[Merge] ERROR: Unknown chip type '{chip}'")
        env.Exit(1)

    for offset, path in sections:
        if not path.is_file():
            print(f"[Merge] ERROR: Missing {path} at offset {hex(offset)}")
            env.Exit(1)
    python_exe = env.subst("$PYTHONEXE")
    esptool_py = Path(env.PioPlatform().get_package_dir("tool-esptoolpy")) / "esptool"
    merge_cmd = f"\"{python_exe}\" \"{esptool_py}\" --chip {chip} merge-bin -o \"{merged_firmware}\" " + \
                " ".join([f"{hex(offset)} \"{path}\"" for offset, path in sections])
    print(f"[Merge] Running: {merge_cmd}")
    if env.Execute(merge_cmd) != 0:
        print("[Merge] ERROR: Failed to merge binaries")
        env.Exit(1)
    print(f"[Merge] Created: {merged_firmware}")

    # Write manifest.json
    manifest = {
        "name": device_type,
        "version": blip_version,
        "chip": chip,
        "vids": vids,
        "pids": pids,
        "espOptions": {
            "baud": env.get("ESP_BAUD", 921600),
            "before": env.get("ESP_BEFORE", "default_reset"),
            "after": env.get("ESP_AFTER", "hard_reset")
        },
        "flashOptions": {
            "flash_mode": env.get("FLASH_MODE", "dio"),
            "flash_freq": env.get("FLASH_FREQ", "80m"),
            "flash_size": env.get("FLASH_SIZE", "4MB")
        }
    }
    manifest_path = build_dir / "manifest.json"
    with manifest_path.open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    print(f"[Merge] Created: {manifest_path}")

    # Export
    export_dir = Path("export") / camel_case(device_type)
    export_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(merged_firmware, export_dir / "firmware.bin")
    shutil.copy2(manifest_path, export_dir / "manifest.json")
    print(f"📦 Exported to: {export_dir}")

# Run after firmware build
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)
