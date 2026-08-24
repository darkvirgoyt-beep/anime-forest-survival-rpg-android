#!/usr/bin/env python3
"""Validate that the Android release has one production online launch path."""
from __future__ import annotations

import configparser
from pathlib import Path


REQUIRED = (
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "beginOnlineStartup", "automatic online startup"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "requestGoogleSignIn", "Google production login"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "SIGN IN WITH GOOGLE", "visible Google login control"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "requestProductionContent", "production content request"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "ENTER CORE ONLINE WORLD", "safe core-world entry when optional high graphics are unavailable"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "markCoreOnlineContentReady", "core-world content readiness boundary"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "override fun onBackPressed", "Android back navigation guard"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "Do you want to close this game?", "close confirmation copy"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "CO-OP READY", "online co-op trust row"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "HOSTED INTERNET CO-OP  •  FREE RENDER SERVICE", "hosted Render multiplayer heading"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "No Termux, local server, or same Wi-Fi is required.", "phone-only hosted multiplayer path"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "RECONNECT HOSTED ROOM", "hosted room reconnect action"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "finishAndRemoveTask()", "confirmed game close"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "isVerticalScrollBarEnabled = true", "scrollable character setup"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/PrivateContentDownloader.kt", "archiveSha256", "private archive verification"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/PrivateContentDownloader.kt", "Private high-end content manifest HTTP", "specific private-content delivery diagnostics"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt", "Google sign-in is not configured for this installed APK", "actionable Android OAuth configuration diagnostic"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt", "Google sign-in is still starting", "actionable credential lifecycle diagnostic"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "requestGoogleAccountLink()", "network-gated Google sign-in button"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "RETRY GOOGLE SIGN-IN", "recoverable cancelled Google chooser retry"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "GOOGLE ACCOUNT CHOOSER IS ALREADY OPEN", "duplicate Google chooser launch guard"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "does not read Gmail, Drive, or contacts", "identity-only cloud-save consent explanation"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "accountLinkConsentAccepted", "required account-link consent gate"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt", "GetGoogleIdOption.Builder", "standard all-account Google identity request"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt", "setFilterByAuthorizedAccounts(false)", "all installed Google accounts are eligible for identity login"),
    ("server/src/server.mjs", "/v1/content/high/manifest", "private high-end manifest route"),
    ("server/src/server.mjs", "/v1/content/high/archive", "private high-end archive route"),
    ("server/.env.example", "PRIVATE_CONTENT_ARCHIVE_PATH", "private archive server configuration"),
    ("app/build.gradle.kts", 'namespace = "com.darkvirgoyt.aethelgrad"', "exact Android namespace"),
    ("app/build.gradle.kts", 'applicationId = "com.darkvirgoyt.aethelgrad"', "exact Android application ID"),
    ("app/src/main/res/values/strings.xml", "https://aethelgard-api-v2.onrender.com/v1", "managed Render v2 API base"),
    ("render.yaml", "dockerContext: ./server", "server Docker build context"),
    ("render.yaml", "DATABASE_SSL_VERIFY", "Render PostgreSQL certificate setting"),
    ("server/src/security.mjs", "databaseSslVerify", "shared PostgreSQL TLS configuration"),
    ("render.yaml", "healthCheckPath: /healthz", "Render health check"),
    ("server/Dockerfile", "CMD [\"node\", \"scripts/start.mjs\"]", "migration-aware container startup"),
    ("server/scripts/start.mjs", "await import(\"./migrate.mjs\")", "startup database migration"),
    ("app/build.gradle.kts", "release {", "release build contract"),
    (".github/workflows/android-build.yml", "gradle bundleRelease assembleRelease", "release CI build"),
    ("tools/build_expansion_obb.py", "production-v1", "production content version"),
    ("server/src/server.mjs", "MAX_COOP_PLAYERS = 4", "strict four-player server cap"),
    ("server/src/server.mjs", "maxPlayers: MAX_COOP_PLAYERS", "four-player room snapshot"),
    ("server/test/coop_room_simulation.test.mjs", "reconnect_capacity_rejected", "durable reconnect cap regression"),
    ("docs/MULTIPLAYER_WORKFLOW.md", "max_players = 4", "documented four-player cap"),
)

