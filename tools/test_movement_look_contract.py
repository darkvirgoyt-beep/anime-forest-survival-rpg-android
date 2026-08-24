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
    require("NativeGameBridge.orbitCamera(-dx * 0.0048f * lookSensitivity, -dy * 0.0032f * lookSensitivity)" in main, "look pad must follow finger direction")
    require("NativeGameBridge.orbitCamera(-dx * 0.0048f, -dy * 0.0032f)" in main, "fallback look path must use the same sign")
    require("InputFrame input{gMoveX, gMoveY, gController.camera.yaw, gSprintHeld}" in game, "native movement must not invert joystick axes")
    require("InputFrame input{-gMoveX" not in game, "legacy movement inversion must be removed")
    require("velocity.x += (targetX - velocity.x) * response" in physics, "horizontal movement must use smooth acceleration")
    require("velocity.y += (targetY - velocity.y) * response" in physics, "depth movement must use smooth acceleration")
    require("cameraRelative" in controller and "input.moveX * cosYaw" in controller, "movement must be camera-relative")
    require("activePointerId" in main and "findPointerIndex(activePointerId)" in main, "look pad must track one touch pointer")
    require("ACTION_POINTER_UP" in main and "ACTION_CANCEL" in main, "touch cancellation must be handled")

    print("MOVEMENT_LOOK_CONTRACT_PASS=1")
    print("JOYSTICK_AXES=direct")
    print("LOOK_AXES=direct_manipulation")
    print("MOVEMENT_RESPONSE=smooth_exponential")
    print("TOUCH_CANCEL_CLEAR=enabled")


if __name__ == "__main__":
    main()
