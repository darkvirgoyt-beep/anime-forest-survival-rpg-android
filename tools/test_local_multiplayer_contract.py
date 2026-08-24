#!/usr/bin/env python3
"""Static contract checks for Android LAN and Wi-Fi Direct co-op."""
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
    require("app/src/main/AndroidManifest.xml", [
        'android:name="android.permission.INTERNET"',
        'android:name="android.permission.ACCESS_NETWORK_STATE"',
        'android:name="android.permission.ACCESS_WIFI_STATE"',
        'android:name="android.permission.CHANGE_WIFI_STATE"',
        'android:name="android.permission.NEARBY_WIFI_DEVICES"',
        'android:name="android.permission.ACCESS_FINE_LOCATION"',
        'android:name="android.hardware.wifi.direct" android:required="false"',
    ])
    require("app/src/main/java/com/darkvirgoyt/aethelgrand/LocalMultiplayerManager.kt", [
        'SERVICE_TYPE = "_aethelgard._tcp."',
        "NsdManager",
        "ServerSocket(PORT",
        "WifiP2pManager",
        "discoverPeers",
        "createGroup",
        "connectToWifiPeer",
        '"HELLO"',
        '"STATE"',
        '"EVENT"',
        '"SNAPSHOT"',
        '"LEAVE"',
        "groupOwnerAddress.hostAddress",
        "MAX_PLAYERS = 4",
    ])
    require("app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt", [
        "LocalMultiplayerManager.Callbacks",
        "ensureLocalPermissions",
        "HOST LAN ROOM",
        "FIND LAN ROOMS",
        "CREATE WI-FI DIRECT GROUP",
        "FIND WI-FI DIRECT DEVICES",
        "localMultiplayer.sendState",
        'localMultiplayer.sendEvent(action)',
        'localMultiplayer.sendEvent(operation)',
        'localMultiplayer.sendEvent("build_camp")',
        "localMultiplayer.leaveSession()",
        "localMultiplayer.stop()",
        "networkOnline || localSessionActive",
    ])
    print("LOCAL_MULTIPLAYER_CONTRACT_PASS=1")


if __name__ == "__main__":
    main()
