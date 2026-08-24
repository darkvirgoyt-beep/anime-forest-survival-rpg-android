# Original creature spawning

`AForestSliceBiomeSpawnDirector` is the original-world entry point for animals and hostile mobs. Place it in an authored Unreal level, assign original Blueprint subclasses of `AForestSliceWildCreature`, assign their fictional biome profiles, and provide player positions from the authoritative game mode/session.

| Fictional profile | Role | Biome intent | Spawn disposition |
|---|---|---|---|
| Emberling | Small companion candidate | Verdant Veil and Sunken Canopy | Passive, capturable only after the server-side capture rule accepts it |
| Rootback Grazer | Herd animal | Verdant Veil | Skittish |
| Frosthorn Wanderer | Alpine animal | Frostwake Crown | Skittish |
| Duskmaw Prowler | Hostile night hunter | Verdant Veil and Emberfall Hollow | Hostile |
| Cinder Warden | Elite hostile guardian | Emberfall Hollow ruins | Hostile and encounter-gated |

The table names are original design placeholders. Before shipping, each needs an original/licensed mesh, rig, animation set, AI behavior tree, sounds, materials, LODs, collision, navigation settings, and Android budget entry in `ASSETS.md`.

## Authority rules

The director runs only on `HasAuthority()`. It uses the existing encounter budget before spawning, uses a deterministic game seed only for candidate order, raycasts against authored world geometry, keeps spawns outside the minimum player distance, caps groups at the remaining active budget, forces elite/boss profiles to one member, replicates creature state, maps every profile to the shared wildlife/predator/companion/mount/boss role taxonomy, and lets Blueprint subclasses begin their own original idle/flee/combat presentation from `OnSpawnStateReady`.

The client must not create creatures, choose spawn locations, approve capture, grant drops, or decide hostile damage. Co-op spawning, combat, capture, rewards, and persistent creature state require the server/session authority paths described elsewhere in this project.

## Level setup

1. Create original Blueprint subclasses such as `BP_Emberling`, `BP_RootbackGrazer`, and `BP_DuskmawProwler` from `AForestSliceWildCreature`.
2. Add skeletal mesh, animation, behavior tree, audio, navigation, and collision only from original or licensed content.
3. Place `AForestSliceBiomeSpawnDirector` in `L_VerdantVeil` and assign the Blueprint class to each allowed `FForestSliceBiomeSpawnProfile`.
4. Have the authoritative game mode call `SetAuthoritativePlayerAnchors` and `RefreshSpawnBudget` after player/session state changes.
5. Test four players, empty areas, combat despawns, travel, reconnects, and Android performance before increasing the active encounter budget.
