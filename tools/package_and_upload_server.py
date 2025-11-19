import argparse
import hashlib
import json
import mimetypes
import re
import sys
import uuid
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Tuple
from urllib import parse, request

DEFAULT_UPLOAD_URL = "https://goldengeek.org/blip/download/server/uploadserver.php"
DEFAULT_PUBLIC_BASE_URL = "https://www.goldengeek.org/blip/download/server/download.php?file={file}"
DEFAULT_MANIFEST_NAME = "latest.json"
MAX_REDIRECTS = 3
VERSION_REGEX = re.compile(r"<!--\s*Version\s+([^\s]+)\s*-->", re.IGNORECASE)


def extract_version(edit_html: Path) -> str:
    for line in edit_html.read_text(encoding="utf-8").splitlines():
        match = VERSION_REGEX.search(line)
        if match:
            return match.group(1)
    raise ValueError(f"Could not find version comment in {edit_html}")


def build_zip(src_dir: Path, version: str, output_dir: Path) -> Path:
    if not src_dir.exists():
        raise FileNotFoundError(f"Source directory '{src_dir}' does not exist")

    output_dir.mkdir(parents=True, exist_ok=True)
    archive_path = output_dir / f"server-compressed-{version}.zip"

    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path in sorted(src_dir.rglob("*")):
            if path.is_file():
                arcname = path.relative_to(src_dir)
                zf.write(path, arcname)
    return archive_path


def compute_sha256(file_path: Path) -> str:
    digest = hashlib.sha256()
    with file_path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_download_url(base: str, filename: str) -> str:
    placeholder = "{file}"
    if placeholder in base:
        return base.replace(placeholder, filename)

    normalized = base.rstrip("/")
    if normalized.endswith("?"):
        return normalized + filename

    return normalized + "/" + filename


def write_manifest(version: str, archive: Path, download_url: str, manifest_path: Path) -> Path:
    manifest = {
        "version": version,
        "url": download_url,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "size": archive.stat().st_size,
        "checksum": {
            "alg": "sha256",
            "value": compute_sha256(archive),
        },
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest_path


def _encode_form(fields: Dict[str, str], file_field: str, file_path: Path) -> Tuple[bytes, str]:
    boundary = uuid.uuid4().hex
    data: bytearray = bytearray()

    def add_line(line: str = "") -> None:
        data.extend(line.encode("utf-8"))
        data.extend(b"\r\n")

    for key, value in fields.items():
        add_line(f"--{boundary}")
        add_line(f"Content-Disposition: form-data; name=\"{key}\"")
        add_line()
        add_line(value)

    mime_type = mimetypes.guess_type(file_path.name)[0] or "application/octet-stream"
    add_line(f"--{boundary}")
    add_line(
        "Content-Disposition: form-data; name=\"{}\"; filename=\"{}\"".format(
            file_field, file_path.name
        )
    )
    add_line(f"Content-Type: {mime_type}")
    add_line()
    data.extend(file_path.read_bytes())
    data.extend(b"\r\n")
    add_line(f"--{boundary}--")

    content_type = f"multipart/form-data; boundary={boundary}"
    return bytes(data), content_type


class _NoRedirect(request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):  # type: ignore[override]
        return None


def upload_archive(archive: Path, version: str, upload_url: str) -> str:
    fields = {"version": version}
    body, content_type = _encode_form(fields, "file", archive)
    opener = request.build_opener(_NoRedirect)

    current_url = upload_url
    for attempt in range(MAX_REDIRECTS + 1):
        req = request.Request(current_url, method="POST")
        req.add_header("Content-Type", content_type)
        req.add_header("Content-Length", str(len(body)))
        try:
            with opener.open(req, data=body, timeout=60) as resp:
                return resp.read().decode("utf-8", errors="replace")
        except request.HTTPError as exc:
            if exc.code in (301, 302, 303, 307, 308):
                location = exc.headers.get("Location")
                if not location:
                    raise
                current_url = parse.urljoin(current_url, location)
                continue
            raise

    raise RuntimeError(
        f"Upload redirected more than {MAX_REDIRECTS} times; last URL was {current_url}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Package server assets and upload them.")
    parser.add_argument("--edit-html", default="data/server/edit.html", help="Path to edit.html")
    parser.add_argument(
        "--compressed-dir",
        default="data/server-compressed",
        help="Directory containing pre-compressed assets",
    )
    parser.add_argument(
        "--output-dir", default="export/releases", help="Directory for generated archives"
    )
    parser.add_argument(
        "--upload-url", default=DEFAULT_UPLOAD_URL, help="Remote upload endpoint"
    )
    parser.add_argument(
        "--skip-upload", action="store_true", help="Create the archive but do not upload"
    )
    parser.add_argument(
        "--public-url-base",
        default=DEFAULT_PUBLIC_BASE_URL,
        help="Public URL base or template for the uploaded archives. Use {file} as a placeholder to inject the filename.",
    )
    parser.add_argument(
        "--manifest-name",
        default=DEFAULT_MANIFEST_NAME,
        help="Filename to use for the uploaded JSON manifest",
    )
    parser.add_argument(
        "--skip-manifest",
        action="store_true",
        help="Do not generate or upload the manifest JSON",
    )

    args = parser.parse_args()
    edit_html = Path(args.edit_html)
    compressed_dir = Path(args.compressed_dir)
    output_dir = Path(args.output_dir)

    try:
        version = extract_version(edit_html)
    except Exception as exc:
        print(f"Error reading version: {exc}", file=sys.stderr)
        sys.exit(1)

    try:
        archive = build_zip(compressed_dir, version, output_dir)
    except Exception as exc:
        print(f"Error creating archive: {exc}", file=sys.stderr)
        sys.exit(1)

    print(f"Created archive: {archive}")

    archive_download_url = resolve_download_url(args.public_url_base, archive.name)
    manifest_path = None

    if not args.skip_manifest:
        manifest_path = output_dir / args.manifest_name
        try:
            write_manifest(version, archive, archive_download_url, manifest_path)
            print(f"Wrote manifest: {manifest_path}")
        except Exception as exc:
            print(f"Error writing manifest: {exc}", file=sys.stderr)
            sys.exit(1)

    if args.skip_upload:
        print("--skip-upload enabled; skipping upload step.")
        return

    try:
        response = upload_archive(archive, version, args.upload_url)
        print("Archive upload completed. Server response:")
        print(response)

        if manifest_path and not args.skip_manifest:
            manifest_response = upload_archive(manifest_path, version, args.upload_url)
            print("Manifest upload completed. Server response:")
            print(manifest_response)
    except Exception as exc:
        print(f"Upload failed: {exc}", file=sys.stderr)
        if isinstance(exc, request.HTTPError):
            print("Server response:", file=sys.stderr)
            try:
                print(exc.read().decode("utf-8", errors="replace"), file=sys.stderr)
            except Exception:
                pass
        sys.exit(1)


if __name__ == "__main__":
    main()
