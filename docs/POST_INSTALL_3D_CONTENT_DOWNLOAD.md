# Optional One-GiB Authored Content Download

Aethelgard starts from a **bundled playable world**. The player may enter the world while sign-in, cloud sync, co-op, and high-detail content reconnect in the background. The optional private HTTPS archive enhances presentation and later sectors; it never blocks the first playable world entry.

| Stage | What is present | What happens next |
|---|---|---|
| Bundled install | Renderer, playable launch world, controls, HUD, loading scene, core audio and fallback materials | The player can enter immediately |
| Optional authored archive | One 1,024 MiB archive for original cooked sectors, characters, textures, foliage, audio, cinematics, VFX, shader libraries and animation | Download can resume in the background over HTTPS |
| Verification | Exact byte count and SHA-256 match the private manifest for the installed APK version | The archive mounts atomically after verification |
| Failure or pause | Missing service, wrong version, network/storage issue, or checksum mismatch | The bundled world stays playable and the optional download can be retried later |

The high-end plan is a **content target, not padding**. Every binary shipped into the archive must have its source file and a license or ownership receipt recorded in `assets/runtime_content/receipts.json`. `tools/validate_runtime_content_package.py --require-authored-payload` refuses a release package until the licensed authored payload reaches the 1,024 MiB target.

The private archive uses cooked Unreal `.pak`/`.ucas`/`.utoc` output, platform-qualified textures, compiled shader libraries, original meshes, rigs, animation, audio, and signed release metadata. Reference images and screenshots are not runtime content.

The camera foundation supports a full 360-degree horizontal yaw loop and nearly 180 degrees of vertical travel, exposed to the player as a 540-degree-class third-person orbit. This is camera motion support; it does not by itself create final authored terrain, skeletal characters, PBR materials, foliage LODs, or cinematic lighting.
