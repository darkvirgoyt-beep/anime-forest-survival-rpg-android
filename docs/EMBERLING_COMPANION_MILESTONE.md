# Emberling Companion Milestone

## Purpose

This milestone adds an original companion loop to the Android vertical slice: **Emberling**, a luminous foxlike forest spirit that appears near the Heartfire after the ember kit is crafted. It is not copied from another game’s creatures, systems, or assets.

The player approaches Emberling and spends one fiber per trust interaction. After three successful interactions, Emberling becomes bonded. The same mobile action changes from bonding to a clear **follow/stay** command. The companion visibly trails the player in third-person rendering, accelerates slightly during storms to avoid falling behind, and persists its trust, bond, and command state in cloud saves.

| Player state | Interaction result | Presentation |
|---|---|---|
| Before the ember kit | Bond blocked | Emberling remains a cautious wild encounter. |
| Ember kit, 0–2 trust | One fiber spent | A pulse communicates trust growth. |
| Third trust | Bond complete | Emberling enters follow mode and awards a small XP beat. |
| Bonded | Command changes | Follow/stay state is shown in the live HUD. |

## Production Boundary

The Android GLES milestone is a deterministic presentation and input contract, not a claim of final creature AI, animation, mounts, breeding, combat assistance, or multiplayer authority. In the real online production path, the backend or authoritative co-op host must validate distance, inventory spend, ownership, and command updates before committing the same state. The cloud-state schema remains backward compatible with existing v1–v3 saves.
