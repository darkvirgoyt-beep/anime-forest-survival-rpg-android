const CAPTURE_DISTANCE_METERS = 0.38;
const CAPTURE_HEALTH_FRACTION = 0.38;
const MAX_POSITION_DISTANCE_METERS = 0.65;
const MAX_CAMP_SLOPE = 0.55;
const MAX_CAMPS_PER_MEMBER = 1;

export const COMPANION_TARGET_SEEDS = Object.freeze({
  moon_deer: Object.freeze({ x: -0.62, y: 0.42, healthFraction: 0.28 }),
  mossback_boar: Object.freeze({ x: -0.28, y: 0.40, healthFraction: 0.34 }),
  river_otter: Object.freeze({ x: 0.42, y: -0.34, healthFraction: 0.25 }),
  canopy_fox: Object.freeze({ x: 0.64, y: 0.26, healthFraction: 0.30 })
});

export const COMPANION_PROFILES = Object.freeze({
  moon_deer: Object.freeze({ displayName: "Moon Deer", tameable: true, fiberCost: 2 }),
  mossback_boar: Object.freeze({ displayName: "Mossback Boar", tameable: true, fiberCost: 3 }),
  river_otter: Object.freeze({ displayName: "River Otter", tameable: true, fiberCost: 2 }),
  canopy_fox: Object.freeze({ displayName: "Canopy Fox", tameable: true, fiberCost: 2 })
});

export const CAMP_RECIPES = Object.freeze({
  field_camp: Object.freeze({
    displayName: "Field Camp",
    woodCost: 6,
    fiberCost: 4,
    maxDistanceMeters: MAX_POSITION_DISTANCE_METERS
  })
});

export function companionProfile(creatureId) {
  return typeof creatureId === "string" ? COMPANION_PROFILES[creatureId.trim().toLowerCase()] ?? null : null;
}

export function campRecipe(recipeId) {
  return typeof recipeId === "string" ? CAMP_RECIPES[recipeId.trim().toLowerCase()] ?? null : null;
}

export function validateCaptureRequest({ creatureId, distanceMeters, healthFraction, fiber, hasCompanion = false }) {
  const profile = companionProfile(creatureId);
  if (!profile) return { accepted: false, error: "creature_not_captureable" };
  if (hasCompanion) return { accepted: false, error: "companion_slot_full" };
  if (!Number.isFinite(distanceMeters) || distanceMeters < 0 || distanceMeters > CAPTURE_DISTANCE_METERS) return { accepted: false, error: "capture_target_out_of_range" };
  if (!Number.isFinite(healthFraction) || healthFraction < 0 || healthFraction > CAPTURE_HEALTH_FRACTION) return { accepted: false, error: "capture_target_too_healthy" };
  if (!Number.isInteger(fiber) || fiber < profile.fiberCost) return { accepted: false, error: "insufficient_fiber" };
  return { accepted: true, profile, remainingFiber: fiber - profile.fiberCost };
}

export function normalizeCampTransform(input) {
  if (!input || typeof input !== "object" || Array.isArray(input)) return null;
  const number = (value, fallback) => value === null || value === undefined || value === "" ? fallback : (Number.isFinite(Number(value)) ? Number(value) : fallback);
  const transform = {
    x: number(input.x, NaN),
    y: number(input.y, NaN),
    z: number(input.z, 0),
    yaw: number(input.yaw, 0),
    scale: number(input.scale, 1)
  };
  if (!Object.values(transform).every(Number.isFinite)) return null;
  transform.x = Math.max(-128, Math.min(128, transform.x));
  transform.y = Math.max(-128, Math.min(128, transform.y));
  transform.z = Math.max(-8, Math.min(32, transform.z));
  transform.yaw = Math.max(-Math.PI, Math.min(Math.PI, transform.yaw));
  transform.scale = Math.max(0.75, Math.min(1.25, transform.scale));
  return transform;
}

export function validateCampPlacement({ recipeId, transform, playerPosition, slope = 0, existingCampCount = 0 }) {
  const recipe = campRecipe(recipeId);
  const normalizedTransform = normalizeCampTransform(transform);
  if (!recipe || !normalizedTransform) return { accepted: false, error: "invalid_camp_recipe_or_transform" };
  if (!playerPosition || !Number.isFinite(Number(playerPosition.x)) || !Number.isFinite(Number(playerPosition.y))) {
    return { accepted: false, error: "player_position_required" };
  }
  const distance = Math.hypot(normalizedTransform.x - Number(playerPosition.x), normalizedTransform.y - Number(playerPosition.y));
  if (distance < 0 || distance > recipe.maxDistanceMeters) return { accepted: false, error: "camp_placement_out_of_range" };
  if (!Number.isFinite(Number(slope)) || Number(slope) < 0 || Number(slope) > MAX_CAMP_SLOPE) return { accepted: false, error: "camp_placement_too_steep" };
  if (!Number.isInteger(existingCampCount) || existingCampCount >= MAX_CAMPS_PER_MEMBER) return { accepted: false, error: "camp_limit_reached" };
  return { accepted: true, recipe, transform: normalizedTransform, distanceMeters: distance };
}

export function authorityConstants() {
  return Object.freeze({ captureDistanceMeters: CAPTURE_DISTANCE_METERS, captureHealthFraction: CAPTURE_HEALTH_FRACTION, maxCampsPerMember: MAX_CAMPS_PER_MEMBER, maxCampSlope: MAX_CAMP_SLOPE });
}
