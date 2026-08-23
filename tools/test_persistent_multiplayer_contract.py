#!/usr/bin/env python3
"""Static contract checks for persistent creator-owned co-op worlds."""
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
    require("server/sql/006_persistent_coop_worlds.sql", [
        "ALTER TABLE coop_rooms",
        "world_name",
        "is_active BOOLEAN NOT NULL DEFAULT true",
        "item_state JSONB",
        "progression_state JSONB",
        "member_revision BIGINT",
        "CREATE TABLE IF NOT EXISTS coop_world_saves",
        "CREATE TABLE IF NOT EXISTS coop_buildings",
    ])
    require("server/src/server.mjs", [
        'app.get("/v1/coop/rooms/:code/player-save"',
        'app.put("/v1/coop/rooms/:code/player-save"',
        'app.get("/v1/coop/rooms/:code/save"',
        'app.put("/v1/coop/rooms/:code/save"',
        'app.post("/v1/coop/rooms/:code/buildings"',
        'world_owner_required',
        'UPDATE coop_members SET is_active = FALSE',
        'is_active = TRUE, last_seen_at = now()',
    ])
    require("app/src/main/java/com/darvirgoyt/aethelgrad/AccountSessionManager.kt", [
        "data class CoOpPlayerSave",
        "data class CoOpWorldSave",
        "lastPersistentCoOpRoomCode",
        "reconnectLastPersistentCoOpWorld",
        "loadCoOpPlayerSave",
        "saveCoOpPlayerState",
        "loadCoOpWorldSave",
        "rememberCoOpRoom",
    ])
    require("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", [
        "savePersistentCoOpState",
        "SAVED ITEMS + PROGRESSION RESTORED",
        "NativeGameBridge.loadCloudState(playerSave.progressionStateJson)",
        "hudHandler.postDelayed(coOpSaveUpdater, 30_000L)",
    ])
    require("server/README.md", [
        "sql/006_persistent_coop_worlds.sql",
        "creator-owned and persistent",
        "/v1/coop/rooms/:code/player-save",
        "/v1/coop/rooms/:code/save",
    ])
    print("PERSISTENT_MULTIPLAYER_CONTRACT_PASS=1")


if __name__ == "__main__":
    main()
