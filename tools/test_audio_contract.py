#!/usr/bin/env python3
"""Static and binary contract checks for the authored Android audio layer."""
from __future__ import annotations

import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME_AUDIO = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/GameAudio.kt"
ACTIVITY = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt"
NATIVE = ROOT / "app/src/main/cpp/forest_game.cpp"
GENERATOR = ROOT / "tools/generate_aethelgard_sfx.py"
AUDIO_ROOT = ROOT / "assets/audio"
RAW_ROOT = ROOT / "app/src/main/res/raw"
PACK_ROOT = ROOT / "assetpack_audio_hd/src/main/assets/launch_slice/audio"


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def main() -> None:
    game_audio = GAME_AUDIO.read_text(encoding="utf-8")
    activity = ACTIVITY.read_text(encoding="utf-8")
    native = NATIVE.read_text(encoding="utf-8")
    generator = GENERATOR.read_text(encoding="utf-8")

    required_assets = {
        "sfx_footsteps_plains.wav",
        "sfx_footsteps_mountain.wav",
        "sfx_jump.wav",
        "sfx_landing.wav",
        "ambience_wind_plains.wav",
        "ambience_mountain_echo.wav",
    }
    for name in sorted(required_assets):
        source = AUDIO_ROOT / name
        raw = RAW_ROOT / name
        packed = PACK_ROOT / name
        for path in (source, raw, packed):
            if not path.is_file() or path.stat().st_size == 0:
                raise AssertionError(f"missing real audio payload: {path}")
        with wave.open(str(source), "rb") as clip:
            if clip.getnchannels() != 1 or clip.getframerate() != 48000 or clip.getsampwidth() != 2:
                raise AssertionError(f"unexpected WAV format: {source}")
        require(generator, name, f"generator output {name}")

    for marker, label in (
        ('load("footsteps_plains", R.raw.sfx_footsteps_plains)', "plains footsteps load"),
        ('load("footsteps_mountain", R.raw.sfx_footsteps_mountain)', "mountain footsteps load"),
        ('load("jump", R.raw.sfx_jump)', "jump load"),
        ('load("landing", R.raw.sfx_landing)', "landing load"),
        ('load("wind_plains", R.raw.ambience_wind_plains)', "wind load"),
        ('load("mountain_echo", R.raw.ambience_mountain_echo)', "echo load"),
        ("fun updateGameplayAudio", "gameplay audio update"),
        ("fun stopGameplayAudio", "gameplay audio cleanup"),
        ('audio.updateGameplayAudio(terrainAudio, locomotion, water)', "HUD audio wiring"),
        ('audio.stopGameplayAudio()', "pause audio cleanup"),
        ("const char* terrainAudioName()", "native terrain audio marker"),
        ('<< terrainAudioName();', "native HUD terrain audio field"),
    ):
        require(game_audio + activity + native, marker, label)

    print("AUDIO_CONTRACT_PASS=1")
    print(f"AUDIO_ASSET_COUNT={len(required_assets)}")
    print(f"AUDIO_ASSET_BYTES={sum((AUDIO_ROOT / name).stat().st_size for name in required_assets)}")


if __name__ == "__main__":
    main()
