from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
health_h = (ROOT / 'Unreal/Source/ForestSlice/Public/ForestSliceHealthComponent.h').read_text()
health_cpp = (ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceHealthComponent.cpp').read_text()
combat_cpp = (ROOT / 'Unreal/Source/ForestSlice/Private/ForestSliceCombatComponent.cpp').read_text()
native_h = (ROOT / 'app/src/main/cpp/controller/third_person_controller.h').read_text()
native_cpp = (ROOT / 'app/src/main/cpp/controller/third_person_controller.cpp').read_text()
native_test = (ROOT / 'tests/combat_controller_test.cpp').read_text()

for marker in ('BlueprintAuthorityOnly', 'HealthRegenPerSecond', 'HealthRegenDelaySeconds', 'DamageImmunitySeconds', 'MaxKnockbackImpulse', 'HealthChanged'):
    assert marker in health_h, marker
for marker in ('NormalizeState', 'HealthRegenDelayRemaining', 'DamageImmunityRemaining', 'GetClampedToMaxSize', 'ForceNetUpdate'):
    assert marker in health_cpp, marker
for marker in ('ForestSliceWeaponComponent.h', 'GetEquippedDefinition', 'ResolvedDamage', 'Health->GetState().bDowned', 'TakeDamage'):
    assert marker in combat_cpp, marker
for marker in ('healthRegenPerSecond', 'healthRegenDelaySeconds', 'healthRegenDelayRemaining'):
    assert marker in native_h, marker
for marker in ('healthRegenDelayRemaining', 'healthRegenPerSecond', 'takeDamage'):
    assert marker in native_cpp, marker
for marker in ('damagedHealth', 'controller.health > damagedHealth', 'controller.health <= controller.maxHealth'):
    assert marker in native_test, marker

print('COMBAT_HEALTH_CONTRACT_PASS=1')
