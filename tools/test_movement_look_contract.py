#!/usr/bin/env python3
"""Validate intuitive joystick movement and direct-manipulation camera look signs."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt"
GAME = ROOT / "app/src/main/cpp/forest_game.cpp"
PHYSICS = ROOT / "app/src/main/cpp/physics/physics.cpp"
CONTROLLER = ROOT / "app/src/main/cpp/controller/third_person_controller.cpp"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL movement_look_contract: {message}")


def main() -> None:
    main = MAIN.read_text(encoding="utf-8")
    game = GAME.read_text(encoding="utf-8")
    physics = PHYSICS.read_text(encoding="utf-8")
    controller = CONTROLLER.read_text(encoding="utf-8")

    require("NativeGameBridge.setMove(x, y)" in main, "joystick must send normalized movement values")
    require("onMove(0f, 0f)" in main, "joystick must clear movement on release/cancel")
    require("NativeGameBridge.orbitCamera(dx * 0.0048f * lookSensitivity, dy * 0.0032f * lookSensitivity)" in main, "look pad must follow finger direction")
    require("NativeGameBridge.orbitCamera(dx * 0.0048f, dy * 0.0032f)" in main, "fallback look path must use the same sign")
    require("-dx * 0.0048f" not in main and "-dy * 0.0032f" not in main, "legacy inverted look signs must be removed")
    require("InputFrame input{gMoveX, gMoveY, gController.camera.yaw, gSprintHeld}" in game, "native movement must not invert joystick axes")
    require("InputFrame input{-gMoveX" not in game, "legacy movement inversion must be removed")
    require("velocity.x += (targetX - velocity.x) * response" in physics, "horizontal movement must use smooth acceleration")
    require("velocity.y += (targetY - velocity.y) * response" in physics, "depth movement must use smooth acceleration")
    require("cameraRelative" in controller and "input.moveX * cosYaw" in controller, "movement must be camera-relative")
    require("const bool pursuing = !mobProfile.tameable && distance < 0.34f" in game, "only nearby hostile creatures may pursue the player")
    require("mob.spawn.x + std::cos(phase)" in game, "non-engaged creatures must use deterministic home wandering")
    require("if (gJumpBufferSeconds > 0.0f)" in game and "if (gController.jump()) gJumpBufferSeconds = 0.0f" in game, "jump must use a buffered native request")
    require(game.index("if (gJumpBufferSeconds > 0.0f)") < game.index("gController.tick(input"), "buffered jump must be evaluated before movement simulation")
    require("activePointerId" in main and "findPointerIndex(activePointerId)" in main, "look pad must track one touch pointer")
    require("ACTION_POINTER_UP" in main and "ACTION_CANCEL" in main, "touch cancellation must be handled")
    require("private fun isAnchoredTouch" in main, "movement joystick must expose a fixed-anchor touch gate")
    require("if (!isAnchoredTouch(event.getX(index), event.getY(index))) return false" in main, "movement joystick must reject arbitrary left-screen touches")
    require("baseX = event.getX(index)" not in main and "baseY = event.getY(index)" not in main, "movement joystick must not relocate to touch-down")
    require("radius = (width.coerceAtMost(dp(360)) * 0.075f).coerceAtLeast(dp(36).toFloat())" in main, "movement joystick must use the compact anchored radius")
    require("x >= width * 0.50f" in main, "look controls must remain on the right half of the screen")

    print("MOVEMENT_LOOK_CONTRACT_PASS=1")
    print("JOYSTICK_AXES=direct")
    print("LOOK_AXES=direct_manipulation_non_inverted")
    print("JOYSTICK_ANCHOR=lower_left_fixed")
    print("MOVEMENT_RESPONSE=smooth_exponential")
    print("TOUCH_CANCEL_CLEAR=enabled")


if __name__ == "__main__":
    main()
