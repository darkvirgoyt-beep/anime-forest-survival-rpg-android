#!/usr/bin/env python3
"""Simulate the Android world-entry warm-up without an Android device."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

MINIMUM_DURATION_MS = 10_000
TICK_MS = 120
TASK_WEIGHTS = {
    "renderer": 28,
    "texture": 20,
    "content": 32,
    "world": 20,
}


@dataclass(frozen=True)
class WarmupSample:
    elapsed_ms: int
    actual_percent: int
    timeline_percent: int
    displayed_percent: int
    all_ready: bool


def progress_at(elapsed_ms: int, ready: set[str]) -> WarmupSample:
    total = sum(TASK_WEIGHTS.values())
    actual = (sum(TASK_WEIGHTS[name] for name in ready) * 100 // total)
    timeline = min(100, max(0, elapsed_ms * 100 // MINIMUM_DURATION_MS))
    displayed = min(actual, timeline)
    return WarmupSample(elapsed_ms, actual, timeline, displayed, ready == set(TASK_WEIGHTS))


def simulate(events: dict[int, set[str]]) -> list[WarmupSample]:
    ready: set[str] = set()
    samples: list[WarmupSample] = []
    tick_times = sorted(set(range(0, MINIMUM_DURATION_MS + TICK_MS, TICK_MS)) | {MINIMUM_DURATION_MS})
    for elapsed in tick_times:
        ready.update(events.get(elapsed, set()))
        samples.append(progress_at(elapsed, ready))
    return samples


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def render(sample: WarmupSample, width: int = 25) -> str:
    filled = round(width * sample.displayed_percent / 100)
    bar = "#" * filled + "." * (width - filled)
    state = "READY" if sample.all_ready else "WAITING"
    return f"{sample.elapsed_ms:>5} ms  [{bar}] {sample.displayed_percent:>3}%  {state}"


def run_simulation() -> None:
    # Events represent callbacks from the renderer, texture upload, selected
    # Low/High pack readiness, and world-state restore/create path.
    early_events = {
        960: {"renderer"},
        2_040: {"texture"},
        4_920: {"content"},
        7_920: {"world"},
    }
    samples = simulate(early_events)
    percentages = [sample.displayed_percent for sample in samples]
    require(percentages == sorted(percentages), "displayed percentage must be monotonic")
    require(max(percentages) <= 100, "displayed percentage must never exceed 100")
    require(next(sample.elapsed_ms for sample in samples if sample.all_ready) >= 7_920, "world task must be the final readiness callback")
    require(all(not sample.all_ready or sample.elapsed_ms >= 7_920 for sample in samples), "readiness cannot precede the final callback")
    require(samples[-1].displayed_percent == 100, "all-ready warm-up must end at 100 percent")
    require(samples[-1].elapsed_ms >= MINIMUM_DURATION_MS, "early-ready world must still wait ten seconds")

    late_events = {
        960: {"renderer"},
        2_040: {"texture"},
        4_920: {"world"},
        10_080: {"content"},
    }
    late_samples = simulate(late_events)
    at_minimum = next(sample for sample in late_samples if sample.elapsed_ms == MINIMUM_DURATION_MS)
    require(at_minimum.displayed_percent < 100, "late content must prevent premature 100 percent")
    require(at_minimum.all_ready is False, "late content must keep the world locked")
    final = next(sample for sample in late_samples if sample.elapsed_ms == 10_080)
    require(final.displayed_percent == 100 and final.all_ready, "late content must reveal only after readiness and ten seconds")

    print("STARTUP_WARMUP_SIMULATION")
    print("elapsed       progress                         state")
    for elapsed in (0, 2_040, 4_920, 7_920, 10_080):
        sample = next(item for item in samples if item.elapsed_ms == elapsed)
        print(render(sample))
    print("SIMULATION_ASSERTIONS_PASS=8")
    print("EARLY_READY_REVEAL_NOT_BEFORE_MS=10000")
    print("LATE_CONTENT_REVEAL_AT_MS=10080")


def check_source_contract(repo: Path) -> None:
    activity = (repo / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt").read_text(encoding="utf-8")
    required = (
        "minimumWorldLoadingDurationMs = 10_000L",
        "worldLoadingProgressTicker",
        "timelinePercent",
        "val percent = minOf(actualReadinessPercent, timelinePercent)",
        "if (allReady && timelinePercent >= 100) revealWorldWhenReady()",
        "worldLoadingProgressHandler.removeCallbacks(worldLoadingProgressTicker)",
    )
    missing = [item for item in required if item not in activity]
    require(not missing, f"startup source contract missing: {missing}")


def parser() -> argparse.ArgumentParser:
    command = argparse.ArgumentParser(description=__doc__)
    command.add_argument("--repo", type=Path, default=Path("."), help="repository root")
    return command


if __name__ == "__main__":
    args = parser().parse_args()
    check_source_contract(args.repo.resolve())
    run_simulation()
