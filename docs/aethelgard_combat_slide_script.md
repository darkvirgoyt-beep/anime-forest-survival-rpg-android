# Server authority protects combat and inventory

## Purpose

The audience must understand that while the client plays combat animations immediately for responsiveness, the shared boss health and rewards are exclusively calculated and protected by the server transaction.

## Note on slide numbering

You requested this script for "slide 5". In the current AETHELGRAD presentation deck, slide 5 is actually "One camera contract supports two play styles" (page 5, `camera_modes`). The authoritative combat slide is slide 9 (page 9, `authority`). This script is written for the authoritative combat slide to match your architectural topic request.

## Spoken script

We want the game to feel immediately responsive, but we cannot trust the client to tell us how much damage a boss took. 

When a player attacks, the Android client plays the weapon animation and sound effect instantly. However, it does not subtract health locally. Instead, it sends an authoritative request to the co-op room service. 

Inside a database transaction, the server verifies that the player is actually in the room, that they are close enough to the Forest Warden, and that they aren't spamming attacks faster than the cooldown allows. Only then does the server calculate the damage, subtract it from the boss's health, and return the new authoritative health value. 

If the network drops and the client retries the request, the server sees the same request ID, recognizes it as a retry, and returns the previous result instead of double-charging the damage. This means players get immediate visual feedback on their own screen, but the shared world state remains perfectly secure and synchronized for everyone in the room.

## Talking points

| Point | Evidence in implementation | Audience takeaway |
| --- | --- | --- |
| **Local responsiveness** | The Kotlin HUD triggers `NativeGameBridge.attack()` before the network request completes. | The player never feels input lag when swinging a weapon. |
| **Server-locked damage** | The `POST /v1/coop/rooms/:code/combat` endpoint calculates damage internally; the client only sends the action type. | Hacked clients cannot send "1,000,000 damage" requests. |
| **Idempotent retries** | The `coop_action_receipts` table stores the `requestId` inside the transaction. | Network stutters won't cause accidental double-damage or duplicate loot. |
| **Authoritative sync** | The client updates its local `gEnemyHealth` native state only when the HTTP response arrives. | All players in the room see the exact same boss health, guaranteed by the server. |

## Transition

That transactional boundary keeps our prototype secure, but it also makes the Android HUD an incredibly powerful testing tool—which brings us to how we expose this state on the device.
