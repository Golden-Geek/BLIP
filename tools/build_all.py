import subprocess
import os
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
import configparser
import glob
from pathlib import Path
import json
import requests
import argparse
import shutil

# Parse environments from platformio.ini and extra_configs
def get_all_envs():
    envs = []
    base_ini = Path("platformio.ini")
    if not base_ini.exists():
        print("❌ platformio.ini not found!")
        sys.exit(1)

    config = configparser.ConfigParser()
    config.read(base_ini)

    # Check for extra_configs
    extra_configs = []
    if "platformio" in config and "extra_configs" in config["platformio"]:
        extra_configs_raw = config["platformio"]["extra_configs"].splitlines()
        for line in extra_configs_raw:
            line = line.strip()
            if line and not line.startswith(";"):
                extra_configs.extend(glob.glob(line))

    # Merge all configs (base + extras)
    all_configs = [str(base_ini)] + extra_configs
    config.read(all_configs)

    # Collect all [env:...] sections
    for section in config.sections():
        if section.startswith("env:"):
            env_name = section.split("env:")[1]
            envs.append(env_name)

    if not envs:
        print("❌ No environments found in platformio.ini or extra_configs.")
        sys.exit(1)

    return envs

# Build a single environment
def build_env(env_name, results):
    print(f"🔨 Building {env_name}...")
    try:
        subprocess.run(["pio", "run", "-e", env_name], check=True)
        results["success"].append(env_name)
        print(f"✅ {env_name} build completed.")
    except subprocess.CalledProcessError:
        results["failed"].append(env_name)
        print(f"❌ Build failed for {env_name}")

# Upload all exported folders
def upload_exports(auto_upload=False):
    export_dir = Path("export")
    if not export_dir.exists():
        print("ℹ️ Creating export folder.")
        export_dir.mkdir(parents=True, exist_ok=True)

    folders = [f for f in export_dir.iterdir() if f.is_dir()]
    if not folders:
        print("⚠️ No exported builds to upload.")
        return

    print("\n📦 Found exported builds:")
    for folder in folders:
        print(f"  - {folder.name}")

    if not auto_upload:
        confirm = input("\n📤 Do you want to upload all exported builds to the server? [y/N]: ").strip().lower()
        if confirm != "y":
            print("🚫 Upload skipped.")
            return
    else:
        print("📤 Auto upload enabled: uploading all exported builds...")

    url = "https://www.goldengeek.org/blip/download/firmware/uploadfw.php"  # Set your server URL here

    for folder in folders:
        firmware = folder / "firmware.bin"
        firmware_full = folder / "firmware_full.bin"
        manifest = folder / "manifest.json"
        if not firmware.exists() or not manifest.exists():
            print(f"⚠️ Skipping {folder.name}: Missing firmware or manifest.")
            continue

        print(f"📤 Uploading {folder.name} to {url}...")
        files = {
            "firmware": open(firmware, "rb"),
            "firmware_full": open(firmware_full, "rb"),
            "manifest": open(manifest, "rb")
        }
        data = {"device_type": folder.name}
        try:
            print(f"   - Sending data: {data}")
            response = requests.post(url, files=files, data=data)
            if response.status_code == 200:
                print(f"✅ {folder.name} upload successful!")
            else:
                print(f"❌ {folder.name} upload failed: {response.status_code} {response.text}")
        except Exception as e:
            print(f"❌ Upload error for {folder.name}: {e}")
        finally:
            for f in files.values():
                f.close()


# Main parallel build logic
def main(selected_envs, auto_upload=False):
    # If user requested upload-only, skip builds and upload existing exports
    if "--upload-only" in sys.argv:
        print("📤 Upload-only mode: uploading exported builds and exiting.")
        upload_exports(auto_upload=True)
        return
    
    all_envs = get_all_envs()
    envs = selected_envs if selected_envs else all_envs

    unknown_envs = [env for env in envs if env not in all_envs]
    if unknown_envs:
        print(f"❌ Unknown environments: {', '.join(unknown_envs)}")
        print(f"✅ Available environments: {', '.join(all_envs)}")
        sys.exit(1)

    max_jobs = os.cpu_count() or 4
    results = {"success": [], "failed": []}

    # Remove export folder if it exists before building
    export_dir = Path("export")
    if export_dir.exists() and export_dir.is_dir():
        shutil.rmtree(export_dir)

    # Remove .pio/build folder if it exists before building
    pio_build_dir = Path(".pio/build")
    if pio_build_dir.exists() and pio_build_dir.is_dir():
        shutil.rmtree(pio_build_dir)


    print(f"🚀 Starting parallel build for environments: {', '.join(envs)}")
    with ThreadPoolExecutor(max_workers=max_jobs) as executor:
        futures = {executor.submit(build_env, env, results): env for env in envs}
        for future in as_completed(futures):
            future.result()

    # Summary
    print("\n📋 Build Summary:")
    if results["success"]:
        print("✅ Successful builds:")
        for env in results["success"]:
            print(f"  - {env}")
    if results["failed"]:
        print("❌ Failed builds:")
        for env in results["failed"]:
            print(f"  - {env}")

    if results["success"]:
        upload_exports(auto_upload)

    if results["failed"]:
        print("\n⚠️ Some builds failed.")
        sys.exit(1)
    else:
        print("\n🎉 All builds succeeded.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build and optionally upload firmwares")
    parser.add_argument("environments", nargs="*", help="Specific environments to build")
    parser.add_argument("-u", "--upload", action="store_true", help="Auto upload after successful builds")
    parser.add_argument("--upload-only", action="store_true", help="Upload exported builds without building")
    args = parser.parse_args()

    main(args.environments, auto_upload=args.upload)
