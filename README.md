# BLIP 
BLIP: Beautiful Light In Props

## Automated Releases

Pushing a tag that matches the current `BLIP_VERSION` defined in `configs/blip.ini` triggers the `Build and Publish BLIP` GitHub Actions workflow. The pipeline:

- Confirms the tag version matches the firmware version baked into the configs (both `v1.2.3` and `1.2.3` style tags work).
- Runs `tools/minify_firstrun.py`, packages the dashboard assets via `tools/package_and_upload_server.py`, and executes `python tools/build_all.py -u` to build/upload every PlatformIO environment.
- Uploads both the generated firmware exports (`export/…`) and dashboard bundle (`export/releases/…`) as workflow artifacts.

Set the optional `BLIP_SERVER_UPLOAD_URL` secret if the default server upload endpoint needs to be overridden for private infrastructure.
