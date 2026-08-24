#!/usr/bin/env python3
"""Validate the Google-hosted plus guest-local Android login contract."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt"
SESSION = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL dual_login_contract: {message}")


def section(source: str, start: str, end: str) -> str:
    require(start in source and end in source, f"missing section boundary: {start}")
    return source.split(start, 1)[1].split(end, 1)[0]


def main() -> None:
    main = MAIN.read_text(encoding="utf-8")
    session = SESSION.read_text(encoding="utf-8")
    release_lock = (ROOT / "config/RELEASE_LOCK.ini").read_text(encoding="utf-8")

    for needle in (
        "SIGN IN WITH GOOGLE",
        "requestGoogleAccountLink()",
        "CONTINUE AS GUEST",
        "requestGuestEntry()",
        "GUEST MODE READY",
        "GUEST LOCAL",
        "saveGuestWorldState",
        "guestWorldState",
        "GUEST_DEFAULT_WORLD_STATE",
        "NativeGameBridge.loadCloudState(savedState)",
        "HOSTED CO-OP UNAVAILABLE",
        "hosted co-op requires Google login",
    ):
        require(needle in main, f"missing dual-login marker: {needle}")
    for needle in (
        "fun requestGuestSignIn(): SessionSnapshot",
        "without Gmail, a backend request, or a hosted session",
        "const val GUEST_PREFS",
        "private fun restoreGuestSession",
        "isGuest = true",
    ):
        require(needle in session, f"missing local guest-session marker: {needle}")

    guest_request = section(session, "fun requestGuestSignIn(): SessionSnapshot", "fun requestGoogleSignIn(): SessionSnapshot")
    require("postJson(" not in guest_request and "requestJson(" not in guest_request, "guest entry must not call the backend")
    require("refreshToken" not in guest_request and "accessToken" not in guest_request, "guest entry must not create cloud tokens")

    startup = section(main, "private fun beginOnlineStartup()", "private fun applyConnectivitySnapshot")
    require(startup.index("snapshot.isGuest") < startup.index("if (!networkOnline)"), "guest startup must be evaluated before network blocking")
    require("continuePendingWorldEntry()" in startup, "guest startup must resume after Stage 1 content preparation")

    setup = section(main, "private fun showCharacterSetup", "private fun nextCoOpRequestId")
    require("ENTER LOCAL WORLD" in setup, "guest setup must provide local-world entry")
    require("NativeGameBridge.loadCloudState" in setup, "guest setup must restore the existing versioned native snapshot")
    require("activeCloudWorld = null" in setup, "guest setup must not claim a cloud world")
    require("CREATE CLOUD WORLD" in setup and "RESUME SELECTED WORLD" in setup, "Google cloud-world actions must remain available")

    co_op = section(main, "private fun showCoOpDialog()", "private fun setPlayerName")
    require(co_op.index("if (accountSession.snapshot.isGuest)") < co_op.index("HOSTED INTERNET CO-OP"), "guest co-op guard must run before hosted controls")
    require("SWITCH TO GOOGLE" in co_op, "guest co-op dialog must offer a Google-login path")

    local_permissions = section(main, "private fun ensureLocalPermissions", "override fun onRequestPermissionsResult")
    require("snapshot.isGuest" in local_permissions and "return false" in local_permissions, "guest mode must block LAN/Wi-Fi multiplayer helpers")

    actions = section(main, "private fun requireOnline", "private fun detectSupportedTargetFps")
    require('snapshot.isGuest && activeCoOpRoom == null && action != "CO-OP"' in actions, "guest local actions must bypass network availability")
    require("Mode=dual-entry" in release_lock, "release lock must declare dual-entry")

    print("DUAL_LOGIN_CONTRACT_PASS=1")
    print("GOOGLE_HOSTED_LOGIN=preserved")
    print("GUEST_LOCAL_LOGIN=enabled")
    print("GUEST_HOSTED_MULTIPLAYER=blocked")


if __name__ == "__main__":
    main()
