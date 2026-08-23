#!/usr/bin/env python3
"""Validate the stable Android/Unreal production foundation contract."""
from __future__ import annotations

from pathlib import Path


CHECKS = (
    ("app/build.gradle.kts", "applicationId =", "Android application identity"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "setContentTierReady", "downloaded quality readiness hook"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/AssetPackCatalog.kt", "requestProductionContent", "Play Asset Delivery request"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "finishPreparation", "content preparation completion path"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "requestProductionContent", "content preparation request path"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/ContentDownloadPlan.kt", "QualityEnvelope", "quality envelope model"),
    ("app/src/main/cpp/forest_game.cpp", "effectiveGraphicsQuality", "native quality gate"),
    ("app/src/main/cpp/forest_game.cpp", "setContentTierReady", "native readiness JNI hook"),
    ("app/src/main/cpp/forest_game.cpp", "drawTerrainChunks", "mobile terrain renderer"),
    ("app/src/main/cpp/forest_game.cpp", "kPlayerMaxHealthHp = 100", "player health contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceProceduralForest.cpp", "GetBiomeAtWorldLocation", "streamed biome lookup"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceProceduralForest.h", "GroundInstances", "procedural world instances"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceInventoryComponent.h", "UForestSliceInventoryComponent", "inventory component"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceResourceNodeComponent.cpp", "TryCollect", "resource gathering"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceMobileHUD.h", "GatherPressed", "mobile gathering HUD input"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceCharacter.h", "ServerTriggerVirtualCollect", "authoritative gathering request"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceWorldSessionSubsystem.h", "MaxCoopMembers = 4", "co-op member limit"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceCreatureCompanionComponent.h", "ServerCaptureCreature", "authoritative companion capture"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceCreatureCompanionComponent.cpp", "ValidateCaptureTarget", "companion capture validation"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceEncounterDirectorComponent.h", "FForestSliceEncounterBudget", "encounter budget contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceEncounterDirectorComponent.cpp", "RebuildBudget", "encounter budget implementation"),
    ("server/src/server.mjs", "room_full", "co-op room capacity response"),
    ("docs/MULTIPLAYER_WORKFLOW.md", "four-player", "co-op workflow documentation"),
    ("docs/HIGH_END_GRAPHICS_AND_CONTENT_TIERS.md", "Download-to-quality contract", "high-end content documentation"),
    ("tools/test_graphics_tier_contract.py", "GRAPHICS_TIER_CONTRACT_PASS", "graphics-tier validator"),
    ("app/src/main/cpp/rpg/companion_system.h", "evaluateCapture", "companion capture rules"),
    ("app/src/main/cpp/rpg/encounter_director.h", "encounterBudgetFor", "deterministic encounter budget"),
    ("app/src/main/cpp/rpg/quality_profile.h", "qualityProfileFor", "runtime quality profiles"),
    ("tests/rpg_systems_test.cpp", "companionAssistDamage", "native RPG domain test"),
)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    failures: list[str] = []
    for relative, needle, label in CHECKS:
        path = root / relative
        if not path.is_file():
            failures.append(f"missing file for {label}: {relative}")
            continue
        if needle not in path.read_text(errors="replace"):
            failures.append(f"missing symbol for {label}: {needle} in {relative}")
    if failures:
        for failure in failures:
            print(f"FAIL production_foundation_contract: {failure}")
        raise SystemExit(1)
    print(f"PRODUCTION_FOUNDATION_CONTRACT_PASS={len(CHECKS)}")


if __name__ == "__main__":
    main()
