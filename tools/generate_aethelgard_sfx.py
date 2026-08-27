from __future__ import annotations

import math
import wave
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1] / "assets" / "audio"
SR = 48_000
RNG = np.random.default_rng(42817)


def save(name: str, samples: np.ndarray) -> None:
    samples = np.asarray(samples, dtype=np.float32)
    samples = np.clip(samples, -1.0, 1.0)
    pcm = (samples * 32767).astype(np.int16)
    with wave.open(str(ROOT / name), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(SR)
        out.writeframes(pcm.tobytes())


def env(n: int, attack: float = 0.005, release: float = 0.18) -> np.ndarray:
    e = np.ones(n, dtype=np.float32)
    a = max(1, int(SR * attack))
    r = max(1, int(SR * release))
    e[:a] = np.linspace(0.0, 1.0, a, dtype=np.float32)
    e[-r:] *= np.linspace(1.0, 0.0, r, dtype=np.float32)
    return e


def tone(duration: float, frequency: float, decay: float = 5.0) -> np.ndarray:
    n = int(SR * duration)
    t = np.arange(n, dtype=np.float32) / SR
    return (np.sin(2 * math.pi * frequency * t) * np.exp(-decay * t)).astype(np.float32)


def noise(duration: float, decay: float = 8.0) -> np.ndarray:
    n = int(SR * duration)
    t = np.arange(n, dtype=np.float32) / SR
    return (RNG.normal(0.0, 1.0, n) * np.exp(-decay * t)).astype(np.float32)


def mix(*parts: np.ndarray) -> np.ndarray:
    n = max(len(p) for p in parts)
    out = np.zeros(n, dtype=np.float32)
    for p in parts:
        out[: len(p)] += p
    peak = max(1.0, float(np.max(np.abs(out))))
    return out / peak * 0.86


def footsteps() -> np.ndarray:
    step = mix(noise(0.12, 18.0) * 0.55, tone(0.12, 105, 18) * 0.35)
    gap = np.zeros(int(SR * 0.13), dtype=np.float32)
    return np.concatenate([step * env(len(step), 0.002, 0.07), gap, step * env(len(step), 0.002, 0.07)])


def sprint() -> np.ndarray:
    return mix(noise(0.8, 2.5) * 0.22, tone(0.8, 72, 2.5) * 0.10) * env(int(SR * 0.8), 0.03, 0.22)


def slide() -> np.ndarray:
    n = int(SR * 0.9)
    t = np.arange(n, dtype=np.float32) / SR
    scrape = RNG.normal(0.0, 1.0, n) * (0.15 + 0.4 * (1.0 - t / 0.9))
    whoosh = np.sin(2 * math.pi * (170 - 100 * t) * t) * 0.25
    return mix(scrape, whoosh) * env(n, 0.01, 0.28)


def attack() -> np.ndarray:
    n = int(SR * 0.8)
    t = np.arange(n, dtype=np.float32) / SR
    whoosh = (np.sin(2 * math.pi * (340 - 250 * t) * t) * (1 - t / 0.8))
    impact = tone(0.22, 92, 16) * 0.9
    impact[: int(SR * 0.03)] += noise(0.03, 22) * 0.7
    combined = whoosh * env(n, 0.01, 0.20)
    combined[int(SR * 0.40): int(SR * 0.40) + len(impact)] += impact
    return mix(combined)


def bow() -> np.ndarray:
    n = int(SR * 0.75)
    t = np.arange(n, dtype=np.float32) / SR
    return mix(tone(0.75, 520, 2.6) * 0.38, tone(0.75, 1040, 4.5) * 0.18, noise(0.75, 5.0) * 0.10) * env(n, 0.02, 0.25)


def gather() -> np.ndarray:
    return mix(tone(0.35, 660, 6) * 0.45, tone(0.50, 990, 7) * 0.35, noise(0.18, 14) * 0.2) * env(int(SR * 0.5), 0.005, 0.18)


def craft() -> np.ndarray:
    hit1 = tone(0.16, 420, 12)
    hit2 = tone(0.16, 560, 12)
    out = np.zeros(int(SR * 0.9), dtype=np.float32)
    out[: len(hit1)] += hit1
    start = int(SR * 0.25)
    out[start:start + len(hit2)] += hit2
    return mix(out, tone(0.9, 880, 3) * 0.12)


def animal() -> np.ndarray:
    n = int(SR * 1.2)
    t = np.arange(n, dtype=np.float32) / SR
    chirp = np.sin(2 * math.pi * (500 + 330 * t) * t) * np.exp(-2.8 * t)
    chirp += 0.35 * np.sin(2 * math.pi * (820 + 100 * t) * t) * np.exp(-4.0 * t)
    return chirp.astype(np.float32) * env(n, 0.02, 0.45)


def ui_click() -> np.ndarray:
    return mix(tone(0.12, 880, 22), tone(0.08, 1320, 28) * 0.45) * env(int(SR * 0.12), 0.002, 0.06)


def boss_roar() -> np.ndarray:
    n = int(SR * 2.2)
    t = np.arange(n, dtype=np.float32) / SR
    roar = np.sin(2 * math.pi * (82 - 35 * t) * t) * 0.7
    roar += noise(2.2, 1.5) * 0.15
    return roar.astype(np.float32) * env(n, 0.08, 0.65)


def terrain_steps(base_frequency: float, grit: float) -> np.ndarray:
    step = mix(noise(0.13, 15.0) * grit, tone(0.13, base_frequency, 16) * 0.42)
    gap = np.zeros(int(SR * 0.14), dtype=np.float32)
    return np.concatenate([step * env(len(step), 0.002, 0.075), gap, step * env(len(step), 0.002, 0.075)])


def jump() -> np.ndarray:
    n = int(SR * 0.34)
    t = np.arange(n, dtype=np.float32) / SR
    rising = np.sin(2 * math.pi * (170 + 270 * t) * t) * 0.32
    air = noise(0.34, 7.0) * 0.08
    return mix(rising, air) * env(n, 0.006, 0.16)


def landing() -> np.ndarray:
    impact = mix(noise(0.22, 18.0) * 0.8, tone(0.22, 72, 15) * 0.65)
    tail = tone(0.42, 118, 8.5) * 0.22
    out = mix(impact, tail)
    return out * env(len(out), 0.001, 0.16)


def plains_wind() -> np.ndarray:
    n = int(SR * 8.0)
    t = np.arange(n, dtype=np.float32) / SR
    gust = 0.5 + 0.5 * np.sin(2 * math.pi * 0.11 * t + 0.4 * np.sin(2 * math.pi * 0.037 * t))
    airy = noise(8.0, 0.18) * (0.12 + 0.20 * gust)
    low = np.sin(2 * math.pi * 62 * t) * 0.035
    out = mix(airy, low) * 0.65
    fade = min(int(SR * 0.35), n // 2)
    out[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
    out[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return out


def mountain_echo() -> np.ndarray:
    n = int(SR * 8.0)
    t = np.arange(n, dtype=np.float32) / SR
    out = noise(8.0, 0.22) * 0.07 + np.sin(2 * math.pi * 48 * t) * 0.055
    for start, freq, amp in ((0.6, 92, 0.18), (2.65, 132, 0.13), (5.05, 76, 0.16)):
        i = int(SR * start)
        tail = tone(1.7, freq, 2.7) * amp
        out[i:i + len(tail)] += tail
    fade = min(int(SR * 0.45), n // 2)
    out[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
    out[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return mix(out) * 0.72


ROOT.mkdir(parents=True, exist_ok=True)
save("sfx_footsteps_forest.wav", footsteps())
save("sfx_sprint_loop.wav", sprint())
save("sfx_slide.wav", slide())
save("sfx_attack_sword.wav", attack())
save("sfx_bow_release.wav", bow())
save("sfx_gather_resource.wav", gather())
save("sfx_craft_workbench.wav", craft())
save("sfx_animal_companion_call.wav", animal())
save("sfx_ui_click.wav", ui_click())
save("sfx_boss_roar.wav", boss_roar())
save("sfx_footsteps_plains.wav", terrain_steps(92, 0.34))
save("sfx_footsteps_mountain.wav", terrain_steps(148, 0.52))
save("sfx_jump.wav", jump())
save("sfx_landing.wav", landing())
save("ambience_wind_plains.wav", plains_wind())
save("ambience_mountain_echo.wav", mountain_echo())
print(f"Generated 16 original procedural SFX/ambience assets in {ROOT}")
