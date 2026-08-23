#!/usr/bin/env python3
"""Upload an Android App Bundle to Google Play internal testing.

The default is a local dry run. A live release requires --commit --yes and
credentials supplied outside the repository. The script uses one Publishing
API edit: insert, bundle upload, track update, then commit.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

API_ROOT = "https://androidpublisher.googleapis.com/androidpublisher/v3/applications"
PLAY_SCOPE = "https://www.googleapis.com/auth/androidpublisher"
ALLOWED_TRACKS = {"qa", "beta", "alpha", "production"}


def fail(message: str) -> "NoReturn":
    raise RuntimeError(message)


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text())
    except FileNotFoundError:
        fail(f"credentials file not found: {path}")
    except json.JSONDecodeError as error:
        fail(f"invalid JSON in credentials file {path}: {error}")
    if not isinstance(value, dict):
        fail("credentials JSON must be an object")
    return value


def access_token(credentials: Path | None) -> str:
    direct = os.environ.get("GOOGLE_PLAY_ACCESS_TOKEN")
    if direct:
        return direct
    try:
        from google.oauth2 import service_account  # type: ignore
        from google.auth.transport.requests import Request as GoogleRequest  # type: ignore
    except ImportError:
        service_account = None
        GoogleRequest = None
    if credentials:
        if service_account is None or GoogleRequest is None:
            fail("service-account mode requires google-auth; install with: pip install google-auth")
        creds = service_account.Credentials.from_service_account_file(
            str(credentials), scopes=[PLAY_SCOPE]
        )
        creds.refresh(GoogleRequest())
        if not creds.token:
            fail("Google did not return an access token")
        return creds.token
    gcloud = shutil.which("gcloud")
    if gcloud:
        result = subprocess.run(
            [gcloud, "auth", "application-default", "print-access-token"],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    fail("no credentials: set GOOGLE_PLAY_ACCESS_TOKEN, pass --service-account, or configure gcloud application-default credentials")


def api_request(
    method: str,
    url: str,
    token: str,
    body: bytes | None = None,
    content_type: str = "application/json",
) -> dict[str, Any]:
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/json",
        "Content-Type": content_type,
    }
    request = Request(url, data=body, headers=headers, method=method)
    try:
        with urlopen(request, timeout=120) as response:
            payload = response.read().decode("utf-8")
    except HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"Play API {method} {url} returned HTTP {error.code}: {detail}") from error
    except URLError as error:
        raise RuntimeError(f"Play API request failed: {error.reason}") from error
    if not payload:
        return {}
    try:
        value = json.loads(payload)
    except json.JSONDecodeError as error:
        raise RuntimeError(f"Play API returned non-JSON data: {payload[:300]}") from error
    if not isinstance(value, dict):
        raise RuntimeError("Play API returned an unexpected JSON shape")
    return value


def validate(args: argparse.Namespace) -> None:
    if args.track not in ALLOWED_TRACKS:
        fail(f"unsupported track {args.track!r}; use qa for internal testing")
    if args.track != "qa" and not args.allow_non_internal:
        fail("this script defaults to internal testing; pass --allow-non-internal to use another track")
    if not args.package_name or "." not in args.package_name:
        fail("--package-name must be an Android application ID")
    if args.commit and not args.yes:
        fail("live publishing requires both --commit and --yes")
    if not args.aab.is_file():
        fail(f"AAB not found: {args.aab}")
    if args.aab.suffix.lower() != ".aab":
        fail("--aab must point to an .aab file")
    if args.aab.stat().st_size < 1024:
        fail("AAB is suspiciously small")


def dry_run(args: argparse.Namespace) -> None:
    size = args.aab.stat().st_size
    print("DRY RUN: no network request and no Play Console change was made.")
    print(f"package={args.package_name}")
    print(f"aab={args.aab.resolve()}")
    print(f"aab_bytes={size}")
    print(f"track={args.track}")
    print("steps=insert_edit,upload_bundle,update_qa_track,commit_edit")
    print("credentials=not loaded")


def publish(args: argparse.Namespace) -> None:
    token = access_token(args.service_account)
    base = f"{API_ROOT}/{args.package_name}"
    edit = api_request("POST", f"{base}/edits", token, b"{}")
    edit_id = edit.get("id")
    if not edit_id:
        fail(f"Play API did not return an edit id: {edit}")
    print(f"edit_id={edit_id}")
    bundle_url = f"{base}/edits/{edit_id}/bundles/upload?uploadType=media"
    bundle = api_request(
        "POST",
        bundle_url,
        token,
        args.aab.read_bytes(),
        "application/octet-stream",
    )
    version_code = bundle.get("versionCode")
    if version_code is None:
        fail(f"Play API did not return a version code: {bundle}")
    print(f"version_code={version_code}")
    release: dict[str, Any] = {
        "versionCodes": [str(version_code)],
        "status": "completed",
        "name": args.release_name,
    }
    if args.release_notes:
        release["releaseNotes"] = [{"language": args.language, "text": args.release_notes}]
    track_body = json.dumps({"track": args.track, "releases": [release]}).encode()
    api_request(
        "PUT",
        f"{base}/edits/{edit_id}/tracks/{args.track}",
        token,
        track_body,
    )
    print(f"track_updated={args.track}")
    committed = api_request("POST", f"{base}/edits/{edit_id}:commit", token, b"{}")
    print(f"committed_edit={committed.get('id', edit_id)}")
    print("PUBLISH_SUCCESS=1")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--aab", type=Path, required=True, help="signed Android App Bundle")
    parser.add_argument("--package-name", required=True, help="Play application ID")
    parser.add_argument("--track", default="qa", help="qa for internal testing")
    parser.add_argument("--release-name", default="Automated internal build")
    parser.add_argument("--release-notes", default="")
    parser.add_argument("--language", default="en-US")
    parser.add_argument("--service-account", type=Path, help="service-account JSON outside the repository")
    parser.add_argument("--allow-non-internal", action="store_true")
    parser.add_argument("--commit", action="store_true", help="perform the live upload and commit")
    parser.add_argument("--yes", action="store_true", help="confirm live publishing")
    args = parser.parse_args()
    try:
        validate(args)
        if not args.commit:
            dry_run(args)
        else:
            publish(args)
    except (RuntimeError, OSError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
