"""Validate that the Android release has one production online launch path."""
from __future__ import annotations

from pathlib import Path


REQUIRED = (
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "beginOnlineStartup", "automatic online startup"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "requestGoogleSignIn", "Google production login"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "SIGN IN WITH GOOGLE", "visible Google login control"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "requestProductionContent", "production content request"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/PrivateContentDownloader.kt", "archiveSha256", "private archive verification"),
    ("server/src/server.mjs", "/v1/content/high/manifest", "private high-end manifest route"),
    ("server/src/server.mjs", "/v1/content/high/archive", "private high-end archive route"),
    ("server/.env.example", "PRIVATE_CONTENT_ARCHIVE_PATH", "private archive server configuration"),
    ("app/build.gradle.kts", 'namespace = "com.darvirgoyt.aethelgrad"', "exact Android namespace"),
    ("app/build.gradle.kts", 'applicationId = "com.darvirgoyt.aethelgrad"', "exact Android application ID"),
    ("app/src/main/res/values/strings.xml", "https://aethelservs-g7pzbnwp.manus.space/api/game-auth", "managed game-auth endpoint"),
    ("render.yaml", "name: aethelgard-api-v2", "deployed v2 Render service"),
    ("render.yaml", "dockerContext: ./server", "server Docker build context"),
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
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "PROTOTYPE_MODE", "prototype runtime flag"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "AssetDeliveryManager", "local asset fallback"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "showLocalPreparationFallback", "local preparation fallback"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "offlinePrototypeMode", "offline prototype runtime mode"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "PLAY OFFLINE PROTOTYPE", "offline prototype entry button"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "canUseBundledFreeFallback", "bundled graphics bypass"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "requestGuestSignIn", "automatic guest login"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "GUEST SESSION", "guest login UI"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "OPTIONAL GOOGLE LINK", "optional login copy"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "FREE LOCAL GRAPHICS MODE READY", "bundled graphics mode"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "activateOfflinePrototype", "offline prototype entry"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/AssetPackCatalog.kt", "canUseBundledFreeFallback", "bundled graphics bypass"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/AccountSessionManager.kt", "requestGuestSignIn", "guest auth implementation"),
    ("app/src/main/res/values/strings.xml", "auth_guest_url", "guest auth endpoint"),
    (".github/workflows/android-build.yml", "assemblePrototype", "prototype CI build"),
    (".github/workflows/android-build.yml", "aethelgard-prototype-apk", "prototype CI artifact"),
    ("tools/build_expansion_obb.py", "prototype-v1", "prototype content version"),
    ("app/src/main/assets/asset_manifest.json", "LOW RESOURCES", "low-end player resource tier"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", "SELECT GRAPHICS QUALITY", "player-facing resource tier selector"),
)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    failures: list[str] = []
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
