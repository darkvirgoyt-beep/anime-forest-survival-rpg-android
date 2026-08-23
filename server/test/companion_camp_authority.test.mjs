import assert from "node:assert/strict";
import {
  COMPANION_PROFILES,
  COMPANION_TARGET_SEEDS,
  authorityConstants,
  campRecipe,
  companionProfile,
  normalizeCampTransform,
  validateCampPlacement,
  validateCaptureRequest
} from "../src/companion-camp-authority.mjs";

const constants = authorityConstants();

assert.equal(Object.keys(COMPANION_PROFILES).length, 4);
assert.deepEqual(Object.keys(COMPANION_TARGET_SEEDS).sort(), Object.keys(COMPANION_PROFILES).sort());
assert.equal(companionProfile("MOON_DEER").displayName, "Moon Deer");
assert.equal(companionProfile("unknown_creature"), null);
assert.equal(campRecipe("FIELD_CAMP").woodCost, 6);
assert.equal(campRecipe("unknown_recipe"), null);

const acceptedCapture = validateCaptureRequest({
  creatureId: "moon_deer",
  distanceMeters: constants.captureDistanceMeters,
  healthFraction: constants.captureHealthFraction,
  fiber: 2,
  hasCompanion: false
});
assert.equal(acceptedCapture.accepted, true);
assert.equal(acceptedCapture.remainingFiber, 0);
assert.equal(validateCaptureRequest({ creatureId: "moon_deer", distanceMeters: constants.captureDistanceMeters + 0.01, healthFraction: 0.2, fiber: 2 }).error, "capture_target_out_of_range");
assert.equal(validateCaptureRequest({ creatureId: "moon_deer", distanceMeters: 0.1, healthFraction: 0.39, fiber: 2 }).error, "capture_target_too_healthy");
assert.equal(validateCaptureRequest({ creatureId: "moon_deer", distanceMeters: 0.1, healthFraction: 0.2, fiber: 1 }).error, "insufficient_fiber");
assert.equal(validateCaptureRequest({ creatureId: "moon_deer", distanceMeters: 0.1, healthFraction: 0.2, fiber: 2, hasCompanion: true }).error, "companion_slot_full");
assert.equal(validateCaptureRequest({ creatureId: "not_allowlisted", distanceMeters: 0.1, healthFraction: 0.2, fiber: 99 }).error, "creature_not_captureable");

assert.deepEqual(normalizeCampTransform({ x: 0.2, y: -0.1 }), { x: 0.2, y: -0.1, z: 0, yaw: 0, scale: 1 });
assert.deepEqual(normalizeCampTransform({ x: 999, y: -999, z: 999, yaw: 999, scale: 0 }), { x: 128, y: -128, z: 32, yaw: Math.PI, scale: 0.75 });
assert.equal(normalizeCampTransform({ x: null, y: 0 }), null);
assert.equal(normalizeCampTransform({ x: 0, y: "nope" }), null);

const validCamp = validateCampPlacement({
  recipeId: "field_camp",
  transform: { x: 0.1, y: 0.1, z: 0, yaw: 0, scale: 1 },
  playerPosition: { x: 0, y: 0 },
  slope: 0,
  existingCampCount: 0
});
assert.equal(validCamp.accepted, true);
assert.equal(validCamp.recipe.woodCost, 6);
assert.equal(validateCampPlacement({ recipeId: "field_camp", transform: { x: 0.7, y: 0 }, playerPosition: { x: 0, y: 0 } }).error, "camp_placement_out_of_range");
assert.equal(validateCampPlacement({ recipeId: "field_camp", transform: { x: 0, y: 0 }, playerPosition: { x: 0, y: 0 }, slope: constants.maxCampSlope + 0.01 }).error, "camp_placement_too_steep");
assert.equal(validateCampPlacement({ recipeId: "field_camp", transform: { x: 0, y: 0 }, playerPosition: { x: 0, y: 0 }, existingCampCount: 1 }).error, "camp_limit_reached");

console.log("companion_camp_authority_test: PASS");
