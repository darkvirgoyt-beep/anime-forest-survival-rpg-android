"""Static contract checks for hosted-first Android multiplayer isolation."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, needles: list[str]) -> None:
    target = ROOT / path
    if not target.is_file():
        raise AssertionError(f"missing required file: {path}")
    text = target.read_text(encoding="utf-8")
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError(f"{path} missing contract markers: {missing}")


def main() -> None:
    manifest = (ROOT / "app/src/main/AndroidManifest.xml").read_text(encoding="utf-8")
    require("app/src/main/AndroidManifest.xml", [
        'android:name="android.permission.INTERNET"',
        'android:name="android.permission.ACCESS_NETWORK_STATE"',
    ])
    forbidden_permissions = [
        'android:name="android.permission.ACCESS_WIFI_STATE"',
        'android:name="android.permission.CHANGE_WIFI_STATE"',
        'android:name="android.permission.NEARBY_WIFI_DEVICES"',
        'android:name="android.permission.ACCESS_FINE_LOCATION"',
        'android:name="android.hardware.wifi.direct" android:required="false"',
    ]
    present = [needle for needle in forbidden_permissions if needle in manifest]
    if present:
        raise AssertionError(f"hosted-only manifest unexpectedly contains local transport markers: {present}")

    require("app/src/main/java/com/darkvirgoyt/aethelgrad/LocalMultiplayerManager.kt", [
        'SERVICE_TYPE = "_aethelgard._tcp."',
        "NsdManager",
        "ServerSocket(PORT",
        "WifiP2pManager",
        "MAX_PLAYERS = 4",
    ])
    activity_path = "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt"
    activity = (ROOT / activity_path).read_text(encoding="utf-8")
    require(activity_path, [
        "HOSTED INTERNET CO-OP  •  FREE RENDER SERVICE",
        "CREATE TOWER ROOM",
        "JOIN TOWER ROOM",
        "RECONNECT HOSTED ROOM",
        "networkOnline) return true",
        "accountSession.heartbeatCoOpRoom",
        "accountSession.authoritativeCombat",
        "accountSession.authoritativeInventory",
    ])
    dialog = activity.split("private fun showCoOpDialog()", 1)[1].split("private fun setPlayerName", 1)[0]
    forbidden_dialog_controls = [
        "HOST LAN ROOM",
        "FIND LAN ROOMS",
        "CREATE WI-FI DIRECT GROUP",
        "FIND WI-FI DIRECT DEVICES",
        "LOCAL MULTIPLAYER  •  SAME WI-FI OR WI-FI DIRECT",
    ]
    present = [needle for needle in forbidden_dialog_controls if needle in dialog]
    if present:
        raise AssertionError(f"hosted co-op dialog unexpectedly contains local controls: {present}")
    print("HOSTED_MULTIPLAYER_CONTRACT_PASS=1")


if __name__ == "__main__":
    main()
