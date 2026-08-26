# Cooked Asset-Pack Budget Measurement

`tools/measure_cooked_asset_packs.py` measures the real byte size of an already staged Unreal Android cook. It is a release guard, not a content generator: it never pads packs, estimates bytes, or treats uncooked Unreal source files as downloadable content.

## Inputs and command

The staging input must follow the output layout of `tools/stage_cooked_unreal_assets.py`:

```text
<staging-root>/asset_packs/<assetpack-module>/<original-relative-cooked-path>
```

For a full-content release, run it after staging and before Gradle or OBB packaging:

```bash
python3 tools/measure_cooked_asset_packs.py \
  --staging-root Build/Android/full-content-staging \
  --cook-root Build/Android/Archive/Saved/StagedBuilds/Android/ForestSlice/Content/Paks \
  --budget-manifest assets/full_content_budget.json \
  --mapping-file tools/unreal_pack_mapping.json \
  --require-nonempty \
  --report-json Build/Android/cooked-pack-budget.json
```

Use `assets/asset_budget.json` instead of `assets/full_content_budget.json` only for the smaller Stage 1 plan, and omit `--require-nonempty` only when intentionally validating a partial development staging area. A production full-content build must retain both `--cook-root` and `--require-nonempty`.

## Enforced release contract

| Check | Failure condition |
| --- | --- |
| Manifest integrity | Pack targets do not equal `target_total_mib`, a module is duplicated, or a target exceeds its delivery ceiling. |
| Mapping integrity | The mapping modules and budget modules differ. |
| Staging assignment | A staged `asset_packs/<module>` directory is not declared in the budget. |
| Raw-cook assignment | A runtime file under `--cook-root` has no mapping, maps to more than one pack, or is not copied with identical bytes into its staged pack. |
| Per-pack budget | Staged bytes exceed that pack’s declared MiB budget. |
| Total budget | All staged pack bytes exceed the manifest’s planned total. |
| Strict release completeness | Any declared pack has no real runtime files when `--require-nonempty` is set. |

The JSON report records exact pack byte counts, file counts, targets, totals, and the source paths used for verification. It is suitable for attaching to a trusted build artifact, but must not be stored inside the OBB staging directory because it is build metadata, not game content.

## Automated integration

`tools/build_full_content.sh` runs the strict command immediately after cooked files are staged and before Gradle packaging. The GitHub Android workflow runs the same strict command only when a trusted cooked directory is present; when no real Unreal cook exists, it deliberately skips standalone OBB packaging instead of inventing payloads. The ordinary no-cook workflow still runs the isolated measurement regression test.

> A passing source or CI contract verifies the mapping and budget policy. It does **not** prove a real Unreal cook, Android device installation, memory profile, or downloadable pack exists. Those claims require an actual licensed/original cooked payload and device validation.

## Local regression check

```bash
python3 -m py_compile tools/measure_cooked_asset_packs.py tools/test_measure_cooked_asset_packs.py
python3 tools/test_measure_cooked_asset_packs.py
```

The regression suite covers an under-budget strict pass, an unexpected staging directory, a per-pack overflow, and an unassigned raw cooked file.
