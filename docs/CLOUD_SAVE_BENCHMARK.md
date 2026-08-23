# Cloud Save Parse and Migration Benchmark

## Scope

This benchmark measures the native cost of parsing a cloud snapshot and canonicalizing it to schema 5. It covers the current schema-5 format and legacy schemas 1–4, including their default discovery-sector behavior and source-schema metadata used by native restoration.

The workload uses the exact compact JSON shapes accepted by `app/src/main/cpp/rpg/cloud_state.h`. Each case performs 10,000 warm-up parses followed by 200 timed samples of 1,000 parses each. The benchmark runs single-threaded and is pinned to one CPU core in CI when `taskset` is available.

> The CI result is a **single-core constrained proxy**, not a physical Android-device measurement. It is useful for comparing schema paths and detecting regressions, but it does not model a particular ARM CPU, Android scheduler, thermal state, storage, or device build.

## CI result

The benchmark ran in [Android CI run #32662327417](https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/actions/runs/32662327417) on commit [`bc68373`](https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/commit/bc68373). Native tests, the Android build, asset validation, PAD local-test packaging, APK/AAB packaging, alignment verification, and OBB generation all passed.

| Snapshot path | Source schema | Mean parse+migration | p50 | p95 | p99 | Throughput |
|---|---:|---:|---:|---:|---:|---:|
| Current full snapshot | 5 | 1.9887 µs | 1.9737 µs | 2.1964 µs | 2.2590 µs | 502,848 ops/s |
| Legacy snapshot | 4 | 4.7577 µs | 4.6423 µs | 5.2735 µs | 5.4259 µs | 210,184 ops/s |
| Legacy snapshot | 3 | 5.3391 µs | 5.2518 µs | 5.9551 µs | 6.1114 µs | 187,297 ops/s |
| Legacy snapshot | 2 | 3.4484 µs | 3.3744 µs | 3.8906 µs | 3.9850 µs | 289,986 ops/s |
| Legacy snapshot | 1 | 3.9639 µs | 3.9184 µs | 4.4368 µs | 4.7083 µs | 252,278 ops/s |

Relative to the current schema-5 path, the measured mean overhead was **73.40% for schema 2**, **99.32% for schema 1**, **139.24% for schema 4**, and **168.47% for schema 3**. The percentages look large because the current parse is only about two microseconds on the constrained host; the absolute maximum legacy mean was still just **5.3391 microseconds per save**.

## Gameplay impact

Even under this constrained proxy, parsing 100 saves would consume approximately **0.199 ms** for schema 5, **0.345 ms** for schema 2, **0.396 ms** for schema 1, **0.476 ms** for schema 4, or **0.534 ms** for schema 3. A normal login or startup loads one active world snapshot, so migration is far below a frame budget and should not affect exploration, rendering, input, or asset downloads.

The migration is therefore not a performance bottleneck. The more important production safeguards are that the parser remains bounded, malformed data remains fail-closed, legacy experience conversion is applied only to source schemas 1–2, and a successful save is reserialized in schema 5 for the next server write.

## Reproducing the benchmark

On a machine with a C++17 compiler:

```bash
g++ -O2 -std=c++17 -Wall -Wextra -Wpedantic \
  -Iapp/src/main/cpp \
  tests/cloud_state_benchmark.cpp \
  -o cloud_state_benchmark

taskset -c 0 ./cloud_state_benchmark cloud-state-benchmark.json
```

The CI workflow stores the generated JSON and CSV files in the `cloud-save-benchmark` artifact. The checked-in CSV copy is [`docs/cloud_state_benchmark_ci.csv`](cloud_state_benchmark_ci.csv).

## Optimization result

The parser was optimized in commits [`4b74001`](https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/commit/4b74001) and [`cf84d3b`](https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/commit/cf84d3b). It now dispatches directly from the fixed `schemaVersion` prefix instead of trying the schema-5, schema-4, schema-3, and schema-2 formats in sequence. The final CI measurement is in [run #32663252849](https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/actions/runs/32663252849).

| Snapshot path | Before mean | Optimized mean | Mean change | Optimized throughput |
|---|---:|---:|---:|---:|
| Current full snapshot | 1.9887 µs | 3.0632 µs | +54.03% in this run | 326,455 ops/s |
| Schema 4 legacy | 4.7577 µs | 1.8324 µs | **61.49% faster** | 545,719 ops/s |
| Schema 3 legacy | 5.3391 µs | 1.6220 µs | **69.62% faster** | 616,525 ops/s |
| Schema 2 legacy | 3.4484 µs | 1.2825 µs | **62.81% faster** | 779,703 ops/s |
| Schema 1 legacy | 3.9639 µs | 0.9396 µs | **76.30% faster** | 1,064,300 ops/s |

The schema-5 comparison varies between CI runs because these are separate short-lived hosted runners and the operation is only a few microseconds. The local repeated run stayed near the earlier schema-5 baseline, so the apparent schema-5 increase should not be treated as a device regression without an on-device paired benchmark. The stable signal is the legacy-path improvement: every legacy schema now avoids several failed formatted scans.

The optimized parser passed `cloud_state_test`, `exploration_progression_test`, the full native test job, server tests, Android compilation, PAD local-test packaging, APK/AAB packaging, 16 KB alignment verification, and OBB generation. The optimized CI CSV is [`docs/cloud_state_benchmark_optimized_ci.csv`](cloud_state_benchmark_optimized_ci.csv).
