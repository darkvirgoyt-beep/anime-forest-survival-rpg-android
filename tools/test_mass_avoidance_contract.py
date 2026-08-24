from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]

required = [
    ROOT / 'Unreal/Source/ForestSlice/Public/ForestSliceMoonDeerAvoidanceTrait.h',
    ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceMoonDeerAvoidanceTrait.cpp',
    ROOT / 'Unreal/Source/ForestSlice/Public/ForestSliceMoonDeerRoamProcessor.h',
    ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceMoonDeerRoamProcessor.cpp',
    ROOT / 'Unreal/Source/ForestSlice/Public/ForestSliceMassAvoidanceSettings.h',
    ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceMassAvoidanceSettings.cpp',
    ROOT / 'docs/MASS_AVOIDANCE_MOON_DEER.md',
]
for path in required:
    assert path.is_file(), path

build_rules = (ROOT / 'Unreal/Source/ForestSlice/ForestSlice.Build.cs').read_text()
for module in ('MassEntity', 'MassCommon', 'MassMovement', 'MassNavigation', 'MassSpawner'):
    assert f'"{module}"' in build_rules, module

uproject = json.loads((ROOT / 'Unreal/ForestSlice.uproject').read_text())
plugins = {item['Name']: item['Enabled'] for item in uproject['Plugins']}
assert plugins.get('MassAI') is True

trait = (ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceMoonDeerAvoidanceTrait.cpp').read_text()
for marker in ('FMassForceFragment', 'FAgentRadiusFragment', 'FMassAvoidanceColliderFragment', 'FMassCircleCollider', 'FForestSliceMoonDeerTag'):
    assert marker in trait, marker

processor = (ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceMoonDeerRoamProcessor.cpp').read_text()
for marker in ('FMassMoveTargetFragment', 'FForestSliceMoonDeerRoamFragment', 'ExecuteBefore.Add', 'ForEachEntityChunk', 'IsMoonDeerMassAvoidanceEnabled'):
    assert marker in processor, marker

settings = (ROOT / 'Unreal/Source/ForestSlice/Public/ForestSliceMassAvoidanceSettings.h').read_text()
assert 'bEnableMoonDeerMassAvoidance = false' in settings
assert 'bAllowMoonDeerMassAvoidanceOnAndroid = false' in settings

fallback = (ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceWildCreature.cpp').read_text()
assert 'bReplicates = true' in fallback
assert 'if (!HasAuthority())' in fallback
assert 'InitializeFromSpawn' in fallback

print('MASS_AVOIDANCE_CONTRACT_PASS=1')
