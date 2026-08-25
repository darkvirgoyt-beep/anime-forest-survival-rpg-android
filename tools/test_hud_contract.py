#!/usr/bin/env python3
"""Validate the original compact Android HUD layout and gameplay bindings."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt"
VIEWS = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/HudOverlayViews.kt"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL hud_contract: {message}")


def main() -> None:
    main = MAIN.read_text(encoding="utf-8")
    views = VIEWS.read_text(encoding="utf-8")

    for needle in (
        "private fun compactControlButton",
        "val quickSlots = LinearLayout(this)",
        "slotSymbols = listOf",
        "WORLD MAP",
        "TOWER",
        "TELEPORT",
        "MENU",
        'compactControlButton("⚔", "ATTACK")',
        'compactControlButton("↟", "JUMP")',
        'compactControlButton("◆", "DODGE")',
        'compactControlButton("♞", "SPRINT")',
        'compactControlButton("✧", "GATHER")',
        'compactControlButton("⌂", "CRAFT")',
        'NativeGameBridge.setSprintHeld(true)',
        'NativeGameBridge.setSprintHeld(false)',
        'submitAuthoritativeCombat("attack")',
        'submitAuthoritativeCombat("heavy_attack")',
        'submitAuthoritativeInventory("gather")',
        'submitAuthoritativeInventory("craft")',
        'NativeGameBridge.jump()',
        'NativeGameBridge.dodge()',
        'Gravity.BOTTOM or Gravity.END',
        'Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL',
        'setPadding(dp(7), dp(5), dp(12), dp(5))',
        'text = "AETHELGRAD"',
        'val profilePanel = LinearLayout(this)',
        'visibility = View.GONE',
    ):
        require(needle in main, f"missing HUD marker: {needle}")

    require('text = "$symbol\\n$label"' in main, "compact buttons must have two-line symbol labels")
    require('text = "${index + 1}\\n$symbol"' in main, "quick slots must have numbered labels")
    require('jump.setOnTouchListener' in main and 'MotionEvent.ACTION_DOWN' in main, "jump must dispatch on touch-down instead of waiting for click release")
    require("nextHunger: Int = 100" in views, "vital meter must support hunger")
    require('drawMeter(canvas, "HP"' in views, "health bar missing")
    require('drawMeter(canvas, "STA"' in views, "stamina bar missing")
    require('drawMeter(canvas, "HUN"' in views, "hunger bar missing")
    require("onMove(x, y)" in main or "setMove(x, y)" in main, "joystick movement binding missing")
    require("LookPadView" in main, "look pad must remain present for camera control")
    require('stateLabel.text = "$biome  •  DAY $daysPlayed  •  $weather"' in main, "region header must avoid gameplay debug clutter")
    print("HUD_CONTRACT_PASS=1")
    print("COMPACT_GROUPED_ACTIONS=enabled")
    print("QUICK_SLOTS=8")
    print("SURVIVAL_BARS=health,stamina,hunger")
    print("PLAYER_AND_GAMEPLAY_BINDINGS=preserved")


if __name__ == "__main__":
    main()
