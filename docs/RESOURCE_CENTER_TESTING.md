# Resource-center UI and progress testing

Use `scripts/test_resource_center.py` to test the UI-facing state model without downloading gigabytes or requiring Google Play services. The script feeds synthetic Play Asset Delivery events into a small aggregate model and checks the same behaviors the Android screen depends on.

Run it from a repository root with:

```bash
python3 /home/ubuntu/skills/aethelgard-game-delivery/scripts/test_resource_center.py \\
  --repo . --unreal-project Unreal/ForestSlice.uproject
```

The repository copy is:

```bash
./tools/test_resource_center.py \\
  --repo . --unreal-project Unreal/ForestSlice.uproject
```

The test covers five deterministic cases: first progress before all pack totals are known, monotonic aggregate progress as additional packs are discovered, completion only after every pack is ready, failure followed by a direct-APK fallback, and rejection of impossible byte counts. The source contract check also verifies the title, progress-bar update, retry control, 6.6 GB manifest target, production Play Asset Delivery request, and wide camera clamp. The `--unreal-project` option also parses `Unreal/ForestSlice.uproject` and requires its `FileVersion` and `Modules` fields, so this check runs against the same Unreal build pipeline validation job.

Keep this test separate from device tests. Use Espresso/UI Automator or a physical Play internal-test install for actual rendering, tapping, Wi-Fi confirmation, cancellation, Google Play services behavior, and real pack mounting. Do not put a 6.6 GB fixture in CI.
