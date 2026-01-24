# BLIP 
BLIP: Beautiful Light In Props

## Decoding Crash Backtraces

When the device prints a `Backtrace:` line (or panic output with `0x...` addresses), decode it against the exact matching PlatformIO build ELF.

- Build the same environment you flashed (example: `creatorsball`).
- Copy/paste the `Backtrace:` line into the decoder:

	- `python tools/decode_backtrace.py -e creatorsball "Backtrace: 0x...:0x... 0x...:0x..."`

Or pipe a captured log:

- `type crash.txt | python tools/decode_backtrace.py -e creatorsball`

## Live Serial Monitor (Auto-Decode)

For an always-human-readable crash workflow, use the decoded serial monitor. It watches for `Backtrace:` output, `abort() was called at PC ...`, and `Guru Meditation Error` panic dumps and prints a decoded section automatically.

- `python tools/monitor_decode.py -e creatorsball -p COM20`

If you see `Access is denied` on Windows, another serial monitor is already using the port — close it and retry.

If the device resets / disconnects and you want auto-reconnect:

- `python tools/monitor_decode.py -e creatorsball -p COM20 --reconnect`

## Automated Releases

Pushing a tag that matches the current `BLIP_VERSION` defined in `configs/blip.ini` triggers the `Build and Publish BLIP` GitHub Actions workflow. The pipeline:

- Confirms the tag version matches the firmware version baked into the configs (both `v1.2.3` and `1.2.3` style tags work).
- Runs `tools/minify_firstrun.py`, packages the dashboard assets via `tools/package_and_upload_server.py`, and executes `python tools/build_all.py -u` to build/upload every PlatformIO environment.
- Uploads both the generated firmware exports (`export/…`) and dashboard bundle (`export/releases/…`) as workflow artifacts.

Set the optional `BLIP_SERVER_UPLOAD_URL` secret if the default server upload endpoint needs to be overridden for private infrastructure.
