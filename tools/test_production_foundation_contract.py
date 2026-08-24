#!/usr/bin/env python3
"""Validate the stable Android/Unreal production foundation contract."""
from __future__ import annotations

from pathlib import Path


CHECKS = (
    ("app/build.gradle.kts", "applicationId =", "Android application identity"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "setContentTierReady", "downloaded quality readiness hook"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AssetPackCatalog.kt", "requestProductionContent", "Play Asset Delivery request"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "finishPreparation", "content preparation completion path"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "requestProductionContent", "content preparation request path"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "GRAPHICS DOWNLOAD FAILED  •  GAME LOCKED", "safe fail-closed graphics state"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "CONTENT READY", "downloaded content ready state"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt", "WARMING HIGH-END GRAPHICS", "high-end startup loading"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AssetPackCatalog.kt", "PrivateContentDownloader", "private high-end content downloader"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/ContentDownloadPlan.kt", "QualityEnvelope", "quality envelope model"),
    ("app/src/main/cpp/forest_game.cpp", "effectiveGraphicsQuality", "native quality gate"),
    ("app/src/main/cpp/forest_game.cpp", "setContentTierReady", "native readiness JNI hook"),
    ("app/src/main/cpp/forest_game.cpp", "drawTerrainChunks", "mobile terrain renderer"),
    ("app/src/main/cpp/forest_game.cpp", "kPlayerMaxHealthHp = 100", "player health contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceProceduralForest.cpp", "GetBiomeAtWorldLocation", "streamed biome lookup"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceProceduralForest.h", "GroundInstances", "procedural world instances"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceInventoryComponent.h", "UForestSliceInventoryComponent", "inventory component"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceResourceNodeComponent.cpp", "TryCollect", "resource gathering"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceMobileHUD.h", "GatherPressed", "mobile gathering HUD input"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceMobileHUD.h", "UForestSliceVirtualJoystick", "runtime virtual joystick"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceMobileHUD.cpp", "BuildRuntimeControlSurface", "runtime mobile control surface"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceMobileHUD.cpp", "ToggleSettingsPressed", "settings button action"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceMobileHUD.cpp", "GraphicsQualityChanged", "live graphics quality control"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceGraphicsSettingsSubsystem.h", "UForestSliceGraphicsSettingsSubsystem", "graphics settings subsystem"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceGraphicsSettingsSubsystem.cpp", "ScalabilityQuality", "Unreal scalability application"),
    ("Unreal/Source/ForestSlice/Public/ForestSlicePlayerController.h", "MobileHUDClass", "mobile HUD controller binding"),
    ("Unreal/Source/ForestSlice/Private/ForestSlicePlayerController.cpp", "CreateWidget<UForestSliceMobileHUD>", "runtime HUD spawn"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceGameMode.cpp", "PlayerControllerClass = AForestSlicePlayerController::StaticClass()", "production controller bootstrap"),
    ("Unreal/Source/ForestSlice/ForestSlice.Build.cs", '"UMG"', "public UUserWidget module dependency"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceGameMode.h", "AForestSliceGameMode", "Unreal production game mode"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceGameMode.cpp", "DefaultPawnClass = AForestSliceCharacter::StaticClass()", "production character bootstrap"),
    ("Unreal/Config/DefaultGame.ini", "GlobalDefaultGameMode=/Script/ForestSlice.ForestSliceGameMode", "default game mode configuration"),
    ("Unreal/Config/DefaultGame.ini", "MaxPlayers=4", "four-player Unreal session cap"),
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
    ("app/src/main/cpp/rpg/animation_state.h", "deriveAnimationIntent", "deterministic animation intent module"),
    ("tests/animation_state_test.cpp", "AnimationUpperBodyState::HeavyAttack", "animation intent unit test"),
    ("Unreal/Source/ForestSlice/Public/ForestSlicePresentationComponent.h", "FForestSliceAnimationIntent", "Unreal animation intent contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSlicePresentationComponent.cpp", "UpdateAnimationIntent", "Unreal animation intent implementation"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceFirstPlayableSubsystem.h", "EForestSliceFirstPlayablePhase", "Unreal first-playable phase contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceFirstPlayableSubsystem.cpp", "ResolveOwnedWorld", "authorized world recovery hand-off"),
    ("Unreal/FIRST_PLAYABLE_MILESTONE.md", "ForestArrival", "first-playable production milestone"),
    ("Unreal/ASSETS.md", "original or appropriately licensed", "Unreal asset governance"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceTerrainDefinition.h", "bDerivedFromGoogleEarthContent", "terrain source licensing boundary"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceTerrainDefinition.cpp", "ValidateForLandscapeImport", "terrain import validation"),
    ("Unreal/TERRAIN_IMPORT_WORKFLOW.md", "planning boundary", "legal 3D terrain import workflow"),
    ("Unreal/ORIGINAL_WORLD_REFERENCE_POLICY.md", "Private reference intake", "KML reference exclusion policy"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceOriginalWorldGenerator.h", "SampleOriginalElevationMeters", "original terrain generator contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceOriginalWorldGenerator.cpp", "FractalNoise", "seed-driven original terrain implementation"),
    ("Unreal/ORIGINAL_WORLD_ATLAS.md", "Verdant Veil", "original first-playable biome atlas"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceWildCreature.h", "InitializeFromSpawn", "replicated original wild creature contract"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceBiomeSpawnDirector.h", "SetAuthoritativePlayerAnchors", "server-authoritative biome spawn contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceBiomeSpawnDirector.cpp", "LineTraceSingleByChannel", "safe creature spawn placement"),
    ("Unreal/CREATURE_SPAWNING.md", "Duskmaw Prowler", "original hostile and animal spawning guide"),
    ("Unreal/BLUEPRINT_CREATURE_IMPLEMENTATION_GUIDE.md", "BP_RootbackGrazer", "Rootback Grazer Blueprint setup guide"),
    ("Unreal/BLUEPRINT_CREATURE_IMPLEMENTATION_GUIDE.md", "RequestAuthoritativeProwlerAttack", "Duskmaw Prowler authority boundary"),
    ("Unreal/BLUEPRINT_CREATURE_IMPLEMENTATION_GUIDE.md", "Estimated sprint jump travel", "creature movement tuning guide"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceWildCreature.cpp", "EAutoPossessAI::PlacedInWorldOrSpawned", "wild creature AI possession readiness"),
    ("Unreal/Source/ForestSlice/Public/ForestSliceWildCreature.h", "GetExpectedJumpDistanceMeters", "wild creature jump-distance contract"),
    ("Unreal/Source/ForestSlice/Private/ForestSliceWildCreature.cpp", "SetWildCreatureLocomotionMode", "server-authoritative wild creature locomotion"),
    (".github/workflows/android-build.yml", "tests/animation_state_test.cpp", "animation test CI coverage"),
    ("server/sql/007_companion_camp_authority.sql", "CREATE TABLE IF NOT EXISTS coop_companions", "companion authority migration"),
    ("server/sql/007_companion_camp_authority.sql", "CREATE TABLE IF NOT EXISTS coop_camps", "camp authority migration"),
    ("server/src/server.mjs", "/v1/coop/rooms/:code/companions/capture", "server companion capture route"),
    ("server/src/server.mjs", "/v1/coop/rooms/:code/camps", "server field camp route"),
    ("server/src/companion-camp-authority.mjs", "validateCampPlacement", "pure camp validation module"),
    ("app/src/main/cpp/rpg/cloud_state.h", "schemaVersion", "schema 5 cloud persistence"),
    ("app/src/main/cpp/forest_game.cpp", "applyAuthoritativeCompanion", "native companion reconciliation JNI"),
    ("app/src/main/cpp/forest_game.cpp", "applyAuthoritativeCamp", "native camp reconciliation JNI"),
    ("app/src/main/java/com/darkvirgoyt/aethelgrad/AccountSessionManager.kt", "fetchCompanionCampState", "typed Android authority wrapper"),
    ("docs/COMPANION_CAMP_AUTHORITY_DESIGN.md", "Deployment order", "authority deployment documentation"),
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
