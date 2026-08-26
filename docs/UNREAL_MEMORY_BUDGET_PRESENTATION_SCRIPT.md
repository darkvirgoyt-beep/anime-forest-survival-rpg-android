# Aethelgard Unreal Memory Budget & CI Guardrails

**Presenter script**  
**Audience:** game-development stakeholders and technical contributors  
**Suggested duration:** 6–8 minutes

## Cover

**Aethelgard: Unreal Memory Budget & CI Guardrails**

**How we prevent oversized Android content before it reaches a build**

---

## Slide 1 — The Guardrails Protect Two Different Things

### On-slide content

- **Storage budget:** controls the size of each downloadable pack.
- **Runtime memory budget:** controls the memory Unreal may use while streaming assets.
- **Truth boundary:** plans are not shipped payloads.

### Speaker script

“The important idea is that storage size and runtime memory are not the same thing. An asset pack can be hundreds of megabytes on disk without being loaded entirely into RAM. We therefore enforce two guardrails. The first is a storage ceiling for every Android pack. The second is a runtime streaming ceiling inside Unreal. Finally, we keep the content status truthful: planned future packs are not represented as already shipped game content.”

---

## Slide 2 — Unreal Runtime Memory Is Now Bounded

### On-slide content

| Unreal setting | Guardrail |
|---|---:|
| Texture streaming pool | **256 MiB maximum** |
| Texture streaming | **Enabled** |
| Pool behavior | **Limited to available VRAM** |
| Frame pacing target | **30–60 FPS** |

### Speaker script

“On the Unreal side, we reduced the texture-streaming pool from 1,000 MiB to 256 MiB. That is the current source-level cap for streamed texture memory. Texture streaming remains enabled, and Unreal is told to limit the pool to the available video memory. This does not promise that every Android phone will run the same settings; it prevents the project configuration from silently reserving a one-gigabyte streaming pool. The project also retains bounded frame pacing between 30 and 60 frames per second.” [1]

---

## Slide 3 — Current Launch Content Has a Separate Budget

### On-slide content

| Measure | Verified value |
|---|---:|
| Stage 1 planned total | **1,024 MiB** |
| Current authored payload | **7.796 MiB** |
| Current pack overages | **0** |
| Deferred-pack packaged bytes | **0** |

### Speaker script

“The current launch-slice plan has a total storage budget of 1,024 MiB. The actual authored payload currently present in the repository is only 7.796 MiB. This is not a failure or a claim that the game is already a one-gigabyte release. It means the project contains a small, real launch-slice harness and metadata while the planned production content has not yet been cooked in Unreal. Every current pack is below its declared Stage 1 budget, and every deferred pack remains at zero packaged bytes.” [2]

---

## Slide 4 — Future Packs Cannot Exceed Their Ceiling

### On-slide content

| Delivery type | Maximum planned size |
|---|---:|
| Install-time core | **1,024 MiB** |
| Dynamic, fast-follow, or on-demand pack | **512 MiB** |
| Largest current future plans | **500 MiB** |

### Speaker script

“For the full production plan, the install-time core is allowed up to 1,024 MiB. Every dynamic pack—whether it is fast-follow or on-demand—is limited to 512 MiB. The largest planned future packs are the character and HD-texture packs at 500 MiB, so they remain inside that ceiling. The planned full-content total is 7,108 MiB, with 6,750 MiB after the install-time core. Those are planning limits, not an assertion that those bytes have been shipped.” [3]

---

## Slide 5 — CI Stops Invalid Budget Changes Early

### On-slide content

- Rejects a dynamic pack above **512 MiB**.
- Rejects an install-time pack above **1,024 MiB**.
- Rejects a mismatch between pack totals and manifest totals.
- Rejects packaged bytes above a current Stage 1 limit.
- Rejects any deferred pack containing packaged bytes.

### Speaker script

“The new `test_unreal_memory_budget.py` check makes these rules executable. It fails a build before release packaging if a contributor increases any dynamic pack beyond 512 MiB, increases the install-time pack beyond one gigabyte, creates an inconsistent manifest total, or puts real files into a pack declared as deferred. This is important because it blocks a size mistake at pull-request or push time instead of discovering it after a long Android build.” [4]

---

## Slide 6 — Packaging Keeps Content Modular

### On-slide content

- Compressed Unreal chunks are required.
- Content is packaged outside the APK base.
- Android App Bundle and Play Asset Delivery remain the delivery boundary.
- Unreal target remains Android, using UE 5.6 project settings.

### Speaker script

“The project is configured for compressed Unreal chunks and keeps game data outside the base APK. This is the correct direction for large Android content because it avoids forcing optional or future content into the initial download. The project descriptor targets Android and the production foundation is associated with Unreal Engine 5.6. These settings provide structure, but they do not replace an actual Unreal Android cook.” [1] [5]

---

## Slide 7 — What Is Verified Versus What Is Still Pending

### On-slide content

| Verified now | Still pending on real hardware |
|---|---|
| Source settings and budget contracts | UE 5.6 cook and Android package |
| CI pack ceilings and byte checks | Texture-memory capture and thermal profile |
| Current manifests and publication gates | Device-specific streaming and frame pacing |
| Android harness build checks | Original/licensed cooked high-end content |

### Speaker script

“We should be precise about the evidence. The CI checks verify source settings, manifest consistency, actual repository bytes, and policy ceilings. They do not prove runtime memory use on a phone, because this environment has not cooked or run the Unreal project. The next production milestone is to open the project on a UE 5.6-capable desktop, cook an Android target, and measure texture memory, frame time, streaming stalls, thermals, and storage on real target devices.” [4] [5]

---

## Slide 8 — Closing: Safe Expansion Requires Measured Content

### On-slide content

**Plan by budget. Publish by measurement. Profile on devices.**

### Speaker script

“The rule for future expansion is simple: plan by budget, publish by measurement, and profile on devices. No future biome, cinematic, shader library, or HD-texture pack becomes published merely because it has a planned size. It must contain original or properly licensed cooked content, report its exact bytes, pass the CI ceiling, and survive Unreal Android device profiling.”

---

## Presenter Q&A Notes

| Question | Suggested answer |
|---|---|
| “Does a 500 MiB pack use 500 MiB of RAM?” | “No. It is a storage ceiling. Runtime memory is separately bounded by Unreal streaming settings and must be measured on a device after cooking.” |
| “Why is the current payload only 7.796 MiB?” | “The repository currently contains the Android harness, contracts, metadata, and small authored launch-slice assets—not an Unreal-cooked production world.” |
| “Can we make the game 10–20 GB now?” | “No. Adding empty padding would be misleading and wasteful. Real original/licensed cooked content must be created, measured, budgeted, and profiled first.” |
| “What blocks a future oversized pack?” | “The CI budget test rejects it before release packaging if it crosses the delivery ceiling or disagrees with the manifest.” |

## References

[1] [Unreal Android rendering, streaming, and packaging settings](../Unreal/Config/DefaultEngine.ini)

[2] [Current Stage 1 asset-pack budget](../assets/asset_budget.json)

[3] [Full future-content asset-pack budget](../assets/full_content_budget.json)

[4] [Automated Unreal memory-budget guardrail](../tools/test_unreal_memory_budget.py)

[5] [Unreal Android project descriptor](../Unreal/ForestSlice.uproject)