FORBIDDEN = (
    ("app/build.gradle.kts", 'create("prototype")', "prototype build type"),
    ("app/build.gradle.kts", "PROTOTYPE_MODE", "prototype build flag"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "PROTOTYPE_MODE", "prototype runtime flag"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "AssetDeliveryManager", "local asset fallback"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "showLocalPreparationFallback", "local preparation fallback"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "offlinePrototypeMode", "offline prototype runtime mode"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "PLAY OFFLINE PROTOTYPE", "offline prototype entry button"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "canUseBundledFreeFallback", "bundled graphics bypass"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "requestGuestSignIn", "automatic guest login"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "GUEST SESSION", "guest login UI"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "OPTIONAL GOOGLE LINK", "optional login copy"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "FREE LOCAL GRAPHICS MODE READY", "bundled graphics mode"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "activateOfflinePrototype", "offline prototype entry"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AssetPackCatalog.kt", "canUseBundledFreeFallback", "bundled graphics bypass"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt", "requestGuestSignIn", "guest auth implementation"),
    ("app/src/main/res/values/strings.xml", "auth_guest_url", "guest auth endpoint"),
    (".github/workflows/android-build.yml", "assemblePrototype", "prototype CI build"),
    (".github/workflows/android-build.yml", "aethelgard-prototype-apk", "prototype CI artifact"),
    ("tools/build_expansion_obb.py", "prototype-v1", "prototype content version"),
    ("app/src/main/assets/asset_manifest.json", "LOW RESOURCES", "low-end player resource tier"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "SELECT GRAPHICS QUALITY", "player-facing resource tier selector"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "LOCAL MULTIPLAYER  •  SAME WI-FI OR WI-FI DIRECT", "local multiplayer UI removed from hosted flow"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "HOST LAN ROOM", "local LAN host control removed from hosted flow"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "CREATE WI-FI DIRECT GROUP", "local Wi-Fi Direct control removed from hosted flow"),
)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    failures: list[str] = []
    release_lock = root / "config/RELEASE_LOCK.ini"
    lock = configparser.ConfigParser()
    if not release_lock.is_file():
        failures.append("missing immutable release configuration lock")
    else:
        lock.read(release_lock)
        expected_lock = {
            ("Manifest", "Version"): "1",
            ("Identity", "AndroidPackage"): "com.darkvirgoyt.aethelgrad",
            ("Identity", "JniPrefix"): "Java_com_darkvirgoyt_aethelgrad_",
            ("Identity", "LauncherLabel"): "AETHELGARD: Wild Horizons",
            ("Platform", "MinSdk"): "30",
            ("Online", "Mode"): "online-only",
            ("Online", "GameAuthBase"): "https://aethelgard-api-v2.onrender.com/v1",
            ("Online", "GameAuthExchange"): "/auth/google-id-token/exchange",
            ("Online", "GameAuthRefresh"): "/auth/refresh",
            ("Content", "HighGraphicsPublished"): "false",
            ("Engine", "UnrealSource"): "external-private-only",
        }
        for (section, key), expected in expected_lock.items():
            actual = lock.get(section, key, fallback="")
            if actual != expected:
                failures.append(f"release lock mismatch for [{section}] {key}: expected {expected!r}, got {actual!r}")
        game_auth_base = expected_lock[("Online", "GameAuthBase")]
        source_checks = (
            ("app/build.gradle.kts", expected_lock[("Identity", "AndroidPackage")]),
            ("app/build.gradle.kts", "minSdk = " + expected_lock[("Platform", "MinSdk")]),
            ("app/src/main/res/values/strings.xml", f">{expected_lock[("Identity", "LauncherLabel")]}</string>"),
            ("app/src/main/cpp/forest_game.cpp", expected_lock[("Identity", "JniPrefix")]),
            ("app/src/main/res/values/strings.xml", game_auth_base),
            ("app/src/main/res/values/strings.xml", game_auth_base + expected_lock[("Online", "GameAuthExchange")]),
            ("app/src/main/res/values/strings.xml", game_auth_base + expected_lock[("Online", "GameAuthRefresh")]),
            ("app/src/main/res/values/strings.xml", ">false</string>"),
            ("RELEASE_CONFIGURATION.md", "config/RELEASE_LOCK.ini"),
        )
        for relative, needle in source_checks:
            path = root / relative
            if not path.is_file() or needle not in path.read_text(errors="replace"):
                failures.append(f"release lock source mismatch: {needle!r} in {relative}")
    for relative, needle, label in REQUIRED:
        path = root / relative
        if not path.is_file():
            failures.append(f"missing file for {label}: {relative}")
        elif needle not in path.read_text(errors="replace"):
            failures.append(f"missing symbol for {label}: {needle} in {relative}")
    for relative, needle, label in FORBIDDEN:
        path = root / relative
        if path.is_file() and needle in path.read_text(errors="replace"):
            failures.append(f"forbidden {label}: {needle} in {relative}")
    if failures:
        for failure in failures:
            print(f"FAIL online_only_contract: {failure}")
        raise SystemExit(1)
    print(f"ONLINE_ONLY_CONTRACT_PASS={len(REQUIRED) + len(FORBIDDEN)}")


if __name__ == "__main__":
    main()
