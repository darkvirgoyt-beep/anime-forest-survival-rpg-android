#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>

#include "physics/physics.h"
#include "controller/third_person_controller.h"
#include "combat/combat_system.h"
#include "rpg/progression.h"
#include "rpg/cloud_state.h"
#include "rpg/emberling_companion.h"
#include "mobs/mob_catalog.h"

namespace {
constexpr float PI = 3.14159265359f;
GLuint gProgram = 0;
GLuint g3DProgram = 0;
GLuint gPlayerTexture = 0;
GLint gPosition = -1;
GLint gColor = -1;
GLint gScale = -1;
GLint gOffset = -1;
GLint g3DPosition = -1;
GLint g3DColor = -1;
GLint g3DMvp = -1;
GLint g3DLightLevel = -1;
GLint g3DFogColor = -1;
GLint g3DFogAmount = -1;
GLuint gBillboardProgram = 0;
GLint gBillboardPosition = -1;
GLint gBillboardUv = -1;
GLint gBillboardMvp = -1;
GLint gBillboardTexture = -1;
float gWidth = 1.0f;
float gHeight = 1.0f;
float gTime = 0.0f;
float gSceneLightLevel = 1.0f;
float gSceneFogR = 0.12f;
float gSceneFogG = 0.20f;
float gSceneFogB = 0.22f;
float gSceneFogAmount = 0.14f;
int gDaysPlayed = 1;
float gPlayerX = -0.55f;
float gPlayerY = -0.08f;
float gMoveX = 0.0f;
float gMoveY = 0.0f;
bool gSprintHeld = false;
float gGyroX = 0.0f;
float gGyroY = 0.0f;
bool gGyroEnabled = false;
bool gAuthoritativeOnline = false;
int gWood = 12;
int gFiber = 8;
int gStone = 4;
int gCraftPulse = 0;
int gAttackPulse = 0;
int gDodgePulse = 0;
int gLevelPulse = 0;
int gQuestPulse = 0;
float gJumpBufferSeconds = 0.0f;
int gGraphicsQuality = 2; // 0=Low, 1=Medium, 2=High, 3=Ultra, 4=Max
bool gContentTierReady = false;

int effectiveGraphicsQuality() {
    // Never show high-end effects against missing streamed content. The small
    // fallback scene remains readable until the selected PAD tier is mounted.
    return gContentTierReady ? gGraphicsQuality : std::min(gGraphicsQuality, 1);
}
float gHunger = 0.82f;
double gPhysicsAccumulator = 0.0;
constexpr float kPhysicsStep = 1.0f / 60.0f;
forest::controller::ThirdPersonController gController{};
forest::combat::CombatSystem gCombat{};
forest::rpg::Progression gProgression{};
forest::rpg::EmberlingState gEmberling{};
bool gHitRegistered = false;
constexpr float kForestWardenMaxHealth = 100.0f;
constexpr int kPlayerMaxHealthHp = 100;
float gEnemyHealth = kForestWardenMaxHealth;
float gEnemyHitFlash = 0.0f;
float gEnemyDefeatTimer = 0.0f;
float gEnemyX = -0.18f;
float gEnemyY = -0.08f;

struct MobState {
    forest::mobs::MobType type;
    forest::physics::Vec2 spawn;
    forest::physics::Vec2 position;
    float health;
    float hitFlash;
    float attackCooldown;
    float defeatTimer;
};

MobState gMobs[forest::mobs::kProfileCount] = {
    {forest::mobs::MobType::ArcaneWizard, {-0.26f, 0.02f}, {-0.26f, 0.02f}, 46.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Barbarian, {-0.04f, -0.12f}, {-0.04f, -0.12f}, 92.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Cleric, {0.18f, 0.14f}, {0.18f, 0.14f}, 64.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Monk, {0.36f, -0.16f}, {0.36f, -0.16f}, 58.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Necromancer, {-0.38f, 0.22f}, {-0.38f, 0.22f}, 52.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Samurai, {0.05f, 0.28f}, {0.05f, 0.28f}, 76.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Artificer, {0.42f, 0.12f}, {0.42f, 0.12f}, 62.0f, 0.0f, 0.0f, 0.0f},
    {forest::mobs::MobType::Druid, {-0.72f, 0.18f}, {-0.72f, 0.18f}, 70.0f, 0.0f, 0.0f, 0.0f}
};

void resetMobs() {
    for (MobState& mob : gMobs) {
        mob.position = mob.spawn;
        mob.health = static_cast<float>(forest::mobs::profile(mob.type).maxHealth);
        mob.hitFlash = 0.0f;
        mob.attackCooldown = 0.0f;
        mob.defeatTimer = 0.0f;
    }
}

int nearestLivingMob() {
    int nearest = -1;
    float nearestDistance = 100.0f;
    for (int i = 0; i < forest::mobs::kProfileCount; ++i) {
        const MobState& mob = gMobs[i];
        if (mob.health <= 0.0f) continue;
        const float dx = mob.position.x - gPlayerX;
        const float dy = mob.position.y - gPlayerY;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < nearestDistance) {
            nearest = i;
            nearestDistance = distance;
        }
    }
    return nearest;
}

const char* nearestMobStatus() {
    const int index = nearestLivingMob();
    if (index < 0) return "NO_TARGET";
    const MobState& mob = gMobs[index];
    const forest::mobs::MobProfile& mobProfile = forest::mobs::profile(mob.type);
    std::ostringstream status;
    status << mobProfile.displayName << '_' << static_cast<int>(std::round(mob.health));
    static std::string value;
    value = status.str();
    return value.c_str();
}

enum class ViewMode {
    ThirdPerson,
    FirstPerson
};
ViewMode gViewMode = ViewMode::ThirdPerson;
bool gWorldMapVisible = false;
float gTowerGlow = 0.0f;
float gTowerCooldown = 0.0f;
float gSynchronizedWorldTime = -1.0f;
int gTowerRevision = 0;
struct CoOpPeer {
    bool active = false;
    float x = 0.0f;
    float y = 0.0f;
    bool atTower = false;
};
CoOpPeer gCoOpPeers[3]{};

enum class Biome {
    Forest,
    Sand,
    Snow
};

// The prototype keeps compact normalized coordinates for GLES drawing, while the
// gameplay map is authored in a readable 100 x 100 world-unit coordinate system.
constexpr float kWorldMinX = 0.0f;
constexpr float kWorldMaxX = 100.0f;
constexpr float kWorldMinY = 0.0f;
constexpr float kWorldMaxY = 100.0f;
constexpr float kSimulationMinX = -0.90f;
constexpr float kSimulationMaxX = 0.90f;
constexpr float kSimulationMinY = -0.50f;
constexpr float kSimulationMaxY = 0.52f;

float worldXFromSimulation(float simulationX) {
    const float normalized = (simulationX - kSimulationMinX) / (kSimulationMaxX - kSimulationMinX);
    return kWorldMinX + std::clamp(normalized, 0.0f, 1.0f) * (kWorldMaxX - kWorldMinX);
}

float worldYFromSimulation(float simulationY) {
    const float normalized = (simulationY - kSimulationMinY) / (kSimulationMaxY - kSimulationMinY);
    return kWorldMinY + std::clamp(normalized, 0.0f, 1.0f) * (kWorldMaxY - kWorldMinY);
}

Biome currentBiome() {
    const float worldX = worldXFromSimulation(gPlayerX);
    if (worldX < 34.0f) return Biome::Forest;
    if (worldX < 68.0f) return Biome::Sand;
    return Biome::Snow;
}

const char* biomeName() {
    switch (currentBiome()) {
        case Biome::Forest: return "FOREST";
        case Biome::Sand: return "SAND";
        case Biome::Snow: return "SNOW";
    }
    return "SAND";
}

constexpr float kDayCycleSeconds = 900.0f; // 15 minutes total
constexpr float kDayPhaseSeconds = 360.0f; // 6 minutes
constexpr float kAfternoonPhaseSeconds = 270.0f; // 4.5 minutes
constexpr float kEveningPhaseSeconds = 180.0f; // 3 minutes
constexpr float kNightPhaseSeconds = 90.0f; // 1.5 minutes: intentionally short

enum class TimePhase {
    Day,
    Afternoon,
    Evening,
    Night
};

enum class WeatherState {
    Clear,
    Rain,
    Thunderstorm
};

// Weather repeats independently of the 15-minute calendar so a storm can cross
// phase boundaries without changing the player-facing day count.
constexpr float kWeatherCycleSeconds = 480.0f;
constexpr float kClearWeatherSeconds = 300.0f;
constexpr float kRainWeatherSeconds = 390.0f;

WeatherState currentWeather() {
    const float weatherTime = std::fmod(std::max(0.0f, gTime), kWeatherCycleSeconds);
    if (weatherTime < kClearWeatherSeconds) return WeatherState::Clear;
    if (weatherTime < kRainWeatherSeconds) return WeatherState::Rain;
    return WeatherState::Thunderstorm;
}

const char* weatherName() {
    switch (currentWeather()) {
        case WeatherState::Clear: return "CLEAR";
        case WeatherState::Rain: return "RAIN";
        case WeatherState::Thunderstorm: return "THUNDERSTORM";
    }
    return "CLEAR";
}

float rainIntensity() {
    switch (currentWeather()) {
        case WeatherState::Clear: return 0.0f;
        case WeatherState::Rain: return 0.55f;
        case WeatherState::Thunderstorm: return 0.92f;
    }
    return 0.0f;
}

float lightningIntensity() {
    if (currentWeather() != WeatherState::Thunderstorm) return 0.0f;
    const float weatherTime = std::fmod(std::max(0.0f, gTime), kWeatherCycleSeconds) - kRainWeatherSeconds;
    constexpr float flashes[] = {7.0f, 26.0f, 48.0f, 72.0f};
    float intensity = 0.0f;
    for (const float flashStart : flashes) {
        const float elapsed = weatherTime - flashStart;
        if (elapsed >= 0.0f && elapsed < 0.18f) {
            intensity = std::max(intensity, 1.0f - elapsed / 0.18f);
        }
    }
    return intensity;
}

TimePhase currentTimePhase() {
    const float phaseTime = std::fmod(std::max(0.0f, gTime), kDayCycleSeconds);
    if (phaseTime < kDayPhaseSeconds) return TimePhase::Day;
    if (phaseTime < kDayPhaseSeconds + kAfternoonPhaseSeconds) return TimePhase::Afternoon;
    if (phaseTime < kDayPhaseSeconds + kAfternoonPhaseSeconds + kEveningPhaseSeconds) return TimePhase::Evening;
    return TimePhase::Night;
}

float currentTimePhaseDuration() {
    switch (currentTimePhase()) {
        case TimePhase::Day: return kDayPhaseSeconds;
        case TimePhase::Afternoon: return kAfternoonPhaseSeconds;
        case TimePhase::Evening: return kEveningPhaseSeconds;
        case TimePhase::Night: return kNightPhaseSeconds;
    }
    return kDayPhaseSeconds;
}

const char* timePhaseName() {
    switch (currentTimePhase()) {
        case TimePhase::Day: return "DAY";
        case TimePhase::Afternoon: return "AFTERNOON";
        case TimePhase::Evening: return "EVENING";
        case TimePhase::Night: return "NIGHT";
    }
    return "DAY";
}

void updateCalendar() {
    gDaysPlayed = static_cast<int>(std::floor(std::max(0.0f, gTime) / kDayCycleSeconds)) + 1;
}

void applySynchronizedWorldTime() {
    if (gSynchronizedWorldTime < 0.0f) return;
    gTime = std::max(0.0f, gSynchronizedWorldTime);
    gSynchronizedWorldTime = -1.0f;
}

const forest::physics::StaticObstacle gObstacles[] = {
    {{{-0.30f, -0.28f}, {0.07f, 0.04f}}},
    {{{0.60f, -0.32f}, {0.06f, 0.04f}}}
};

// A shallow stream in the prototype demonstrates the same gameplay contract that
// production water volumes will use: surface height, current, buoyancy, and drag.
const forest::physics::WaterVolume gWaterVolumes[] = {
    {{{0.04f, -0.36f}, {0.20f, 0.11f}}, -0.28f, {0.025f, 0.0f}, 0.78f, 3.2f}
};

const char* waterStateName() {
    if (gController.body.water.submerged) return "SWIMMING";
    if (gController.body.water.overlapping) return "WADING";
    return "DRY";
}

const char* locomotionStateName() {
    using forest::controller::LocomotionState;
    switch (gController.state) {
        case LocomotionState::Idle: return "IDLE";
        case LocomotionState::Walk: return "WALK";
        case LocomotionState::Sprint: return "SPRINT";
        case LocomotionState::Jump: return "JUMP";
        case LocomotionState::Fall: return "FALL";
        case LocomotionState::Swim: return "SWIM";
        case LocomotionState::Dodge: return "DODGE";
        case LocomotionState::Slide: return "SLIDE";
        case LocomotionState::Attack: return "ATTACK";
        case LocomotionState::Hitstun: return "HITSTUN";
        case LocomotionState::Dead: return "DEAD";
    }
    return "IDLE";
}

const char* kVertexShader = R"GLSL(
#version 300 es
layout(location = 0) in vec2 aPosition;
uniform vec2 uScale;
uniform vec2 uOffset;
void main() {
    gl_Position = vec4(aPosition * uScale + uOffset, 0.0, 1.0);
}
)GLSL";

const char* kFragmentShader = R"GLSL(
#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; }
)GLSL";

const char* k3DVertexShader = R"GLSL(
#version 300 es
layout(location = 0) in vec3 aPosition;
uniform mat4 uMvp;
uniform vec4 uColor;
out vec4 vColor;
out float vHeightShade;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vColor = uColor;
    // Procedural meshes carry no normal stream, so their local height gives us
    // a stable stylized top-light gradient on every low-poly primitive.
    vHeightShade = clamp(aPosition.y * 0.72 + 0.5, 0.0, 1.0);
}
)GLSL";

const char* k3DFragmentShader = R"GLSL(
#version 300 es
precision mediump float;
in vec4 vColor;
in float vHeightShade;
uniform float uLightLevel;
uniform vec3 uFogColor;
uniform float uFogAmount;
out vec4 fragColor;
void main() {
    float topLight = mix(0.78, 1.16, vHeightShade);
    vec3 litColor = vColor.rgb * (uLightLevel * topLight + 0.12);
    // Depth fog adds the soft atmospheric separation seen in stylized open worlds.
    float depthFog = clamp(pow(gl_FragCoord.z, 2.2) * uFogAmount, 0.0, 0.86);
    fragColor = vec4(mix(litColor, uFogColor, depthFog), vColor.a);
}
)GLSL";

const char* kBillboardVertexShader = R"GLSL(
#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUv;
uniform mat4 uMvp;
out vec2 vUv;
void main() {
    gl_Position = uMvp * vec4(aPosition, 1.0);
    vUv = aUv;
}
)GLSL";

const char* kBillboardFragmentShader = R"GLSL(
#version 300 es
precision mediump float;
in vec2 vUv;
uniform sampler2D uTexture;
out vec4 fragColor;
void main() {
    vec4 color = texture(uTexture, vUv);
    if (color.a < 0.03) discard;
    fragColor = color;
}
)GLSL";

struct Mat4 {
    float v[16]{};
};

Mat4 identityMatrix() {
    Mat4 result{};
    result.v[0] = result.v[5] = result.v[10] = result.v[15] = 1.0f;
    return result;
}

Mat4 multiplyMatrix(const Mat4& a, const Mat4& b) {
    Mat4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            for (int k = 0; k < 4; ++k) result.v[column * 4 + row] += a.v[k * 4 + row] * b.v[column * 4 + k];
        }
    }
    return result;
}

Mat4 perspectiveMatrix(float fovRadians, float aspect, float nearPlane, float farPlane) {
    Mat4 result{};
    const float f = 1.0f / std::tan(fovRadians * 0.5f);
    result.v[0] = f / std::max(0.1f, aspect);
    result.v[5] = f;
    result.v[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result.v[11] = -1.0f;
    result.v[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return result;
}

struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3 subtractVec3(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 crossVec3(const Vec3& a, const Vec3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
Vec3 normalizeVec3(const Vec3& value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (length < 0.0001f) return {0.0f, 0.0f, -1.0f};
    return {value.x / length, value.y / length, value.z / length};
}

Mat4 lookAtMatrix(const Vec3& eye, const Vec3& center) {
    const Vec3 forward = normalizeVec3(subtractVec3(center, eye));
    const Vec3 side = normalizeVec3(crossVec3(forward, {0.0f, 1.0f, 0.0f}));
    const Vec3 up = crossVec3(side, forward);
    Mat4 result = identityMatrix();
    result.v[0] = side.x; result.v[4] = side.y; result.v[8] = side.z;
    result.v[1] = up.x; result.v[5] = up.y; result.v[9] = up.z;
    result.v[2] = -forward.x; result.v[6] = -forward.y; result.v[10] = -forward.z;
    result.v[12] = -(side.x * eye.x + side.y * eye.y + side.z * eye.z);
    result.v[13] = -(up.x * eye.x + up.y * eye.y + up.z * eye.z);
    result.v[14] = forward.x * eye.x + forward.y * eye.y + forward.z * eye.z;
    return result;
}

Mat4 modelMatrix(float x, float y, float z, float width, float height, float depth) {
    Mat4 result = identityMatrix();
    result.v[0] = width;
    result.v[5] = height;
    result.v[10] = depth;
    result.v[12] = x;
    result.v[13] = y;
    result.v[14] = z;
    return result;
}

GLuint compileShader(GLenum type, const char* source);

void createBillboardProgram() {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, kBillboardVertexShader);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, kBillboardFragmentShader);
    gBillboardProgram = glCreateProgram();
    glAttachShader(gBillboardProgram, vertex);
    glAttachShader(gBillboardProgram, fragment);
    glLinkProgram(gBillboardProgram);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    gBillboardPosition = glGetAttribLocation(gBillboardProgram, "aPosition");
    gBillboardUv = glGetAttribLocation(gBillboardProgram, "aUv");
    gBillboardMvp = glGetUniformLocation(gBillboardProgram, "uMvp");
    gBillboardTexture = glGetUniformLocation(gBillboardProgram, "uTexture");
}

void create3DProgram() {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, k3DVertexShader);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, k3DFragmentShader);
    g3DProgram = glCreateProgram();
    glAttachShader(g3DProgram, vertex);
    glAttachShader(g3DProgram, fragment);
    glLinkProgram(g3DProgram);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    g3DPosition = glGetAttribLocation(g3DProgram, "aPosition");
    g3DColor = glGetUniformLocation(g3DProgram, "uColor");
    g3DMvp = glGetUniformLocation(g3DProgram, "uMvp");
    g3DLightLevel = glGetUniformLocation(g3DProgram, "uLightLevel");
    g3DFogColor = glGetUniformLocation(g3DProgram, "uFogColor");
    g3DFogAmount = glGetUniformLocation(g3DProgram, "uFogAmount");
}

void draw3DBox(const Mat4& viewProjection, float x, float y, float z, float width, float height, float depth,
              float r, float g, float b, float a = 1.0f) {
    static const GLfloat cube[] = {
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
         0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
         0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f,-0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
        -0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f
    };
    glUseProgram(g3DProgram);
    const Mat4 mvp = multiplyMatrix(viewProjection, modelMatrix(x, y, z, width, height, depth));
    glUniformMatrix4fv(g3DMvp, 1, GL_FALSE, mvp.v);
    glUniform4f(g3DColor, r, g, b, a);
    glUniform1f(g3DLightLevel, gSceneLightLevel);
    glUniform3f(g3DFogColor, gSceneFogR, gSceneFogG, gSceneFogB);
    glUniform1f(g3DFogAmount, gSceneFogAmount);
    glVertexAttribPointer(g3DPosition, 3, GL_FLOAT, GL_FALSE, 0, cube);
    glEnableVertexAttribArray(g3DPosition);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void draw3DMesh(const Mat4& viewProjection, const std::vector<GLfloat>& vertices,
                 float x, float y, float z, float width, float height, float depth,
                 float r, float g, float b, float a = 1.0f, GLenum primitive = GL_TRIANGLES) {
    if (vertices.empty()) return;
    glUseProgram(g3DProgram);
    const Mat4 mvp = multiplyMatrix(viewProjection, modelMatrix(x, y, z, width, height, depth));
    glUniformMatrix4fv(g3DMvp, 1, GL_FALSE, mvp.v);
    glUniform4f(g3DColor, r, g, b, a);
    glUniform1f(g3DLightLevel, gSceneLightLevel);
    glUniform3f(g3DFogColor, gSceneFogR, gSceneFogG, gSceneFogB);
    glUniform1f(g3DFogAmount, gSceneFogAmount);
    glVertexAttribPointer(g3DPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices.data());
    glEnableVertexAttribArray(g3DPosition);
    glDrawArrays(primitive, 0, static_cast<GLsizei>(vertices.size() / 3));
}

void drawBillboard(const Mat4& viewProjection, const std::vector<GLfloat>& vertices) {
    if (gPlayerTexture == 0 || vertices.empty()) return;
    glDisable(GL_CULL_FACE);
    glUseProgram(gBillboardProgram);
    glUniformMatrix4fv(gBillboardMvp, 1, GL_FALSE, viewProjection.v);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPlayerTexture);
    glUniform1i(gBillboardTexture, 0);
    glVertexAttribPointer(gBillboardPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vertices.data());
    glEnableVertexAttribArray(gBillboardPosition);
    glVertexAttribPointer(gBillboardUv, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vertices.data() + 3);
    glEnableVertexAttribArray(gBillboardUv);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 5));
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_CULL_FACE);
}

void drawTextured3DPlayer(const Mat4& viewProjection, float px, float y, float pz) {
    const float facingX = -std::sin(gController.camera.yaw);
    const float facingZ = -std::cos(gController.camera.yaw);
    const float rightX = -facingZ;
    const float rightZ = facingX;
    constexpr float width = 1.18f;
    constexpr float height = 2.70f;
    const float leftX = px - rightX * width * 0.5f;
    const float leftZ = pz - rightZ * width * 0.5f;
    const float rightEdgeX = px + rightX * width * 0.5f;
    const float rightEdgeZ = pz + rightZ * width * 0.5f;
    const float bottom = 0.04f + y;
    const float top = bottom + height;
    const std::vector<GLfloat> vertices = {
        leftX, bottom, leftZ, 0.0f, 1.0f,
        rightEdgeX, bottom, rightEdgeZ, 1.0f, 1.0f,
        rightEdgeX, top, rightEdgeZ, 1.0f, 0.0f,
        leftX, bottom, leftZ, 0.0f, 1.0f,
        rightEdgeX, top, rightEdgeZ, 1.0f, 0.0f,
        leftX, top, leftZ, 0.0f, 0.0f
    };
    drawBillboard(viewProjection, vertices);
}

void draw3DCylinder(const Mat4& viewProjection, float x, float y, float z,
                    float radius, float height, float r, float g, float b, float a = 1.0f) {
    constexpr int segments = 12;
    std::vector<GLfloat> vertices;
    vertices.reserve(segments * 36);
    const float halfHeight = height * 0.5f;
    for (int i = 0; i < segments; ++i) {
        const float angle0 = (static_cast<float>(i) / segments) * 2.0f * PI;
        const float angle1 = (static_cast<float>(i + 1) / segments) * 2.0f * PI;
        const float x0 = std::cos(angle0) * radius;
        const float z0 = std::sin(angle0) * radius;
        const float x1 = std::cos(angle1) * radius;
        const float z1 = std::sin(angle1) * radius;
        vertices.insert(vertices.end(), {
            x0, -halfHeight, z0, x1, -halfHeight, z1, x1, halfHeight, z1,
            x0, -halfHeight, z0, x1, halfHeight, z1, x0, halfHeight, z0,
            0.0f, halfHeight, 0.0f, x1, halfHeight, z1, x0, halfHeight, z0,
            0.0f, -halfHeight, 0.0f, x0, -halfHeight, z0, x1, -halfHeight, z1
        });
    }
    draw3DMesh(viewProjection, vertices, x, y, z, 1.0f, 1.0f, 1.0f, r, g, b, a);
}

void draw3DSphere(const Mat4& viewProjection, float x, float y, float z,
                  float radius, float r, float g, float b, float a = 1.0f) {
    constexpr int slices = 14;
    constexpr int stacks = 8;
    std::vector<GLfloat> vertices;
    vertices.reserve(slices * stacks * 18);
    for (int stack = 0; stack < stacks; ++stack) {
        const float phi0 = -PI * 0.5f + PI * static_cast<float>(stack) / stacks;
        const float phi1 = -PI * 0.5f + PI * static_cast<float>(stack + 1) / stacks;
        for (int slice = 0; slice < slices; ++slice) {
            const float theta0 = 2.0f * PI * static_cast<float>(slice) / slices;
            const float theta1 = 2.0f * PI * static_cast<float>(slice + 1) / slices;
            const auto point = [](float phi, float theta) {
                return Vec3{std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta)};
            };
            const Vec3 a0 = point(phi0, theta0);
            const Vec3 b0 = point(phi0, theta1);
            const Vec3 a1 = point(phi1, theta0);
            const Vec3 b1 = point(phi1, theta1);
            vertices.insert(vertices.end(), {
                a0.x, a0.y, a0.z, b0.x, b0.y, b0.z, b1.x, b1.y, b1.z,
                a0.x, a0.y, a0.z, b1.x, b1.y, b1.z, a1.x, a1.y, a1.z
            });
        }
    }
    draw3DMesh(viewProjection, vertices, x, y, z, radius, radius, radius, r, g, b, a);
}

void draw3DGlowOrb(const Mat4& viewProjection, float x, float y, float z, float radius,
                   float r, float g, float b, float intensity) {
    const float pulse = 0.90f + 0.10f * std::sin(gTime * 4.0f + x * 0.4f);
    draw3DSphere(viewProjection, x, y, z, radius * (1.75f + 0.10f * pulse), r, g, b,
                 std::clamp(intensity * 0.12f, 0.02f, 0.26f));
    draw3DSphere(viewProjection, x, y, z, radius, std::min(1.0f, r * 1.35f),
                 std::min(1.0f, g * 1.25f), std::min(1.0f, b * 1.18f), std::clamp(intensity, 0.16f, 1.0f));
}

void draw3DTree(const Mat4& viewProjection, float x, float z, float scale, float tint) {
    draw3DCylinder(viewProjection, x, 0.42f * scale, z, 0.12f * scale, 0.84f * scale, 0.25f, 0.12f, 0.07f);
    draw3DCylinder(viewProjection, x + 0.06f * scale, 0.50f * scale, z - 0.03f * scale,
                   0.045f * scale, 0.65f * scale, 0.34f, 0.17f, 0.08f);
    draw3DSphere(viewProjection, x, 1.03f * scale, z, 0.52f * scale, 0.06f + tint, 0.24f + tint, 0.18f);
    draw3DSphere(viewProjection, x - 0.24f * scale, 0.87f * scale, z + 0.08f, 0.29f * scale, 0.04f + tint, 0.18f + tint, 0.14f);
    draw3DSphere(viewProjection, x + 0.24f * scale, 0.92f * scale, z - 0.06f, 0.27f * scale, 0.05f + tint, 0.20f + tint, 0.15f);
    draw3DSphere(viewProjection, x + 0.02f * scale, 1.25f * scale, z + 0.02f,
                 0.21f * scale, 0.16f + tint, 0.40f + tint, 0.22f);
}

void draw3DRock(const Mat4& viewProjection, float x, float z, float scale, float r, float g, float b) {
    draw3DBox(viewProjection, x, 0.075f * scale, z, 0.72f * scale, 0.14f * scale, 0.52f * scale,
              r * 0.52f, g * 0.52f, b * 0.52f, 0.72f);
    draw3DSphere(viewProjection, x - 0.05f * scale, 0.19f * scale, z, 0.30f * scale,
                 r, g, b, 0.96f);
    draw3DSphere(viewProjection, x + 0.18f * scale, 0.15f * scale, z + 0.05f, 0.18f * scale,
                 std::min(1.0f, r * 1.16f), std::min(1.0f, g * 1.16f), std::min(1.0f, b * 1.16f), 0.92f);
}

void draw3DGrassTuft(const Mat4& viewProjection, float x, float z, float scale, float r, float g, float b) {
    const float sway = 0.025f * std::sin(gTime * 2.4f + x * 1.7f + z);
    draw3DBox(viewProjection, x, 0.14f * scale, z, 0.035f * scale, 0.28f * scale, 0.035f * scale, r, g, b, 0.92f);
    draw3DBox(viewProjection, x + sway, 0.18f * scale, z + 0.02f, 0.032f * scale, 0.36f * scale, 0.032f,
              std::min(1.0f, r * 1.18f), std::min(1.0f, g * 1.12f), std::min(1.0f, b * 1.10f), 0.88f);
    draw3DBox(viewProjection, x - sway, 0.15f * scale, z - 0.02f, 0.028f * scale, 0.30f * scale, 0.028f,
              r * 0.82f, g * 0.92f, b * 0.84f, 0.86f);
}

void draw3DMob(const Mat4& viewProjection, const MobState& mob, bool combatTarget) {
    const forest::mobs::MobProfile& mobProfile = forest::mobs::profile(mob.type);
    const float scale = mobProfile.scale;
    const float x = mob.position.x * 4.3f;
    const float z = -mob.position.y * 4.0f;
    const float flash = mob.hitFlash > 0.0f ? 0.28f : 0.0f;
    const float skinR = std::min(1.0f, 0.72f + flash);
    const float skinG = std::min(1.0f, 0.44f + flash * 0.55f);
    const float skinB = std::min(1.0f, 0.30f + flash * 0.35f);
    const float y = 0.02f * std::sin(gTime * 4.0f + static_cast<float>(static_cast<int>(mob.type)));

    if (mob.health <= 0.0f) {
        if (mob.defeatTimer > 0.0f) {
            const float fade = std::clamp(mob.defeatTimer / 1.5f, 0.0f, 1.0f);
            draw3DSphere(viewProjection, x, 0.18f + (1.0f - fade) * 0.45f, z, 0.16f * scale,
                         0.32f, 0.76f, 0.86f, 0.18f * fade);
        }
        return;
    }

    // All archetypes share a readable contact shadow and a low-poly humanoid base.
    draw3DBox(viewProjection, x, 0.026f, z, 0.48f * scale, 0.025f, 0.34f * scale,
              0.015f, 0.025f, 0.035f, 0.50f);
    draw3DCylinder(viewProjection, x - 0.075f * scale, 0.25f * scale, z,
                   0.055f * scale, 0.44f * scale, 0.10f, 0.07f, 0.10f);
    draw3DCylinder(viewProjection, x + 0.075f * scale, 0.25f * scale, z,
                   0.055f * scale, 0.44f * scale, 0.10f, 0.07f, 0.10f);

    switch (mob.type) {
        case forest::mobs::MobType::ArcaneWizard:
            draw3DBox(viewProjection, x, 0.70f * scale + y, z, 0.46f * scale, 0.72f * scale, 0.34f * scale,
                      std::min(1.0f, 0.08f + flash), 0.14f, std::min(1.0f, 0.36f + flash));
            draw3DSphere(viewProjection, x, 1.23f * scale + y, z, 0.20f * scale, skinR, skinG, skinB);
            draw3DCylinder(viewProjection, x, 1.52f * scale + y, z, 0.17f * scale, 0.42f * scale,
                           0.04f, 0.08f, 0.22f);
            draw3DSphere(viewProjection, x, 1.76f * scale + y, z, 0.075f * scale, 0.22f, 0.72f, 1.0f, 0.94f);
            draw3DCylinder(viewProjection, x + 0.30f * scale, 0.78f * scale + y, z,
                           0.018f * scale, 1.18f * scale, 0.20f, 0.12f, 0.08f);
            draw3DSphere(viewProjection, x + 0.30f * scale, 1.42f * scale + y, z, 0.085f * scale,
                         0.24f, 0.86f, 1.0f, 0.90f);
            break;
        case forest::mobs::MobType::Barbarian:
            draw3DBox(viewProjection, x, 0.73f * scale + y, z, 0.58f * scale, 0.70f * scale, 0.40f * scale,
                      std::min(1.0f, 0.30f + flash), 0.12f, 0.07f);
            draw3DSphere(viewProjection, x, 1.24f * scale + y, z, 0.22f * scale, skinR, skinG, skinB);
            draw3DBox(viewProjection, x - 0.25f * scale, 0.88f * scale + y, z,
                      0.18f * scale, 0.52f * scale, 0.22f * scale, 0.28f, 0.31f, 0.35f);
            draw3DBox(viewProjection, x + 0.25f * scale, 0.88f * scale + y, z,
                      0.18f * scale, 0.52f * scale, 0.22f * scale, 0.28f, 0.31f, 0.35f);
            draw3DCylinder(viewProjection, x + 0.42f * scale, 0.98f * scale + y, z,
                           0.022f * scale, 1.42f * scale, 0.20f, 0.12f, 0.07f);
            draw3DBox(viewProjection, x + 0.42f * scale, 1.58f * scale + y, z,
                      0.34f * scale, 0.34f * scale, 0.12f * scale, 0.46f, 0.47f, 0.50f);
            draw3DBox(viewProjection, x + 0.58f * scale, 1.58f * scale + y, z,
                      0.16f * scale, 0.26f * scale, 0.12f * scale, 0.58f, 0.59f, 0.62f);
            break;
        case forest::mobs::MobType::Cleric:
            draw3DBox(viewProjection, x, 0.72f * scale + y, z, 0.48f * scale, 0.74f * scale, 0.34f * scale,
                      std::min(1.0f, 0.80f + flash), std::min(1.0f, 0.82f + flash), std::min(1.0f, 0.86f + flash));
            draw3DSphere(viewProjection, x, 1.25f * scale + y, z, 0.20f * scale, skinR, skinG, skinB);
            draw3DCylinder(viewProjection, x, 1.50f * scale + y, z, 0.27f * scale, 0.035f * scale,
                           0.98f, 0.74f, 0.25f);
            draw3DCylinder(viewProjection, x + 0.30f * scale, 0.82f * scale + y, z,
                           0.018f * scale, 1.35f * scale, 0.56f, 0.34f, 0.12f);
            draw3DSphere(viewProjection, x + 0.30f * scale, 1.54f * scale + y, z, 0.095f * scale,
                         1.0f, 0.82f, 0.28f, 0.95f);
            draw3DBox(viewProjection, x, 0.82f * scale + y, z - 0.20f * scale,
                      0.14f * scale, 0.32f * scale, 0.06f * scale, 0.96f, 0.70f, 0.20f);
            break;
        case forest::mobs::MobType::Monk:
            draw3DBox(viewProjection, x, 0.72f * scale + y, z, 0.40f * scale, 0.62f * scale, 0.30f * scale,
                      std::min(1.0f, 0.76f + flash), std::min(1.0f, 0.38f + flash * 0.4f), 0.08f);
            draw3DSphere(viewProjection, x, 1.22f * scale + y, z, 0.19f * scale, skinR, skinG, skinB);
            draw3DCylinder(viewProjection, x, 1.42f * scale + y, z, 0.18f * scale, 0.12f * scale,
                           0.28f, 0.13f, 0.06f);
            draw3DCylinder(viewProjection, x - 0.34f * scale, 0.83f * scale + y, z,
                           0.016f * scale, 1.50f * scale, 0.38f, 0.22f, 0.10f);
            draw3DSphere(viewProjection, x - 0.34f * scale, 1.60f * scale + y, z, 0.045f * scale,
                         0.95f, 0.72f, 0.25f);
            draw3DBox(viewProjection, x - 0.23f * scale, 0.88f * scale + y, z,
                      0.08f * scale, 0.08f * scale, 0.48f * scale, skinR, skinG, skinB);
            draw3DBox(viewProjection, x + 0.23f * scale, 0.88f * scale + y, z,
                      0.08f * scale, 0.08f * scale, 0.48f * scale, skinR, skinG, skinB);
            break;
        case forest::mobs::MobType::Necromancer:
            draw3DBox(viewProjection, x, 0.70f * scale + y, z, 0.56f * scale, 0.78f * scale, 0.38f * scale,
                      std::min(1.0f, 0.18f + flash), 0.035f, std::min(1.0f, 0.26f + flash));
            draw3DSphere(viewProjection, x, 1.25f * scale + y, z, 0.22f * scale, 0.16f, 0.08f, 0.20f);
            draw3DCylinder(viewProjection, x, 1.28f * scale + y, z, 0.23f * scale, 0.36f * scale,
                           0.09f, 0.035f, 0.13f);
            draw3DCylinder(viewProjection, x + 0.33f * scale, 0.80f * scale + y, z,
                           0.018f * scale, 1.42f * scale, 0.12f, 0.06f, 0.16f);
            draw3DSphere(viewProjection, x + 0.33f * scale, 1.52f * scale + y, z, 0.10f * scale,
                         0.36f, 0.98f, 0.42f, 0.86f);
            draw3DSphere(viewProjection, x - 0.30f * scale, 0.62f * scale + y, z - 0.08f,
                         0.085f * scale, 0.72f, 0.82f, 0.78f);
            break;
        case forest::mobs::MobType::Samurai:
            draw3DBox(viewProjection, x, 0.74f * scale + y, z, 0.48f * scale, 0.70f * scale, 0.36f * scale,
                      std::min(1.0f, 0.50f + flash), 0.06f, 0.08f);
            draw3DSphere(viewProjection, x, 1.25f * scale + y, z, 0.20f * scale, skinR, skinG, skinB);
            draw3DBox(viewProjection, x, 1.44f * scale + y, z, 0.42f * scale, 0.16f * scale, 0.36f * scale,
                      0.10f, 0.13f, 0.20f);
            draw3DBox(viewProjection, x, 1.57f * scale + y, z, 0.62f * scale, 0.08f * scale, 0.15f * scale,
                      0.76f, 0.58f, 0.22f);
            draw3DCylinder(viewProjection, x + 0.36f * scale, 0.95f * scale + y, z,
                           0.014f * scale, 1.44f * scale, 0.12f, 0.08f, 0.06f);
            draw3DBox(viewProjection, x + 0.36f * scale, 1.66f * scale + y, z,
                      0.06f * scale, 0.62f * scale, 0.035f * scale, 0.84f, 0.88f, 0.92f);
            draw3DBox(viewProjection, x + 0.36f * scale, 1.37f * scale + y, z,
                      0.20f * scale, 0.04f * scale, 0.08f * scale, 0.96f, 0.72f, 0.22f);
            break;
        case forest::mobs::MobType::Artificer:
            draw3DBox(viewProjection, x, 0.72f * scale + y, z, 0.48f * scale, 0.70f * scale, 0.36f * scale,
                      std::min(1.0f, 0.50f + flash), 0.30f, 0.14f);
            draw3DSphere(viewProjection, x, 1.25f * scale + y, z, 0.20f * scale, skinR, skinG, skinB);
            draw3DSphere(viewProjection, x - 0.075f * scale, 1.30f * scale + y, z - 0.18f * scale,
                         0.045f * scale, 0.20f, 0.52f, 0.68f);
            draw3DSphere(viewProjection, x + 0.075f * scale, 1.30f * scale + y, z - 0.18f * scale,
                         0.045f * scale, 0.20f, 0.52f, 0.68f);
            draw3DBox(viewProjection, x + 0.31f * scale, 0.80f * scale + y, z,
                      0.12f * scale, 0.24f * scale, 0.22f * scale, 0.12f, 0.16f, 0.18f);
            draw3DCylinder(viewProjection, x + 0.37f * scale, 0.98f * scale + y, z,
                           0.018f * scale, 0.86f * scale, 0.76f, 0.42f, 0.12f);
            draw3DSphere(viewProjection, x + 0.37f * scale, 1.43f * scale + y, z, 0.07f * scale,
                         0.98f, 0.58f, 0.16f);
            break;
        case forest::mobs::MobType::Druid:
            draw3DBox(viewProjection, x, 0.72f * scale + y, z, 0.54f * scale, 0.76f * scale, 0.40f * scale,
                      std::min(1.0f, 0.14f + flash), std::min(1.0f, 0.34f + flash * 0.45f), 0.18f);
            draw3DSphere(viewProjection, x, 1.25f * scale + y, z, 0.21f * scale, 0.36f, 0.62f, 0.27f);
            draw3DSphere(viewProjection, x, 1.17f * scale + y, z - 0.17f * scale, 0.14f * scale,
                         0.12f, 0.30f, 0.14f);
            draw3DSphere(viewProjection, x, 1.48f * scale + y, z, 0.23f * scale,
                         0.08f, 0.22f, 0.12f);
            draw3DCylinder(viewProjection, x + 0.36f * scale, 0.82f * scale + y, z,
                           0.022f * scale, 1.52f * scale, 0.26f, 0.15f, 0.07f);
            draw3DSphere(viewProjection, x + 0.36f * scale, 1.62f * scale + y, z, 0.10f * scale,
                         0.30f, 0.74f, 0.28f);
            // Small fox-like spirit companion, echoing the reference sheet.
            draw3DSphere(viewProjection, x - 0.32f * scale, 0.34f * scale + y, z + 0.10f,
                         0.10f * scale, 0.92f, 0.34f, 0.10f);
            draw3DSphere(viewProjection, x - 0.32f * scale, 0.50f * scale + y, z + 0.10f,
                         0.075f * scale, 0.98f, 0.48f, 0.14f);
            break;
    }

    if (combatTarget) {
        const float healthRatio = std::clamp(mob.health / static_cast<float>(mobProfile.maxHealth), 0.0f, 1.0f);
        draw3DBox(viewProjection, x, 1.96f * scale, z, 0.52f * scale, 0.028f, 0.028f,
                  0.02f, 0.04f, 0.06f, 0.92f);
        draw3DBox(viewProjection, x - 0.26f * scale + 0.26f * scale * healthRatio,
                  1.96f * scale, z, 0.52f * scale * healthRatio, 0.018f, 0.018f,
                  0.28f, 0.90f, 0.52f, 0.98f);
    }
}

void draw3DMobs(const Mat4& viewProjection) {
    const int nearest = nearestLivingMob();
    for (int i = 0; i < forest::mobs::kProfileCount; ++i) {
        const MobState& mob = gMobs[i];
        const float dx = mob.position.x - gPlayerX;
        const float dy = mob.position.y - gPlayerY;
        const bool combatTarget = i == nearest && std::sqrt(dx * dx + dy * dy) < 0.52f;
        draw3DMob(viewProjection, mob, combatTarget);
    }
}

void draw3DPlayer(const Mat4& viewProjection, bool firstPerson) {
    const float jumpHeight = gController.body.verticalPosition;
    if (firstPerson) {
        draw3DBox(viewProjection, 0.36f, -0.18f + jumpHeight, -0.72f, 0.08f, 0.08f, 0.82f, 0.78f, 0.81f, 0.84f, 0.92f);
        draw3DBox(viewProjection, 0.36f, -0.25f + jumpHeight, -0.36f, 0.16f, 0.06f, 0.10f, 0.92f, 0.62f, 0.22f, 0.96f);
        return;
    }
    const float px = gPlayerX * 4.3f;
    const float pz = -gPlayerY * 4.0f;
    const float speed = std::sqrt(gController.body.velocity.x * gController.body.velocity.x +
                                  gController.body.velocity.y * gController.body.velocity.y);
    const float bob = gController.body.grounded && speed > 0.05f ? std::sin(gTime * 14.0f) * 0.035f : 0.0f;
    const float y = jumpHeight + bob;

    // A soft contact shadow grounds the avatar while the independent vertical
    // offset makes the jump readable from the third-person camera.
    draw3DBox(viewProjection, px, 0.026f, pz, 0.58f, 0.025f, 0.38f, 0.02f, 0.03f, 0.04f, 0.52f);
    if (gPlayerTexture != 0) {
        drawTextured3DPlayer(viewProjection, px, y, pz);
        return;
    }
    draw3DCylinder(viewProjection, px - 0.10f, 0.25f + y, pz, 0.07f, 0.50f, 0.08f, 0.04f, 0.14f);
    draw3DCylinder(viewProjection, px + 0.10f, 0.25f + y, pz, 0.07f, 0.50f, 0.10f, 0.05f, 0.17f);
    draw3DBox(viewProjection, px, 0.72f + y, pz, 0.42f, 0.70f, 0.28f, 0.45f, 0.10f, 0.28f);
    draw3DSphere(viewProjection, px, 1.24f + y, pz, 0.23f, 0.73f, 0.37f, 0.26f);
    draw3DSphere(viewProjection, px, 1.40f + y, pz - 0.015f, 0.25f, 0.10f, 0.04f, 0.18f);
    draw3DBox(viewProjection, px - 0.28f, 0.78f + y, pz, 0.10f, 0.56f, 0.11f, 0.48f, 0.12f, 0.30f);
    draw3DBox(viewProjection, px + 0.28f, 0.78f + y, pz, 0.10f, 0.56f, 0.11f, 0.48f, 0.12f, 0.30f);
    draw3DCylinder(viewProjection, px + 0.38f, 0.80f + y, pz, 0.025f, 0.94f, 0.90f, 0.70f, 0.24f);
}

void drawTeleportationTower(const Mat4& viewProjection) {
    const float pulse = 0.72f + 0.18f * std::sin(gTime * 2.7f);
    draw3DBox(viewProjection, 0.0f, 1.0f, -1.15f, 1.08f, 2.0f, 1.08f, 0.07f, 0.11f, 0.17f);
    draw3DBox(viewProjection, 0.0f, 2.18f, -1.15f, 0.72f, 0.42f, 0.72f, 0.30f, 0.13f, 0.26f);
    draw3DBox(viewProjection, 0.0f, 2.62f, -1.15f, 0.22f, 0.58f, 0.22f, 0.93f, 0.66f, 0.22f, pulse);
    draw3DGlowOrb(viewProjection, 0.0f, 3.04f, -1.15f, 0.14f, 1.0f, 0.60f, 0.18f, pulse);
    draw3DBox(viewProjection, -0.46f, 1.15f, -1.15f, 0.16f, 1.62f, 0.16f, 0.72f, 0.48f, 0.16f);
    draw3DBox(viewProjection, 0.46f, 1.15f, -1.15f, 0.16f, 1.62f, 0.16f, 0.72f, 0.48f, 0.16f);
    draw3DBox(viewProjection, 0.0f, 0.08f, -1.15f, 1.50f, 0.08f, 1.50f, 0.95f, 0.69f, 0.24f, 0.55f);
    if (gTowerGlow > 0.0f) {
        const float glow = std::clamp(gTowerGlow / 1.8f, 0.0f, 1.0f);
        draw3DBox(viewProjection, 0.0f, 3.12f, -1.15f, 0.34f, 0.34f, 0.34f, 1.0f, 0.82f, 0.28f, glow);
    }
}

void drawQuad(float x, float y, float width, float height, float r, float g, float b, float a);
void drawTriangle(float x, float y, float width, float height, float r, float g, float b, float a);
void drawCircle(float x, float y, float radius, float r, float g, float b, float a);

void draw3DPeer(const Mat4& viewProjection, const CoOpPeer& peer, int index) {
    if (!peer.active) return;
    const float px = peer.x * 4.3f;
    const float pz = -peer.y * 4.0f;
    const float tint = 0.14f * static_cast<float>(index);
    draw3DBox(viewProjection, px, 0.26f, pz, 0.34f, 0.50f, 0.26f, 0.18f + tint, 0.30f, 0.62f);
    draw3DBox(viewProjection, px, 0.72f, pz, 0.32f, 0.34f, 0.32f, 0.82f, 0.52f, 0.30f);
    if (peer.atTower) {
        draw3DBox(viewProjection, px, 0.04f, pz, 0.56f, 0.05f, 0.56f, 1.0f, 0.78f, 0.26f, 0.82f);
    }
}

void draw3DEmberling(const Mat4& viewProjection) {
    const float px = gEmberling.x * 4.3f;
    const float pz = -gEmberling.y * 4.0f;
    const float bob = 0.05f + 0.028f * std::sin(gTime * 5.1f);
    const float glow = 0.34f + 0.20f * gEmberling.pulse + (gEmberling.bonded ? 0.16f : 0.0f);
    draw3DBox(viewProjection, px, 0.08f, pz, 0.58f, 0.035f, 0.58f, 1.0f, 0.46f, 0.12f, glow * 0.34f);
    draw3DBox(viewProjection, px, 0.30f + bob, pz, 0.42f, 0.32f, 0.56f, 0.18f, 0.30f, 0.34f);
    draw3DBox(viewProjection, px, 0.54f + bob, pz + 0.06f, 0.30f, 0.25f, 0.30f, 0.58f, 0.84f, 0.90f);
    draw3DBox(viewProjection, px - 0.16f, 0.52f + bob, pz + 0.04f, 0.13f, 0.24f, 0.13f, 0.26f, 0.60f, 0.66f);
    draw3DBox(viewProjection, px + 0.16f, 0.52f + bob, pz + 0.04f, 0.13f, 0.24f, 0.13f, 0.26f, 0.60f, 0.66f);
    draw3DBox(viewProjection, px, 0.73f + bob, pz + 0.07f, 0.06f, 0.18f, 0.06f, 1.0f, 0.68f, 0.20f, glow);
    draw3DGlowOrb(viewProjection, px, 0.78f + bob, pz + 0.07f, 0.075f, 1.0f, 0.38f, 0.08f, glow);
}

void draw3DSkyOrb(const Mat4& viewProjection, float px, float pz, float yaw, float daylight) {
    const bool night = currentTimePhase() == TimePhase::Night;
    const float orbX = px + std::sin(yaw) * 22.0f;
    const float orbZ = pz + std::cos(yaw) * 22.0f;
    if (night) {
        draw3DSphere(viewProjection, orbX, 8.0f, orbZ, 1.45f, 0.78f, 0.86f, 1.0f, 0.92f);
        draw3DSphere(viewProjection, orbX + 0.30f, 8.12f, orbZ - 0.08f, 1.42f, 0.015f, 0.035f, 0.11f, 0.92f);
    } else {
        const float warmth = 0.86f + daylight * 0.14f;
        draw3DSphere(viewProjection, orbX, 8.0f, orbZ, 1.30f, 1.0f, warmth, 0.42f, 0.92f);
        draw3DSphere(viewProjection, orbX, 8.0f, orbZ, 1.75f, 1.0f, 0.66f, 0.20f, 0.12f);
    }
}

void draw3DVegetationDetails(const Mat4& viewProjection, float daylight) {
    constexpr float details[][4] = {
        {-6.0f, -0.30f, 0.22f, 0.00f}, {-5.0f, 2.15f, 0.17f, 0.10f},
        {-3.4f, 3.05f, 0.20f, 0.05f}, {3.95f, 2.10f, 0.18f, 0.16f},
        {5.30f, -0.62f, 0.15f, 0.08f}, {2.55f, 1.55f, 0.13f, 0.02f},
        {-1.75f, 1.25f, 0.14f, 0.12f}, {1.90f, 3.20f, 0.12f, 0.04f}
    };
    const int visible = std::min(8, 3 + effectiveGraphicsQuality() * 2);
    for (int i = 0; i < visible; ++i) {
        const float x = details[i][0];
        const float z = details[i][1];
        const float size = details[i][2];
        const float sway = std::sin(gTime * 2.2f + static_cast<float>(i)) * 0.012f;
        draw3DBox(viewProjection, x, 0.10f, z, size * 0.18f, 0.20f, size * 0.12f,
                  0.07f * daylight, 0.25f * daylight, 0.16f * daylight, 0.92f);
        draw3DBox(viewProjection, x + sway, 0.24f, z, size * 0.10f, 0.30f, size * 0.08f,
                  0.18f * daylight, 0.52f * daylight, 0.25f * daylight, 0.88f);
        draw3DBox(viewProjection, x - sway, 0.22f, z + 0.015f, size * 0.08f, 0.26f, size * 0.08f,
                  0.28f * daylight, 0.66f * daylight, 0.30f * daylight, 0.82f);
    }
}

void draw3DWaterSurface(const Mat4& viewProjection) {
    const auto& stream = gWaterVolumes[0];
    const float x = stream.bounds.center.x * 4.3f;
    const float z = -stream.bounds.center.y * 4.0f;
    const float width = stream.bounds.halfExtents.x * 8.6f;
    const float depth = stream.bounds.halfExtents.y * 8.0f;
    draw3DBox(viewProjection, x, stream.surfaceY + 0.026f, z, width, 0.024f, depth,
              0.04f, 0.30f, 0.42f, 0.78f);
    const int waves = std::min(7, 3 + effectiveGraphicsQuality());
    for (int i = 0; i < waves; ++i) {
        const float waveX = x - width * 0.36f + static_cast<float>(i) * width * 0.12f;
        const float waveZ = z + std::sin(gTime * 2.4f + static_cast<float>(i)) * depth * 0.23f;
        draw3DBox(viewProjection, waveX, stream.surfaceY + 0.045f, waveZ, width * 0.08f, 0.010f, 0.035f,
                  0.30f, 0.82f, 0.88f, 0.72f);
    }
}

void draw3DWeather(const Mat4& viewProjection) {
    const float intensity = rainIntensity();
    if (intensity <= 0.0f) return;
    const int drops = 20 + effectiveGraphicsQuality() * 8;
    for (int i = 0; i < drops; ++i) {
        const float seed = static_cast<float>(i) * 0.371f;
        const float x = -8.0f + std::fmod(seed * 13.0f + gTime * 1.9f, 16.0f);
        const float z = -6.0f + std::fmod(seed * 17.0f + gTime * 1.3f, 14.0f);
        const float y = 3.9f - std::fmod(seed * 5.0f + gTime * (2.8f + intensity), 5.0f);
        draw3DBox(viewProjection, x, y, z, 0.018f, 0.48f + intensity * 0.20f, 0.018f, 0.36f, 0.72f, 0.88f, 0.48f);
    }
    const float flash = lightningIntensity();
    if (flash > 0.0f) {
        draw3DBox(viewProjection, 0.0f, 4.0f, 0.0f, 18.0f, 0.05f, 18.0f, 0.72f, 0.86f, 1.0f, flash * 0.42f);
    }
}

void draw3DMapOverlay() {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(gProgram);
    constexpr float left = -0.86f;
    constexpr float right = -0.26f;
    constexpr float bottom = 0.02f;
    constexpr float top = 0.68f;
    const auto mapX = [](float xKm) { return left + (xKm / 100.0f) * (right - left); };
    const auto mapY = [](float yKm) { return bottom + (yKm / 100.0f) * (top - bottom); };

    drawQuad(-0.56f, 0.35f, 0.68f, 0.72f, 0.015f, 0.035f, 0.055f, 0.96f);
    drawQuad(mapX(16.5f), 0.35f, (right - left) * 0.33f, top - bottom - 0.04f, 0.10f, 0.24f, 0.18f, 0.96f);
    drawQuad(mapX(50.5f), 0.35f, (right - left) * 0.34f, top - bottom - 0.04f, 0.42f, 0.25f, 0.10f, 0.96f);
    drawQuad(mapX(84.0f), 0.35f, (right - left) * 0.32f, top - bottom - 0.04f, 0.22f, 0.40f, 0.52f, 0.96f);

    // Main road and river are deliberately thick in the harness so they remain
    // readable on small phone screens while the Unreal map uses authored splines.
    drawQuad(mapX(49.0f), mapY(50.0f), right - left - 0.06f, 0.014f, 0.86f, 0.66f, 0.25f, 0.92f);
    drawQuad(mapX(31.0f), mapY(50.0f), 0.016f, top - bottom - 0.08f, 0.18f, 0.67f, 0.76f, 0.90f);

    const float landmarks[][2] = {
        {10.0f, 16.0f}, {14.0f, 40.0f}, {26.0f, 78.0f},
        {40.0f, 48.0f}, {47.0f, 28.0f}, {55.0f, 69.0f},
        {74.0f, 57.0f}, {84.0f, 59.0f}, {88.0f, 83.0f}
    };
    for (const auto& landmark : landmarks) {
        drawCircle(mapX(landmark[0]), mapY(landmark[1]), 0.012f, 0.95f, 0.80f, 0.35f, 0.98f);
    }
    const float playerMapX = mapX(std::clamp((gPlayerX + 0.90f) / 1.80f * 100.0f, 0.0f, 100.0f));
    const float playerMapY = mapY(std::clamp((gPlayerY + 0.55f) / 1.10f * 100.0f, 0.0f, 100.0f));
    drawCircle(playerMapX, playerMapY, 0.030f, 1.0f, 0.76f, 0.24f, 1.0f);
    drawCircle(playerMapX, playerMapY, 0.012f, 0.10f, 0.12f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

float prototypeTerrainHeight(int chunkX, int chunkZ) {
    const float x = static_cast<float>(chunkX);
    const float z = static_cast<float>(chunkZ);
    return 0.018f * std::sin(x * 1.7f + z * 0.8f) + 0.012f * std::cos(z * 1.2f - x * 0.4f);
}

void drawPrototypeTerrainChunks(const Mat4& viewProjection, float daylight) {
    constexpr int chunkRadius = 2;
    constexpr float chunkSize = 3.6f;
    for (int chunkZ = -chunkRadius; chunkZ <= chunkRadius; ++chunkZ) {
        for (int chunkX = -chunkRadius; chunkX <= chunkRadius; ++chunkX) {
            const float worldX = static_cast<float>(chunkX) * chunkSize;
            const float worldZ = static_cast<float>(chunkZ) * chunkSize;
            const float terrainY = prototypeTerrainHeight(chunkX, chunkZ);
            const bool riverBand = chunkX == 1;
            const bool roadBand = chunkZ == 0 && chunkX != 1;
            const float topR = riverBand ? 0.08f : roadBand ? 0.33f : 0.16f;
            const float topG = riverBand ? 0.36f : roadBand ? 0.19f : 0.29f;
            const float topB = riverBand ? 0.43f : roadBand ? 0.08f : 0.18f;
            const float shimmer = 0.018f * std::sin(gTime * 1.6f + static_cast<float>(chunkX * 3 + chunkZ));
            // Solid terrain body plus a separate thin topsoil surface makes the
            // playable ground and its upper face explicit in the GLES slice.
            draw3DBox(viewProjection, worldX, terrainY - 0.10f, worldZ,
                      chunkSize - 0.06f, 0.20f, chunkSize - 0.06f,
                      0.06f * daylight, 0.15f * daylight, 0.12f * daylight);
            draw3DBox(viewProjection, worldX, terrainY + 0.012f + shimmer, worldZ,
                      chunkSize - 0.10f, 0.028f, chunkSize - 0.10f,
                      std::min(1.0f, topR * daylight + 0.018f),
                      std::min(1.0f, topG * daylight + 0.018f),
                      std::min(1.0f, topB * daylight + 0.018f));
        }
    }
}

void draw3DWorld() {
    const float aspect = gWidth / std::max(1.0f, gHeight);
    const float px = gPlayerX * 4.3f;
    const float pz = -gPlayerY * 4.0f;
    const float yaw = gController.camera.yaw;
    const float pitch = gController.camera.pitch;
    const float jumpHeight = gController.body.verticalPosition;
    const bool firstPerson = gViewMode == ViewMode::FirstPerson;
    const float horizontal = std::cos(pitch);
    const float vertical = std::sin(pitch);
    Vec3 eye{};
    Vec3 target{};
    if (firstPerson) {
        eye = {px, 1.48f + jumpHeight, pz};
        target = {px + std::sin(yaw) * horizontal, 1.48f + jumpHeight + vertical, pz + std::cos(yaw) * horizontal};
    } else {
        const float distance = std::clamp(gController.camera.distance, gController.camera.minDistance, gController.camera.maxDistance);
        target = {px, 0.72f + jumpHeight, pz};
        eye = {px - std::sin(yaw) * horizontal * distance, 0.72f + jumpHeight + vertical * distance + 1.0f, pz - std::cos(yaw) * horizontal * distance};
    }
    const Mat4 viewProjection = multiplyMatrix(perspectiveMatrix(1.03f, aspect, 0.05f, 60.0f), lookAtMatrix(eye, target));
    float daylight = 1.0f;
    switch (currentTimePhase()) {
        case TimePhase::Day: daylight = 1.0f; break;
        case TimePhase::Afternoon: daylight = 0.86f; break;
        case TimePhase::Evening: daylight = 0.52f; break;
        case TimePhase::Night: daylight = 0.22f; break;
    }
    if (currentWeather() == WeatherState::Thunderstorm) daylight *= 0.72f;
    gSceneLightLevel = daylight;
    switch (currentBiome()) {
        case Biome::Forest:
            gSceneFogR = 0.10f; gSceneFogG = 0.24f; gSceneFogB = 0.23f;
            break;
        case Biome::Sand:
            gSceneFogR = 0.34f; gSceneFogG = 0.20f; gSceneFogB = 0.12f;
            break;
        case Biome::Snow:
            gSceneFogR = 0.34f; gSceneFogG = 0.52f; gSceneFogB = 0.66f;
            break;
    }
    if (currentTimePhase() == TimePhase::Night) {
        gSceneFogR *= 0.42f; gSceneFogG *= 0.46f; gSceneFogB *= 0.72f;
    }
    gSceneFogAmount = 0.10f + 0.045f * static_cast<float>(effectiveGraphicsQuality());
    if (currentWeather() == WeatherState::Rain) gSceneFogAmount += 0.08f;
    if (currentWeather() == WeatherState::Thunderstorm) gSceneFogAmount += 0.14f;
    const float skyR = 0.028f + 0.085f * daylight;
    const float skyG = 0.060f + 0.21f * daylight;
    const float skyB = 0.12f + 0.38f * daylight;
    glClearColor(skyR, skyG, skyB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(g3DProgram);
    glUniform1f(g3DLightLevel, gSceneLightLevel);
    glUniform3f(g3DFogColor, gSceneFogR, gSceneFogG, gSceneFogB);
    glUniform1f(g3DFogAmount, gSceneFogAmount);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    draw3DSkyOrb(viewProjection, px, pz, yaw, daylight);
    drawPrototypeTerrainChunks(viewProjection, daylight);
    draw3DBox(viewProjection, -4.4f, 0.055f, 0.7f, 4.2f, 0.08f, 7.0f, 0.10f, 0.25f, 0.19f);
    draw3DBox(viewProjection, 0.0f, -0.01f, 0.7f, 4.3f, 0.08f, 7.0f, 0.54f, 0.31f, 0.12f);
    draw3DBox(viewProjection, 4.4f, 0.0f, 0.7f, 4.2f, 0.08f, 7.0f, 0.40f, 0.62f, 0.72f);
    draw3DBox(viewProjection, 0.4f, -0.015f, 1.0f, 0.90f, 0.04f, 7.0f, 0.15f, 0.38f, 0.39f);
    // A narrow reflective stream and warm path make the biome transition feel authored
    // rather than like three disconnected color planes.
    draw3DBox(viewProjection, 0.42f, 0.045f, 0.85f, 0.52f, 0.035f, 7.1f, 0.045f, 0.26f, 0.38f, 0.90f);
    draw3DBox(viewProjection, 0.42f, 0.071f, 0.85f, 0.34f, 0.012f, 7.0f, 0.22f, 0.68f, 0.76f, 0.48f);
    draw3DBox(viewProjection, 0.0f, 0.082f, 0.85f, 0.56f, 0.016f, 7.0f, 0.58f, 0.38f, 0.16f, 0.74f);
    draw3DTree(viewProjection, -5.0f, -1.0f, 1.25f, 0.02f);
    draw3DTree(viewProjection, -3.3f, 2.2f, 0.96f, 0.04f);
    draw3DTree(viewProjection, -5.5f, 3.3f, 1.48f, 0.01f);
    draw3DTree(viewProjection, -2.9f, -2.8f, 0.78f, 0.06f);
    draw3DTree(viewProjection, -5.8f, 0.4f, 0.74f, 0.08f);
    draw3DTree(viewProjection, 4.8f, 2.6f, 1.18f, 0.08f);
    draw3DTree(viewProjection, 5.5f, -0.8f, 0.90f, 0.12f);
    draw3DRock(viewProjection, -2.5f, 0.9f, 0.78f, 0.28f, 0.38f, 0.32f);
    draw3DRock(viewProjection, 2.2f, 2.2f, 0.90f, 0.56f, 0.38f, 0.20f);
    draw3DRock(viewProjection, 5.0f, -2.0f, 1.10f, 0.48f, 0.64f, 0.72f);
    if (effectiveGraphicsQuality() >= 1) {
        draw3DGrassTuft(viewProjection, -4.1f, -1.9f, 0.90f, 0.12f, 0.42f, 0.18f);
        draw3DGrassTuft(viewProjection, -2.0f, 1.9f, 0.68f, 0.16f, 0.50f, 0.20f);
        draw3DGrassTuft(viewProjection, 2.7f, -2.0f, 0.74f, 0.48f, 0.36f, 0.12f);
        draw3DGrassTuft(viewProjection, 4.0f, 1.1f, 0.86f, 0.56f, 0.78f, 0.80f);
    }
    draw3DVegetationDetails(viewProjection, daylight);
    draw3DBox(viewProjection, 3.8f, 0.10f, -1.2f, 1.2f, 0.20f, 0.8f, 0.78f, 0.48f, 0.16f);
    draw3DBox(viewProjection, 3.8f, 0.28f, -1.2f, 0.75f, 0.18f, 0.52f, 0.92f, 0.65f, 0.22f);
    drawTeleportationTower(viewProjection);
    if (gProgression.questStage == forest::rpg::QuestStage::DefeatWarden) {
        draw3DBox(viewProjection, -1.4f, 0.48f, 1.6f, 0.72f, 0.96f, 0.72f, 0.20f, 0.12f, 0.32f);
        draw3DBox(viewProjection, -1.4f, 1.18f, 1.6f, 0.88f, 0.18f, 0.88f, 0.58f, 0.28f, 0.77f);
    }
    draw3DMobs(viewProjection);
    for (const CoOpPeer& peer : gCoOpPeers) draw3DPeer(viewProjection, peer, static_cast<int>(&peer - gCoOpPeers));
    draw3DEmberling(viewProjection);
    draw3DWaterSurface(viewProjection);
    draw3DPlayer(viewProjection, firstPerson);
    draw3DWeather(viewProjection);
    glDisable(GL_CULL_FACE);
    if (gWorldMapVisible) draw3DMapOverlay();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[512]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        __android_log_print(ANDROID_LOG_ERROR, "ForestSlice", "Shader error: %s", log);
    }
    return shader;
}

void createProgram() {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    gProgram = glCreateProgram();
    glAttachShader(gProgram, vertex);
    glAttachShader(gProgram, fragment);
    glLinkProgram(gProgram);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    gPosition = glGetAttribLocation(gProgram, "aPosition");
    gColor = glGetUniformLocation(gProgram, "uColor");
    gScale = glGetUniformLocation(gProgram, "uScale");
    gOffset = glGetUniformLocation(gProgram, "uOffset");
}

void useColor(float r, float g, float b, float a = 1.0f) {
    glUniform4f(gColor, r, g, b, a);
}

void drawQuad(float x, float y, float width, float height, float r, float g, float b, float a = 1.0f) {
    const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    useColor(r, g, b, a);
    glUniform2f(gScale, width, height);
    glUniform2f(gOffset, x, y);
    glVertexAttribPointer(gPosition, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(gPosition);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawTriangle(float x, float y, float width, float height, float r, float g, float b, float a = 1.0f) {
    const GLfloat vertices[] = { 0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
    useColor(r, g, b, a);
    glUniform2f(gScale, width, height);
    glUniform2f(gOffset, x, y);
    glVertexAttribPointer(gPosition, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(gPosition);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void drawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f) {
    std::vector<GLfloat> vertices;
    vertices.reserve(44);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    constexpr int segments = 20;
    for (int i = 0; i <= segments; ++i) {
        const float angle = (static_cast<float>(i) / segments) * 2.0f * PI;
        vertices.push_back(std::cos(angle) * 0.5f);
        vertices.push_back(std::sin(angle) * 0.5f);
    }
    useColor(r, g, b, a);
    glUniform2f(gScale, radius * 2.0f, radius * 2.0f);
    glUniform2f(gOffset, x, y);
    glVertexAttribPointer(gPosition, 2, GL_FLOAT, GL_FALSE, 0, vertices.data());
    glEnableVertexAttribArray(gPosition);
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
}

void drawTree(float x, float y, float size) {
    drawCircle(x + 0.012f, y - 0.018f, size * 0.58f, 0.02f, 0.08f, 0.07f, 0.35f);
    drawQuad(x, y - size * 0.58f, size * 0.22f, size * 0.75f, 0.27f, 0.14f, 0.08f);
    drawCircle(x - size * 0.25f, y, size * 0.48f, 0.08f, 0.28f, 0.25f);
    drawCircle(x + size * 0.24f, y + size * 0.06f, size * 0.48f, 0.12f, 0.38f, 0.28f);
    drawCircle(x, y + size * 0.24f, size * 0.52f, 0.18f, 0.48f, 0.31f);
    drawCircle(x - size * 0.12f, y + size * 0.33f, size * 0.16f, 0.42f, 0.68f, 0.42f, 0.75f);
}

void drawRock(float x, float y, float size) {
    drawTriangle(x, y, size, size * 0.78f, 0.27f, 0.33f, 0.34f);
    drawTriangle(x, y + size * 0.08f, size * 0.65f, size * 0.5f, 0.52f, 0.58f, 0.57f);
}

void drawCloud(float x, float y, float scale, float alpha = 0.34f) {
    drawCircle(x - 0.055f * scale, y, 0.055f * scale, 0.90f, 0.97f, 0.98f, alpha);
    drawCircle(x, y + 0.020f * scale, 0.072f * scale, 0.96f, 0.99f, 1.0f, alpha + 0.06f);
    drawCircle(x + 0.062f * scale, y, 0.048f * scale, 0.88f, 0.95f, 0.98f, alpha);
}

void drawMountain(float x, float y, float width, float height, float r, float g, float b) {
    drawTriangle(x, y, width, height, r, g, b, 0.92f);
    drawTriangle(x - width * 0.13f, y + height * 0.05f, width * 0.42f, height * 0.60f,
                 std::min(1.0f, r + 0.12f), std::min(1.0f, g + 0.12f), std::min(1.0f, b + 0.12f), 0.88f);
}

void drawLantern(float x, float y, float scale, bool lit) {
    drawQuad(x, y - 0.028f * scale, 0.010f * scale, 0.085f * scale, 0.12f, 0.08f, 0.06f);
    drawQuad(x, y + 0.022f * scale, 0.036f * scale, 0.044f * scale, 0.08f, 0.06f, 0.04f);
    if (lit) {
        drawCircle(x, y + 0.022f * scale, 0.060f * scale, 1.0f, 0.62f, 0.18f, 0.12f);
        drawCircle(x, y + 0.022f * scale, 0.026f * scale, 1.0f, 0.88f, 0.52f, 0.92f);
    } else {
        drawCircle(x, y + 0.022f * scale, 0.018f * scale, 0.32f, 0.24f, 0.15f, 0.95f);
    }
}

void drawNightStars() {
    constexpr float stars[][3] = {
        {-0.86f, 0.74f, 0.008f}, {-0.67f, 0.68f, 0.006f}, {-0.46f, 0.78f, 0.009f},
        {-0.23f, 0.70f, 0.005f}, {-0.04f, 0.82f, 0.007f}, {0.18f, 0.73f, 0.005f},
        {0.36f, 0.84f, 0.008f}, {0.57f, 0.75f, 0.006f}, {0.82f, 0.80f, 0.009f},
        {-0.79f, 0.49f, 0.005f}, {-0.55f, 0.56f, 0.007f}, {-0.31f, 0.48f, 0.005f},
        {-0.10f, 0.57f, 0.008f}, {0.11f, 0.50f, 0.005f}, {0.43f, 0.54f, 0.007f},
        {0.67f, 0.46f, 0.005f}
    };
    const int visibleStars = std::min(16, 5 + effectiveGraphicsQuality() * 3);
    for (int i = 0; i < visibleStars; ++i) {
        const auto& star = stars[i];
        drawCircle(star[0], star[1], star[2], 0.82f, 0.93f, 1.0f, 0.80f);
    }
}

void drawRain(float intensity) {
    if (intensity <= 0.0f) return;
    const int rainDrops = 8 + effectiveGraphicsQuality() * 5;
    for (int i = 0; i < rainDrops; ++i) {
        const float seed = static_cast<float>(i) * 0.173f;
        const float x = -0.96f + std::fmod(seed + gTime * (0.07f + 0.004f * static_cast<float>(i)), 1.92f);
        const float y = 0.78f - std::fmod(seed * 2.0f + gTime * (0.21f + 0.009f * static_cast<float>(i)), 1.58f);
        const float length = 0.08f + 0.025f * intensity;
        drawQuad(x, y, 0.0055f, length, 0.48f, 0.78f, 0.92f, 0.34f + intensity * 0.28f);
        drawQuad(x + 0.004f, y - length * 0.35f, 0.0025f, length * 0.50f, 0.84f, 0.95f, 1.0f, 0.20f + intensity * 0.16f);
    }
    drawCircle(-0.46f, -0.35f, 0.045f, 0.28f, 0.72f, 0.78f, 0.12f * intensity);
    drawCircle(-0.46f, -0.35f, 0.024f, 0.62f, 0.90f, 0.94f, 0.18f * intensity);
    drawCircle(0.08f, -0.31f, 0.038f, 0.32f, 0.70f, 0.76f, 0.12f * intensity);
}

void drawLightning(float intensity) {
    if (intensity <= 0.0f) return;
    drawQuad(0.0f, 0.18f, 2.0f, 1.42f, 0.72f, 0.86f, 1.0f, intensity * 0.20f);
    drawQuad(0.48f, 0.47f, 0.014f, 0.20f, 0.88f, 0.96f, 1.0f, intensity * 0.92f);
    drawQuad(0.43f, 0.37f, 0.014f, 0.16f, 0.88f, 0.96f, 1.0f, intensity * 0.92f);
    drawQuad(0.53f, 0.28f, 0.014f, 0.16f, 0.88f, 0.96f, 1.0f);
}

void drawCampfire(float weatherFactor) {
    const float flicker = 0.88f + 0.12f * std::sin(gTime * 10.0f);
    const float intensity = std::max(0.30f, weatherFactor) * flicker;
    drawCircle(-0.63f, -0.36f, 0.14f, 1.0f, 0.36f, 0.06f, 0.055f * intensity);
    drawCircle(-0.63f, -0.36f, 0.085f, 1.0f, 0.52f, 0.08f, 0.12f * intensity);
    drawQuad(-0.63f, -0.36f, 0.16f, 0.024f, 0.20f, 0.10f, 0.05f, 1.0f);
    drawQuad(-0.63f, -0.36f, 0.024f, 0.13f, 0.20f, 0.10f, 0.05f, 1.0f);
    drawTriangle(-0.63f, -0.26f, 0.10f, 0.18f, 0.92f, 0.24f, 0.05f, 0.90f);
    drawTriangle(-0.63f, -0.24f, 0.058f, 0.12f, 1.0f, 0.70f, 0.18f, 0.96f);
    drawCircle(-0.63f, -0.25f, 0.020f, 1.0f, 0.94f, 0.58f, 0.96f);
    drawCircle(-0.61f, -0.14f, 0.007f, 1.0f, 0.72f, 0.24f, 0.72f * intensity);
    drawCircle(-0.67f, -0.18f, 0.005f, 1.0f, 0.82f, 0.36f, 0.68f * intensity);
}

void drawHeartfire(float x, float y, bool emberKitCrafted) {
    const float pulse = 0.82f + 0.18f * std::sin(gTime * 6.0f);
    const float glow = emberKitCrafted ? 0.22f * pulse : 0.08f * pulse;
    drawCircle(x, y, 0.12f, emberKitCrafted ? 0.62f : 0.30f, 0.16f, emberKitCrafted ? 0.86f : 0.24f, glow);
    drawQuad(x, y - 0.026f, 0.14f, 0.022f, 0.12f, 0.08f, 0.05f, 1.0f);
    drawTriangle(x, y + 0.055f, 0.052f, 0.12f, emberKitCrafted ? 0.58f : 0.24f, 0.12f, emberKitCrafted ? 0.92f : 0.28f, 0.94f);
    drawCircle(x, y + 0.066f, 0.018f, emberKitCrafted ? 0.94f : 0.42f, 0.72f, 1.0f, emberKitCrafted ? 0.94f : 0.60f);
}

void drawResourceCache(float x, float y, float r, float g, float b) {
    drawCircle(x, y - 0.018f, 0.038f, 0.025f, 0.045f, 0.035f, 0.45f);
    drawQuad(x, y, 0.050f, 0.070f, r, g, b, 0.96f);
    drawCircle(x - 0.018f, y + 0.042f, 0.022f, r * 1.18f, g * 1.18f, b * 1.18f, 0.90f);
    drawCircle(x + 0.018f, y + 0.044f, 0.020f, r * 0.86f, g * 0.86f, b * 0.86f, 0.90f);
}

void drawForestPath() {
    drawTriangle(-0.42f, -0.43f, 0.20f, 0.22f, 0.50f, 0.34f, 0.18f, 0.50f);
    drawTriangle(-0.46f, -0.25f, 0.12f, 0.26f, 0.56f, 0.39f, 0.20f, 0.36f);
    drawCircle(-0.43f, -0.12f, 0.030f, 0.72f, 0.55f, 0.30f, 0.28f);
}

void drawSandMarketDetails() {
    drawQuad(0.02f, 0.04f, 0.22f, 0.018f, 0.22f, 0.10f, 0.05f, 0.75f);
    drawTriangle(0.02f, 0.10f, 0.22f, 0.11f, 0.85f, 0.35f, 0.10f, 0.92f);
    drawQuad(0.02f, -0.05f, 0.014f, 0.16f, 0.25f, 0.12f, 0.06f);
    drawQuad(0.14f, -0.05f, 0.014f, 0.16f, 0.25f, 0.12f, 0.06f);
    drawCircle(-0.18f, -0.30f, 0.038f, 0.77f, 0.52f, 0.20f, 0.86f);
    drawCircle(-0.18f, -0.30f, 0.018f, 0.98f, 0.80f, 0.34f, 0.95f);
}

void drawSnowCrystalCluster(float x, float y, float scale) {
    drawTriangle(x - 0.030f * scale, y, 0.052f * scale, 0.16f * scale, 0.36f, 0.70f, 0.84f, 0.80f);
    drawTriangle(x + 0.026f * scale, y + 0.005f * scale, 0.040f * scale, 0.12f * scale, 0.58f, 0.86f, 0.96f, 0.82f);
    drawCircle(x, y - 0.058f * scale, 0.016f * scale, 0.74f, 0.94f, 1.0f, 0.75f);
}

void drawPlayer() {
    // The hero is built from deliberately flat color planes. Drawing the ink silhouette
    // first and the lit/shadow planes second gives the 2D prototype a mobile-safe cel look.
    const float bob = std::sin(gTime * 4.0f) * 0.004f;
    const float px = gPlayerX;
    const float py = gPlayerY + bob;
    const float hairSway = gController.secondaryMotion.hairOffset.x;
    const float hairLift = gController.secondaryMotion.hairOffset.y;
    const float clothSway = gController.secondaryMotion.clothOffset.x;
    const float wetness = gController.secondaryMotion.wetness;
    const float inkR = 0.035f;
    const float inkG = 0.022f;
    const float inkB = 0.055f;

    // Ground contact and boots.
    drawCircle(px + 0.014f, py - 0.038f, 0.073f, inkR, inkG, inkB, 0.55f);
    drawQuad(px - 0.028f, py - 0.025f, 0.040f, 0.086f, inkR, inkG, inkB);
    drawQuad(px + 0.028f, py - 0.025f, 0.040f, 0.086f, inkR, inkG, inkB);
    drawQuad(px - 0.028f, py - 0.024f, 0.024f, 0.068f, 0.15f, 0.10f, 0.22f);
    drawQuad(px + 0.028f, py - 0.024f, 0.024f, 0.068f, 0.23f, 0.14f, 0.28f);
    drawQuad(px - 0.035f, py - 0.066f, 0.052f, 0.018f, 0.08f, 0.05f, 0.13f);
    drawQuad(px + 0.035f, py - 0.066f, 0.052f, 0.018f, 0.08f, 0.05f, 0.13f);

    // Tunic silhouette, then a cool shadow plane and a single warm key plane.
    drawTriangle(px + clothSway * 0.40f, py + 0.040f, 0.154f, 0.215f, inkR, inkG, inkB);
    drawTriangle(px + clothSway * 0.40f, py + 0.043f, 0.128f, 0.190f, 0.43f, 0.12f, 0.29f);
    drawTriangle(px - 0.029f + clothSway * 0.24f, py + 0.052f, 0.058f, 0.158f, 0.25f, 0.07f, 0.20f);
    drawTriangle(px + 0.018f + clothSway * 0.52f, py + 0.073f, 0.044f, 0.112f, 0.68f, 0.18f, 0.34f);
    drawQuad(px + clothSway * 0.55f, py + 0.004f, 0.114f, 0.024f, inkR, inkG, inkB);
    drawQuad(px + clothSway * 0.55f, py + 0.009f, 0.096f, 0.012f, 0.06f, 0.38f, 0.38f);
    drawQuad(px + 0.024f + clothSway * 0.75f, py + 0.040f, 0.022f, 0.096f, 0.10f, 0.55f, 0.51f);

    // Arms: clean dark contour, colored sleeve, then small skin hands.
    drawQuad(px - 0.066f, py + 0.064f, 0.042f, 0.112f, inkR, inkG, inkB);
    drawQuad(px - 0.066f, py + 0.064f, 0.026f, 0.092f, 0.36f, 0.10f, 0.25f);
    drawCircle(px - 0.066f, py + 0.002f, 0.021f, 0.58f, 0.31f, 0.24f);
    drawQuad(px + 0.066f, py + 0.064f, 0.042f, 0.112f, inkR, inkG, inkB);
    drawQuad(px + 0.066f, py + 0.064f, 0.026f, 0.092f, 0.62f, 0.16f, 0.31f);
    drawCircle(px + 0.067f, py + 0.002f, 0.021f, 0.88f, 0.53f, 0.36f);

    // Neck, scarf and face with a hard diagonal light plane.
    drawQuad(px, py + 0.105f, 0.042f, 0.050f, inkR, inkG, inkB);
    drawQuad(px, py + 0.109f, 0.027f, 0.040f, 0.73f, 0.36f, 0.27f);
    drawQuad(px, py + 0.092f, 0.070f, 0.022f, inkR, inkG, inkB);
    drawQuad(px, py + 0.096f, 0.053f, 0.012f, 0.80f, 0.18f, 0.25f);
    drawCircle(px, py + 0.142f, 0.079f, inkR, inkG, inkB);
    drawCircle(px, py + 0.143f, 0.066f, 0.67f, 0.32f, 0.24f);
    drawTriangle(px + 0.018f, py + 0.151f, 0.068f, 0.088f, 0.93f, 0.62f, 0.42f);
    drawTriangle(px - 0.041f, py + 0.134f, 0.042f, 0.062f, 0.48f, 0.19f, 0.18f);

    // Spiky violet hair silhouette, highlights and bangs.
    drawCircle(px + 0.006f + hairSway, py + 0.177f + hairLift, 0.079f, inkR, inkG, inkB);
    drawCircle(px + 0.010f + hairSway, py + 0.181f + hairLift, 0.068f, 0.12f, 0.06f, 0.22f);
    drawTriangle(px - 0.050f + hairSway * 1.08f, py + 0.215f + hairLift, 0.048f, 0.082f, inkR, inkG, inkB);
    drawTriangle(px - 0.050f + hairSway * 1.08f, py + 0.215f + hairLift, 0.030f, 0.061f, 0.17f, 0.08f, 0.30f);
    drawTriangle(px - 0.014f + hairSway * 1.12f, py + 0.226f + hairLift, 0.046f, 0.092f, inkR, inkG, inkB);
    drawTriangle(px - 0.014f + hairSway * 1.12f, py + 0.226f + hairLift, 0.027f, 0.070f, 0.20f, 0.10f, 0.35f);
    drawTriangle(px + 0.030f + hairSway * 1.16f, py + 0.220f + hairLift, 0.052f, 0.082f, inkR, inkG, inkB);
    drawTriangle(px + 0.030f + hairSway * 1.16f, py + 0.220f + hairLift, 0.032f, 0.061f, 0.17f, 0.08f, 0.29f);
    drawTriangle(px + 0.035f + hairSway * 1.22f, py + 0.191f + hairLift, 0.042f, 0.075f, 0.29f, 0.15f, 0.43f);
    drawTriangle(px - 0.021f + hairSway * 1.28f, py + 0.170f + hairLift, 0.035f, 0.055f, 0.09f, 0.04f, 0.17f);
    if (wetness > 0.05f) {
        drawCircle(px + 0.052f + hairSway, py + 0.170f + hairLift, 0.006f + wetness * 0.004f, 0.40f, 0.75f, 0.86f, wetness * 0.75f);
    }

    // Eyes and mouth are kept high-contrast so the face reads at phone scale.
    drawQuad(px - 0.029f, py + 0.143f, 0.018f, 0.010f, 0.035f, 0.025f, 0.045f);
    drawQuad(px + 0.029f, py + 0.143f, 0.018f, 0.010f, 0.035f, 0.025f, 0.045f);
    drawCircle(px - 0.028f, py + 0.144f, 0.0035f, 0.98f, 0.78f, 0.38f);
    drawCircle(px + 0.030f, py + 0.144f, 0.0035f, 0.98f, 0.78f, 0.38f);
    drawQuad(px + 0.002f, py + 0.122f, 0.018f, 0.005f, 0.30f, 0.10f, 0.12f);

    // Short blade: dark contour, steel face, and gold guard.
    drawQuad(px + 0.094f, py + 0.030f, 0.024f, 0.170f, inkR, inkG, inkB);
    drawQuad(px + 0.094f, py + 0.034f, 0.010f, 0.144f, 0.78f, 0.83f, 0.84f);
    drawQuad(px + 0.094f, py + 0.103f, 0.054f, 0.012f, 0.98f, 0.72f, 0.24f);
    drawQuad(px + 0.094f, py - 0.062f, 0.024f, 0.032f, inkR, inkG, inkB);
    drawQuad(px + 0.094f, py - 0.062f, 0.010f, 0.025f, 0.78f, 0.47f, 0.17f);

    if (gAttackPulse > 0) {
        drawCircle(px + 0.145f, py + 0.035f, 0.062f, 0.97f, 0.85f, 0.42f, 0.70f);
        --gAttackPulse;
    }
    if (gDodgePulse > 0) {
        drawCircle(px, py + 0.035f, 0.13f, 0.40f, 0.85f, 0.95f, 0.22f);
        --gDodgePulse;
    }
}

void drawAnimal(float x, float y, float tint) {
    drawCircle(x + 0.01f, y - 0.02f, 0.045f, 0.01f, 0.05f, 0.04f, 0.3f);
    drawQuad(x, y, 0.14f, 0.09f, 0.32f + tint, 0.25f, 0.19f);
    drawCircle(x + 0.065f, y + 0.035f, 0.045f, 0.40f + tint, 0.32f, 0.22f);
    drawTriangle(x + 0.045f, y + 0.083f, 0.035f, 0.045f, 0.48f + tint, 0.38f, 0.27f);
    drawTriangle(x + 0.084f, y + 0.083f, 0.035f, 0.045f, 0.48f + tint, 0.38f, 0.27f);
    drawCircle(x + 0.09f, y + 0.045f, 0.007f, 0.95f, 0.85f, 0.47f);
}

void awardExperience(int amount) {
    const int previousLevel = gProgression.level;
    gProgression.awardExperience(amount);
    if (gProgression.level > previousLevel) gLevelPulse = 120;
}

void drawPerson(float x, float y, float skinR, float clothR, float clothG, float clothB) {
    const float inkR = 0.035f;
    const float inkG = 0.022f;
    const float inkB = 0.055f;
    drawCircle(x + 0.008f, y - 0.020f, 0.040f, inkR, inkG, inkB, 0.45f);
    drawQuad(x, y + 0.018f, 0.048f, 0.092f, inkR, inkG, inkB);
    drawQuad(x, y + 0.020f, 0.031f, 0.074f, clothR, clothG, clothB);
    drawQuad(x - 0.028f, y + 0.028f, 0.016f, 0.064f, inkR, inkG, inkB);
    drawQuad(x + 0.028f, y + 0.028f, 0.016f, 0.064f, inkR, inkG, inkB);
    drawCircle(x, y + 0.086f, 0.032f, inkR, inkG, inkB);
    drawCircle(x, y + 0.086f, 0.025f, skinR, skinR * 0.64f, skinR * 0.46f);
    drawTriangle(x + 0.010f, y + 0.110f, 0.037f, 0.042f, clothR * 0.55f, clothG * 0.55f, clothB * 0.55f);
    drawQuad(x - 0.014f, y - 0.025f, 0.015f, 0.038f, inkR, inkG, inkB);
    drawQuad(x + 0.014f, y - 0.025f, 0.015f, 0.038f, inkR, inkG, inkB);
}

void drawFarmPlot(float x, float y, float scale) {
    drawQuad(x, y, 0.22f * scale, 0.09f * scale, 0.12f, 0.08f, 0.05f);
    drawQuad(x, y + 0.014f * scale, 0.18f * scale, 0.038f * scale, 0.33f, 0.18f, 0.08f);
    for (int i = -1; i <= 1; ++i) {
        const float cropX = x + static_cast<float>(i) * 0.052f * scale;
        drawQuad(cropX, y + 0.046f * scale, 0.012f * scale, 0.052f * scale, 0.10f, 0.30f, 0.12f);
        drawCircle(cropX + 0.010f * scale, y + 0.074f * scale, 0.018f * scale, 0.32f, 0.66f, 0.22f);
    }
}

void drawForestVillage() {
    drawQuad(-0.68f, -0.02f, 0.24f, 0.18f, 0.18f, 0.11f, 0.09f);
    drawTriangle(-0.68f, 0.12f, 0.30f, 0.18f, 0.40f, 0.13f, 0.16f);
    drawQuad(-0.68f, -0.06f, 0.050f, 0.10f, 0.42f, 0.23f, 0.12f);
    drawFarmPlot(-0.72f, -0.30f, 1.0f);
    drawFarmPlot(-0.48f, -0.35f, 0.82f);
    drawPerson(-0.80f, -0.22f, 0.83f, 0.17f, 0.40f, 0.24f);
    drawPerson(-0.42f, -0.24f, 0.72f, 0.42f, 0.18f, 0.12f);
}

void drawSandSettlement() {
    drawQuad(-0.03f, -0.02f, 0.23f, 0.20f, 0.54f, 0.28f, 0.13f);
    drawTriangle(-0.03f, 0.16f, 0.29f, 0.20f, 0.78f, 0.43f, 0.17f);
    drawQuad(-0.03f, -0.07f, 0.052f, 0.10f, 0.18f, 0.10f, 0.06f);
    drawQuad(0.18f, -0.18f, 0.14f, 0.12f, 0.60f, 0.33f, 0.16f);
    drawTriangle(0.18f, -0.08f, 0.18f, 0.14f, 0.80f, 0.50f, 0.18f);
    drawQuad(-0.23f, -0.28f, 0.020f, 0.16f, 0.16f, 0.32f, 0.15f);
    drawTriangle(-0.23f, -0.17f, 0.090f, 0.090f, 0.20f, 0.52f, 0.20f);
    drawPerson(-0.11f, -0.25f, 0.76f, 0.52f, 0.26f, 0.10f);
    drawPerson(0.20f, -0.28f, 0.88f, 0.16f, 0.28f, 0.46f);
}

void drawForestWarden(float x, float y, float scale, bool combatTarget) {
    if (combatTarget && gEnemyDefeatTimer > 0.0f) {
        const float fade = std::clamp(gEnemyDefeatTimer / 1.5f, 0.0f, 1.0f);
        drawCircle(x, y + 0.035f * scale, 0.18f * scale, 0.42f, 0.72f, 0.88f, 0.12f * fade);
        return;
    }
    const float flash = combatTarget && gEnemyHitFlash > 0.0f ? 0.32f : 0.0f;
    const float inkR = 0.025f;
    const float inkG = 0.035f;
    const float inkB = 0.070f;
    const float bodyR = 0.24f + flash;
    const float bodyG = 0.18f + flash * 0.35f;
    const float bodyB = 0.34f + flash * 0.25f;

    // Root-bound warden silhouette with violet lit planes and hard midnight shadows.
    drawCircle(x + 0.018f * scale, y - 0.055f * scale, 0.14f * scale, inkR, inkG, inkB, 0.55f);
    drawQuad(x - 0.070f * scale, y - 0.020f * scale, 0.050f * scale, 0.15f * scale, inkR, inkG, inkB);
    drawQuad(x + 0.070f * scale, y - 0.020f * scale, 0.050f * scale, 0.15f * scale, inkR, inkG, inkB);
    drawQuad(x - 0.070f * scale, y - 0.018f * scale, 0.030f * scale, 0.12f * scale, bodyR * 0.60f, bodyG * 0.60f, bodyB * 0.68f);
    drawQuad(x + 0.070f * scale, y - 0.018f * scale, 0.030f * scale, 0.12f * scale, bodyR, bodyG, bodyB);
    drawCircle(x, y + 0.018f * scale, 0.115f * scale, inkR, inkG, inkB);
    drawCircle(x, y + 0.020f * scale, 0.096f * scale, bodyR, bodyG, bodyB);
    drawTriangle(x - 0.055f * scale, y + 0.125f * scale, 0.085f * scale, 0.13f * scale, inkR, inkG, inkB);
    drawTriangle(x + 0.055f * scale, y + 0.125f * scale, 0.085f * scale, 0.13f * scale, inkR, inkG, inkB);
    drawTriangle(x - 0.055f * scale, y + 0.123f * scale, 0.052f * scale, 0.095f * scale, bodyR * 0.70f, bodyG * 0.70f, bodyB * 0.78f);
    drawTriangle(x + 0.055f * scale, y + 0.123f * scale, 0.052f * scale, 0.095f * scale, bodyR, bodyG, bodyB);
    drawTriangle(x + 0.018f * scale, y + 0.030f * scale, 0.085f * scale, 0.090f * scale, 0.70f + flash, 0.86f, 0.94f);
    drawTriangle(x - 0.042f * scale, y + 0.040f * scale, 0.060f * scale, 0.105f * scale, bodyR * 0.65f, bodyG * 0.62f, bodyB * 0.72f);
    drawCircle(x - 0.035f * scale, y + 0.070f * scale, 0.014f * scale, 1.0f, 0.72f, 0.22f);
    drawCircle(x + 0.035f * scale, y + 0.070f * scale, 0.014f * scale, 1.0f, 0.72f, 0.22f);
    drawQuad(x, y - 0.005f * scale, 0.054f * scale, 0.012f * scale, 0.06f, 0.10f, 0.16f);
    drawTriangle(x, y - 0.075f * scale, 0.070f * scale, 0.075f * scale, 0.50f, 0.72f, 0.84f);

    if (combatTarget) {
        const float healthRatio = std::clamp(gEnemyHealth / kForestWardenMaxHealth, 0.0f, 1.0f);
        drawQuad(x, y + 0.235f * scale, 0.30f * scale, 0.020f * scale, 0.025f, 0.045f, 0.08f, 0.92f);
        drawQuad(x - 0.15f * scale + 0.15f * scale * healthRatio, y + 0.235f * scale,
                 0.30f * scale * healthRatio, 0.013f * scale, 0.28f, 0.84f, 0.98f, 0.98f);
    }
}


void updateMobs(float deltaSeconds) {
    for (MobState& mob : gMobs) {
        const forest::mobs::MobProfile& mobProfile = forest::mobs::profile(mob.type);
        mob.hitFlash = std::max(0.0f, mob.hitFlash - deltaSeconds);
        mob.attackCooldown = std::max(0.0f, mob.attackCooldown - deltaSeconds);
        mob.defeatTimer = std::max(0.0f, mob.defeatTimer - deltaSeconds);
        if (mob.health <= 0.0f) continue;

        const float dx = gPlayerX - mob.position.x;
        const float dy = gPlayerY - mob.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > mobProfile.attackRange * 0.82f && distance > 0.001f) {
            const float travel = mobProfile.moveSpeed * deltaSeconds;
            mob.position.x += dx / distance * travel;
            mob.position.y += dy / distance * travel;
            mob.position.x = std::clamp(mob.position.x, kSimulationMinX + 0.03f, kSimulationMaxX - 0.03f);
            mob.position.y = std::clamp(mob.position.y, kSimulationMinY + 0.03f, kSimulationMaxY - 0.03f);
        } else if (!gAuthoritativeOnline && distance <= mobProfile.attackRange && mob.attackCooldown <= 0.0f) {
            gController.health = std::max(0.0f, gController.health - mobProfile.attackDamage / 100.0f);
            mob.attackCooldown = mobProfile.attackCooldown;
            gQuestPulse = std::max(gQuestPulse, 12);
        }
    }
}

void applyMobDamage(const forest::combat::Hitbox& hitbox, const forest::physics::Vec2& facing) {
    const forest::physics::Aabb attackBox{
        gController.body.position + hitbox.offset,
        hitbox.halfExtents
    };
    for (MobState& mob : gMobs) {
        if (mob.health <= 0.0f) continue;
        const forest::mobs::MobProfile& mobProfile = forest::mobs::profile(mob.type);
        const float hitRadius = 0.075f * mobProfile.scale;
        const forest::physics::Aabb mobBox{mob.position, {hitRadius, 0.10f * mobProfile.scale}};
        if (!forest::combat::intersects(attackBox, mobBox)) continue;
        mob.health = std::max(0.0f, mob.health - hitbox.damage * 100.0f);
        mob.hitFlash = 0.15f;
        gHitRegistered = true;
        gCombat.confirmHit();
        awardExperience(12);
        if (mob.health <= 0.0f) {
            mob.defeatTimer = 1.5f;
            gQuestPulse = 120;
        }
        break;
    }
    (void)facing;
}

void simulatePhysicsStep() {
    applySynchronizedWorldTime();
    gTime += kPhysicsStep;
    updateCalendar();
    if (gGyroEnabled) gController.camera.orbit(gGyroX * 0.012f, gGyroY * 0.008f);
    // Android screen and world handedness are opposite on the horizontal axis in
    // this camera setup. Mirror both axes exactly once here so thumb movement
    // produces intuitive visible traversal regardless of joystick placement.
    const forest::controller::InputFrame input{-gMoveX, -gMoveY, gController.camera.yaw, gSprintHeld};
    gController.tick(input, kPhysicsStep, gObstacles, static_cast<int>(sizeof(gObstacles) / sizeof(gObstacles[0])),
                      gWaterVolumes, static_cast<int>(sizeof(gWaterVolumes) / sizeof(gWaterVolumes[0])));
    updateMobs(kPhysicsStep);
    gCombat.tick(kPhysicsStep);
    if (gJumpBufferSeconds > 0.0f) {
        gJumpBufferSeconds = std::max(0.0f, gJumpBufferSeconds - kPhysicsStep);
        if (gController.jump()) gJumpBufferSeconds = 0.0f;
    }
    gEnemyHitFlash = std::max(0.0f, gEnemyHitFlash - kPhysicsStep);
    gEnemyDefeatTimer = std::max(0.0f, gEnemyDefeatTimer - kPhysicsStep);
    gTowerGlow = std::max(0.0f, gTowerGlow - kPhysicsStep);
    gTowerCooldown = std::max(0.0f, gTowerCooldown - kPhysicsStep);
    const forest::combat::CombatEvent combatEvent = gCombat.consumeEvent();
    if (combatEvent.attackStarted) {
        gAttackPulse = combatEvent.heavyAttack ? 16 : 6 + combatEvent.comboIndex * 2;
        gHitRegistered = false;
    }
    if (gCombat.isHitActive() && !gHitRegistered && !gAuthoritativeOnline) {
        const forest::physics::Vec2 facing{
            std::cos(gController.facingRadians),
            std::sin(gController.facingRadians)
        };
        const forest::combat::Hitbox hitbox = gCombat.currentHitbox(facing);
        applyMobDamage(hitbox, facing);
        if (!gHitRegistered) {
            const forest::physics::Aabb attackBox{
                gController.body.position + hitbox.offset,
                hitbox.halfExtents
            };
            const forest::physics::Aabb enemyBox{{gEnemyX, gEnemyY}, {0.09f, 0.10f}};
            if (forest::combat::intersects(attackBox, enemyBox)) {
                gEnemyHealth = std::max(0.0f, gEnemyHealth - hitbox.damage * 100.0f);
                gEnemyHitFlash = 0.12f;
                gHitRegistered = true;
                gCombat.confirmHit();
                awardExperience(10);
                if (gEnemyHealth <= 0.0f && !gProgression.wardenDefeated) {
                    gProgression.recordWardenDefeat();
                    gEnemyDefeatTimer = 1.5f;
                    gQuestPulse = 150;
                }
            }
        }
    }
    gPlayerX = gController.body.position.x;
    gPlayerY = gController.body.position.y;
    forest::rpg::updateEmberling(gEmberling, gPlayerX, gPlayerY, kPhysicsStep, currentWeather() == WeatherState::Thunderstorm);
    gHunger = std::max(0.0f, gHunger - kPhysicsStep * 0.001f);
    if (gHunger < 0.20f) gController.health = std::max(0.0f, gController.health - kPhysicsStep * 0.004f);
    gLevelPulse = std::max(0, gLevelPulse - 1);
    gQuestPulse = std::max(0, gQuestPulse - 1);
}

void drawWorld() {
    if (gViewMode == ViewMode::FirstPerson || gViewMode == ViewMode::ThirdPerson) {
        draw3DWorld();
        return;
    }
    const float phaseTime = std::fmod(std::max(0.0f, gTime), kDayCycleSeconds);
    const float phaseStart = phaseTime < kDayPhaseSeconds ? 0.0f
        : phaseTime < kDayPhaseSeconds + kAfternoonPhaseSeconds ? kDayPhaseSeconds
        : phaseTime < kDayPhaseSeconds + kAfternoonPhaseSeconds + kEveningPhaseSeconds ? kDayPhaseSeconds + kAfternoonPhaseSeconds
        : kDayPhaseSeconds + kAfternoonPhaseSeconds + kEveningPhaseSeconds;
    const float phaseProgress = std::clamp((phaseTime - phaseStart) / currentTimePhaseDuration(), 0.0f, 1.0f);
    float clearR = 0.025f;
    float clearG = 0.09f;
    float clearB = 0.105f;
    float tintR = 0.0f;
    float tintG = 0.0f;
    float tintB = 0.0f;
    float tintAlpha = 0.0f;
    switch (currentTimePhase()) {
        case TimePhase::Day:
            clearR = 0.045f; clearG = 0.16f; clearB = 0.18f;
            break;
        case TimePhase::Afternoon:
            clearR = 0.20f; clearG = 0.11f; clearB = 0.07f;
            tintR = 0.95f; tintG = 0.42f; tintB = 0.12f; tintAlpha = 0.10f + phaseProgress * 0.05f;
            break;
        case TimePhase::Evening:
            clearR = 0.18f; clearG = 0.055f; clearB = 0.13f;
            tintR = 0.58f; tintG = 0.08f; tintB = 0.24f; tintAlpha = 0.16f + phaseProgress * 0.08f;
            break;
        case TimePhase::Night:
            clearR = 0.010f; clearG = 0.025f; clearB = 0.075f;
            tintR = 0.015f; tintG = 0.04f; tintB = 0.18f; tintAlpha = 0.34f + phaseProgress * 0.08f;
            break;
    }
    glClearColor(clearR, clearG, clearB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(gProgram);

    // Three side-by-side regions keep the prototype traversable while making the
    // biome contrast immediately readable on a phone screen.
    drawQuad(-0.60f, 0.18f, 0.60f, 1.42f, 0.035f, 0.17f, 0.14f);
    drawQuad(0.00f, 0.18f, 0.60f, 1.42f, 0.34f, 0.20f, 0.09f);
    drawQuad(0.60f, 0.18f, 0.60f, 1.42f, 0.70f, 0.82f, 0.88f);
    drawQuad(-0.60f, -0.53f, 0.60f, 0.58f, 0.075f, 0.28f, 0.17f);
    drawQuad(0.00f, -0.53f, 0.60f, 0.58f, 0.78f, 0.48f, 0.20f);
    drawQuad(0.60f, -0.53f, 0.60f, 0.58f, 0.82f, 0.90f, 0.94f);
    drawQuad(-0.30f, 0.05f, 0.012f, 1.10f, 0.07f, 0.10f, 0.09f, 0.70f);
    drawQuad(0.30f, 0.05f, 0.012f, 1.10f, 0.20f, 0.25f, 0.28f, 0.70f);

    // Layered distance shapes add depth while preserving the lightweight GLES path.
    drawCloud(-0.70f, 0.56f, 0.95f, 0.22f);
    drawCloud(-0.06f, 0.66f, 0.70f, 0.18f);
    drawCloud(0.56f, 0.62f, 0.82f, 0.20f);
    drawMountain(-0.74f, 0.40f, 0.46f, 0.34f, 0.08f, 0.25f, 0.22f);
    drawMountain(-0.42f, 0.39f, 0.36f, 0.28f, 0.06f, 0.20f, 0.18f);
    drawMountain(0.00f, 0.40f, 0.44f, 0.32f, 0.42f, 0.23f, 0.13f);
    drawMountain(0.25f, 0.38f, 0.34f, 0.28f, 0.50f, 0.28f, 0.15f);
    drawMountain(0.52f, 0.43f, 0.42f, 0.40f, 0.44f, 0.68f, 0.82f);
    drawMountain(0.78f, 0.45f, 0.38f, 0.44f, 0.52f, 0.74f, 0.86f);
    drawForestPath();
    drawSandMarketDetails();
    drawSnowCrystalCluster(0.50f, -0.35f, 0.90f);
    drawSnowCrystalCluster(0.84f, -0.31f, 0.72f);
    const bool lanternsLit = currentTimePhase() != TimePhase::Day;
    drawLantern(-0.88f, -0.14f, 0.90f, lanternsLit);
    drawLantern(-0.25f, -0.10f, 0.82f, lanternsLit);
    drawLantern(0.07f, -0.08f, 0.82f, lanternsLit);

    // Forest biome: tree line, village hut, farms, crops, and local people.
    drawTriangle(-0.72f, 0.43f, 0.50f, 0.40f, 0.08f, 0.28f, 0.20f);
    drawTriangle(-0.43f, 0.37f, 0.42f, 0.34f, 0.06f, 0.22f, 0.17f);
    drawTree(-0.82f, 0.08f, 0.20f);
    drawTree(-0.48f, 0.27f, 0.18f);
    drawForestVillage();
    drawAnimal(-0.57f, -0.30f, 0.02f);
    drawCircle(-0.56f, -0.28f, 0.028f, 0.82f, 0.68f, 0.24f);
    drawCircle(-0.50f, -0.25f, 0.026f, 0.68f, 0.84f, 0.36f);

    // Sand biome: warm settlement, awning, cactus, and two residents.
    drawTriangle(-0.04f, 0.44f, 0.45f, 0.34f, 0.67f, 0.38f, 0.13f);
    drawTriangle(0.19f, 0.30f, 0.24f, 0.25f, 0.72f, 0.42f, 0.16f);
    drawSandSettlement();
    drawCircle(0.27f, -0.36f, 0.035f, 0.92f, 0.66f, 0.23f);
    drawQuad(0.27f, -0.28f, 0.012f, 0.15f, 0.18f, 0.42f, 0.16f);
    drawTriangle(0.27f, -0.19f, 0.072f, 0.075f, 0.22f, 0.56f, 0.20f);

    // Snow biome: ice peaks and drifting snow, with no human residents.
    drawTriangle(0.46f, 0.43f, 0.52f, 0.48f, 0.43f, 0.60f, 0.70f);
    drawTriangle(0.76f, 0.48f, 0.50f, 0.58f, 0.52f, 0.68f, 0.78f);
    drawTriangle(0.46f, 0.51f, 0.16f, 0.16f, 0.90f, 0.96f, 1.0f);
    drawTriangle(0.76f, 0.58f, 0.18f, 0.18f, 0.92f, 0.98f, 1.0f);
    drawCircle(0.42f, 0.23f, 0.010f, 1.0f, 1.0f, 1.0f, 0.78f);
    drawCircle(0.55f, 0.37f, 0.008f, 1.0f, 1.0f, 1.0f, 0.78f);
    drawCircle(0.83f, 0.28f, 0.011f, 1.0f, 1.0f, 1.0f, 0.78f);
    drawCircle(0.70f, 0.14f, 0.007f, 1.0f, 1.0f, 1.0f, 0.78f);
    if (gProgression.questStage == forest::rpg::QuestStage::DefeatWarden || gEnemyDefeatTimer > 0.0f) {
        drawForestWarden(gEnemyX, gEnemyY, 1.0f, true);
    }

    // A translucent full-scene wash makes the time phase readable even though the
    // prototype uses flat 2D geometry rather than a dynamic skybox.
    if (tintAlpha > 0.0f) {
        drawQuad(0.0f, 0.18f, 2.0f, 1.42f, tintR, tintG, tintB, tintAlpha);
    }
    if (currentTimePhase() == TimePhase::Night) {
        drawCircle(0.72f, 0.58f, 0.082f, 0.92f, 0.95f, 1.0f, 0.92f);
        drawCircle(0.75f, 0.60f, 0.082f, 0.015f, 0.04f, 0.14f, 0.92f);
    } else {
        const float sunGlow = 0.08f + 0.04f * std::sin(phaseProgress * PI);
        drawCircle(0.72f, 0.58f, 0.12f, 1.0f, 0.84f, 0.42f, sunGlow);
        drawCircle(0.72f, 0.58f, 0.072f, 1.0f, 0.70f, 0.28f, 0.90f);
    }
    const float currentRain = rainIntensity();
    const WeatherState weather = currentWeather();
    drawCampfire(weather == WeatherState::Thunderstorm ? 0.58f : weather == WeatherState::Rain ? 0.82f : 1.0f);
    drawHeartfire(-0.63f, -0.22f, gProgression.emberKitCrafted);
    drawResourceCache(-0.56f, -0.28f, 0.30f, 0.48f, 0.22f);
    drawResourceCache(-0.40f, -0.18f, 0.22f, 0.52f, 0.46f);
    drawResourceCache(-0.24f, -0.28f, 0.52f, 0.34f, 0.18f);
    drawRain(currentRain * (0.65f + static_cast<float>(effectiveGraphicsQuality()) * 0.10f));
    drawLightning(lightningIntensity() * (0.70f + static_cast<float>(effectiveGraphicsQuality()) * 0.075f));

    // Water is drawn before the hero so the body remains readable while ripples and
    // surface highlights communicate depth and movement on the small prototype screen.
    const auto& stream = gWaterVolumes[0];
    drawQuad(stream.bounds.center.x, stream.bounds.center.y, stream.bounds.halfExtents.x * 2.0f,
             stream.bounds.halfExtents.y * 2.0f, 0.06f, 0.34f, 0.47f, 0.82f);
    for (int i = 0; i < 5; ++i) {
        const float waveX = stream.bounds.center.x - 0.14f + static_cast<float>(i) * 0.07f;
        const float waveY = stream.surfaceY + 0.008f * std::sin(gTime * 2.6f + static_cast<float>(i));
        drawQuad(waveX, waveY, 0.045f, 0.006f, 0.34f, 0.84f, 0.90f, 0.58f);
    }
    if (gController.body.water.overlapping) {
        const float ripple = 0.045f + 0.012f * std::sin(gTime * 7.0f);
        drawCircle(gPlayerX, stream.surfaceY, ripple, 0.45f, 0.90f, 0.94f, 0.34f);
        drawCircle(gPlayerX, stream.surfaceY, ripple * 0.58f, 0.11f, 0.50f, 0.63f, 0.25f);
    }
    drawPlayer();
}
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_init(JNIEnv*, jobject, jint width, jint height) {
    gPhysicsAccumulator = 0.0;
    gTime = 0.0f;
    gDaysPlayed = 1;
    gWidth = static_cast<float>(std::max(1, width));
    gHeight = static_cast<float>(std::max(1, height));
    gController = {};
    gCombat = {};
    gViewMode = ViewMode::ThirdPerson;
    gWorldMapVisible = false;
    gTowerGlow = 0.0f;
    gTowerCooldown = 0.0f;
    gSynchronizedWorldTime = -1.0f;
    gTowerRevision = 0;
    for (CoOpPeer& peer : gCoOpPeers) peer = {};
    gProgression = {};
    gEmberling = {};
    gWood = 12;
    gFiber = 8;
    gStone = 4;
    gHunger = 0.82f;
    gEnemyHealth = kForestWardenMaxHealth;
    gEnemyHitFlash = 0.0f;
    gEnemyDefeatTimer = 0.0f;
    resetMobs();
    gJumpBufferSeconds = 0.0f;
    gLevelPulse = 0;
    gQuestPulse = 0;
    gGraphicsQuality = 2;
    gContentTierReady = false;
    gController.body.position = {-0.55f, -0.08f};
    gController.body.velocity = {0.0f, 0.0f};
    gController.body.verticalPosition = 0.0f;
    gController.body.verticalVelocity = 0.0f;
    if (gProgram == 0) createProgram();
    if (g3DProgram == 0) create3DProgram();
    if (gBillboardProgram == 0) createBillboardProgram();
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_resize(JNIEnv*, jobject, jint width, jint height) {
    gWidth = static_cast<float>(std::max(1, width));
    gHeight = static_cast<float>(std::max(1, height));
    glViewport(0, 0, width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_render(JNIEnv*, jobject, jfloat delta) {
    const double dt = std::min(0.10, std::max(0.0, static_cast<double>(delta)));
    gPhysicsAccumulator = std::min(0.25, gPhysicsAccumulator + dt);
    int steps = 0;
    while (gPhysicsAccumulator >= kPhysicsStep && steps < 8) {
        simulatePhysicsStep();
        gPhysicsAccumulator -= kPhysicsStep;
        ++steps;
    }
    if (steps == 8 && gPhysicsAccumulator >= kPhysicsStep) gPhysicsAccumulator = 0.0;
    drawWorld();
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setMove(JNIEnv*, jobject, jfloat x, jfloat y) {
    gMoveX = std::clamp(static_cast<float>(x), -1.0f, 1.0f);
    gMoveY = std::clamp(static_cast<float>(y), -1.0f, 1.0f);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setSprintHeld(JNIEnv*, jobject, jboolean held) {
    gSprintHeld = held == JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_orbitCamera(JNIEnv*, jobject, jfloat deltaYaw, jfloat deltaPitch) {
    gController.camera.orbit(static_cast<float>(deltaYaw), static_cast<float>(deltaPitch));
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_toggleViewMode(JNIEnv*, jobject) {
    gViewMode = gViewMode == ViewMode::ThirdPerson ? ViewMode::FirstPerson : ViewMode::ThirdPerson;
    gWorldMapVisible = false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setPlayerCharacterTexture(JNIEnv* env, jobject, jint width, jint height, jintArray pixels) {
    if (width <= 0 || height <= 0 || pixels == nullptr) return;
    const jsize pixelCount = env->GetArrayLength(pixels);
    if (pixelCount < width * height) return;
    std::vector<jint> argb(static_cast<size_t>(width) * static_cast<size_t>(height));
    env->GetIntArrayRegion(pixels, 0, static_cast<jsize>(argb.size()), argb.data());
    std::vector<uint8_t> rgba(argb.size() * 4u);
    for (size_t i = 0; i < argb.size(); ++i) {
        const uint32_t value = static_cast<uint32_t>(argb[i]);
        rgba[i * 4u + 0u] = static_cast<uint8_t>((value >> 16u) & 0xffu);
        rgba[i * 4u + 1u] = static_cast<uint8_t>((value >> 8u) & 0xffu);
        rgba[i * 4u + 2u] = static_cast<uint8_t>(value & 0xffu);
        rgba[i * 4u + 3u] = static_cast<uint8_t>((value >> 24u) & 0xffu);
    }
    if (gPlayerTexture == 0) glGenTextures(1, &gPlayerTexture);
    glBindTexture(GL_TEXTURE_2D, gPlayerTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setWorldMapVisible(JNIEnv*, jobject, jboolean visible) {
    gWorldMapVisible = visible == JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setAuthoritativeOnline(JNIEnv*, jobject, jboolean enabled) {
    gAuthoritativeOnline = enabled == JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setAuthoritativeBossHealth(JNIEnv*, jobject, jint health) {
    gEnemyHealth = std::clamp(static_cast<float>(health), 0.0f, kForestWardenMaxHealth);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_applyAuthoritativeInventory(JNIEnv*, jobject, jint wood, jint fiber, jint stone, jboolean emberKit) {
    gWood = std::max(0, static_cast<int>(wood));
    gFiber = std::max(0, static_cast<int>(fiber));
    gStone = std::max(0, static_cast<int>(stone));
    gProgression.emberKitCrafted = emberKit == JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setWorldTime(JNIEnv*, jobject, jfloat worldTime) {
    if (std::isfinite(static_cast<float>(worldTime))) gSynchronizedWorldTime = std::max(0.0f, static_cast<float>(worldTime));
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setCoOpPeer(JNIEnv*, jobject, jint index, jboolean active, jfloat x, jfloat y, jboolean atTower) {
    const int peerIndex = static_cast<int>(index);
    if (peerIndex < 0 || peerIndex >= static_cast<int>(sizeof(gCoOpPeers) / sizeof(gCoOpPeers[0]))) return;
    CoOpPeer& peer = gCoOpPeers[peerIndex];
    peer.active = active == JNI_TRUE;
    peer.x = std::clamp(static_cast<float>(x), kSimulationMinX, kSimulationMaxX);
    peer.y = std::clamp(static_cast<float>(y), kSimulationMinY, kSimulationMaxY);
    peer.atTower = atTower == JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_clearCoOpPeers(JNIEnv*, jobject) {
    for (CoOpPeer& peer : gCoOpPeers) peer = {};
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_teleportToTower(JNIEnv*, jobject) {
    if (gTowerCooldown > 0.0f) return;
    gController.body.position = {-0.06f, 0.28f};
    gController.body.velocity = {0.0f, 0.0f};
    gController.body.verticalPosition = 0.0f;
    gController.body.verticalVelocity = 0.0f;
    gPlayerX = gController.body.position.x;
    gPlayerY = gController.body.position.y;
    gTowerGlow = 1.8f;
    gTowerCooldown = 4.0f;
    ++gTowerRevision;
    gQuestPulse = 120;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_syncTeleportToTower(JNIEnv*, jobject, jint revision) {
    gController.body.position = {-0.06f, 0.28f};
    gController.body.velocity = {0.0f, 0.0f};
    gController.body.verticalPosition = 0.0f;
    gController.body.verticalVelocity = 0.0f;
    gPlayerX = gController.body.position.x;
    gPlayerY = gController.body.position.y;
    gTowerGlow = 1.8f;
    gTowerCooldown = 0.8f;
    gTowerRevision = std::max(gTowerRevision, static_cast<int>(revision));
    gQuestPulse = 120;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setGyroEnabled(JNIEnv*, jobject, jboolean enabled) {
    gGyroEnabled = enabled == JNI_TRUE;
    if (!gGyroEnabled) {
        gGyroX = 0.0f;
        gGyroY = 0.0f;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setGyro(JNIEnv*, jobject, jfloat rotationX, jfloat rotationY, jfloat sensitivity) {
    gGyroX = static_cast<float>(rotationX) * static_cast<float>(sensitivity);
    gGyroY = static_cast<float>(rotationY) * static_cast<float>(sensitivity);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setGraphicsQuality(JNIEnv*, jobject, jint level) {
    gGraphicsQuality = std::clamp(static_cast<int>(level), 0, 4);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_setContentTierReady(JNIEnv*, jobject, jboolean ready, jint tier) {
    gContentTierReady = ready == JNI_TRUE;
    gGraphicsQuality = std::clamp(static_cast<int>(tier), 0, 4);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_attack(JNIEnv*, jobject) {
    if (gCombat.requestAttack()) gController.state = forest::controller::LocomotionState::Attack;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_heavyAttack(JNIEnv*, jobject) {
    if (gCombat.requestHeavyAttack()) gController.state = forest::controller::LocomotionState::Attack;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_jump(JNIEnv*, jobject) {
    // Preserve a short input window so a tap made at the exact landing frame is
    // still consumed on the next grounded simulation tick.
    gJumpBufferSeconds = 0.16f;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_dodge(JNIEnv*, jobject) {
    if (gCombat.requestDodge() && gController.dodge()) gDodgePulse = 8;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_slide(JNIEnv*, jobject) {
    if (gCombat.requestDodge() && gController.slide()) gDodgePulse = 6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_gather(JNIEnv*, jobject) {
    const float forestCache = std::abs(gPlayerX + 0.56f) + std::abs(gPlayerY + 0.28f);
    const float rootCache = std::abs(gPlayerX + 0.40f) + std::abs(gPlayerY + 0.18f);
    const float wardenCache = std::abs(gPlayerX + 0.24f) + std::abs(gPlayerY + 0.28f);
    const float nearestResource = std::min(forestCache, std::min(rootCache, wardenCache));
    if (nearestResource < 0.32f) {
        if (forestCache <= rootCache && forestCache <= wardenCache) {
            gWood += 1;
            gFiber += 2;
        } else if (rootCache <= wardenCache) {
            gWood += 1;
            gFiber += 1;
        } else {
            gStone += 2;
        }
        gProgression.recordGather();
        gQuestPulse = 90;
        gHunger = std::min(1.0f, gHunger + 0.003f);
    } else {
        gStone += 1;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_craft(JNIEnv*, jobject) {
    if (gWood >= 3 && gFiber >= 2) {
        gWood -= 3;
        gFiber -= 2;
        ++gCraftPulse;
        if (!gProgression.emberKitCrafted) {
            gProgression.recordCraft();
            gQuestPulse = 120;
        } else {
            awardExperience(8);
        }
    } else {
        gStone += 1;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_interactEmberling(JNIEnv*, jobject) {
    const forest::rpg::EmberlingInteraction outcome = forest::rpg::interactWithEmberling(
        gEmberling, gProgression.emberKitCrafted, gFiber, gPlayerX, gPlayerY);
    if (outcome == forest::rpg::EmberlingInteraction::Bonded) {
        awardExperience(30);
        gQuestPulse = 150;
    } else if (outcome == forest::rpg::EmberlingInteraction::BondAdvanced || outcome == forest::rpg::EmberlingInteraction::CommandChanged) {
        gQuestPulse = 90;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_getHudState(JNIEnv* env, jobject) {
    std::ostringstream state;
    state << gProgression.level << '|'
          << gProgression.experience << '|'
          << gProgression.experienceToNext << '|'
          << std::clamp(static_cast<int>(std::round(gController.health * static_cast<float>(kPlayerMaxHealthHp))), 0, kPlayerMaxHealthHp) << '|'
          << static_cast<int>(std::round(gController.stamina * 100.0f)) << '|'
          << static_cast<int>(std::round(gHunger * 100.0f)) << '|'
          << gWood << '|'
          << gFiber << '|'
          << gStone << '|'
          << static_cast<int>(std::round(gEnemyHealth)) << '|'
          << gLevelPulse << '|'
          << gQuestPulse << '|'
          << biomeName() << '|'
          << timePhaseName() << '|'
          << gDaysPlayed << '|'
          << gProgression.questObjective() << '|'
          << waterStateName() << '|'
          << locomotionStateName() << '|'
          << weatherName() << '|'
          << (gViewMode == ViewMode::FirstPerson ? "FIRST_PERSON" : "THIRD_PERSON") << '|'
          << (gWorldMapVisible ? "MAP_ON" : "MAP_OFF") << '|'
          << (gTowerCooldown > 0.0f ? "TOWER_COOLDOWN" : "TOWER_READY") << '|'
          << forest::rpg::emberlingStatus(gEmberling) << '|'
          << nearestMobStatus();
    const std::string value = state.str();
    return env->NewStringUTF(value.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_getCoOpLocalState(JNIEnv* env, jobject) {
    const float towerDistance = std::abs(gPlayerX + 0.06f) + std::abs(gPlayerY - 0.28f);
    std::ostringstream state;
    state << gPlayerX << '|' << gPlayerY << '|' << (towerDistance < 0.16f ? 1 : 0) << '|' << gTowerRevision << '|' << gTime;
    const std::string value = state.str();
    return env->NewStringUTF(value.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_getCloudState(JNIEnv* env, jobject) {
    forest::rpg::CloudState state{};
    state.playerX = gPlayerX;
    state.playerY = gPlayerY;
    state.health = gController.health;
    state.stamina = gController.stamina;
    state.hunger = gHunger;
    state.wood = gWood;
    state.fiber = gFiber;
    state.stone = gStone;
    state.experience = gProgression.experience;
    state.level = gProgression.level;
    state.experienceToNext = gProgression.experienceToNext;
    state.totalExperience = gProgression.totalExperience;
    state.day = gDaysPlayed;
    state.worldTime = gTime;
    state.gatheringActions = gProgression.gatheringActions;
    state.questStage = static_cast<int>(gProgression.questStage);
    state.emberKitCrafted = gProgression.emberKitCrafted;
    state.wardenDefeated = gProgression.wardenDefeated;
    state.emberlingTrust = gEmberling.trust;
    state.emberlingBonded = gEmberling.bonded;
    state.emberlingStay = gEmberling.stay;
    const std::string value = forest::rpg::serializeCloudState(state);
    return env->NewStringUTF(value.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_darvirgoyt_aethelgrad_NativeGameBridge_loadCloudState(JNIEnv* env, jobject, jstring payload) {
    if (payload == nullptr) return JNI_FALSE;
    const char* chars = env->GetStringUTFChars(payload, nullptr);
    if (chars == nullptr) return JNI_FALSE;
    forest::rpg::CloudState state{};
    const bool parsed = forest::rpg::parseCloudState(chars, state);
    env->ReleaseStringUTFChars(payload, chars);
    if (!parsed) return JNI_FALSE;
    gPlayerX = std::clamp(state.playerX, kSimulationMinX, kSimulationMaxX);
    gPlayerY = std::clamp(state.playerY, kSimulationMinY, kSimulationMaxY);
    gController.body.position = {gPlayerX, gPlayerY};
    gController.body.velocity = {0.0f, 0.0f};
    gController.body.verticalPosition = 0.0f;
    gController.body.verticalVelocity = 0.0f;
    gController.body.grounded = true;
    gController.health = state.health;
    gController.stamina = state.stamina;
    gHunger = state.hunger;
    gWood = state.wood;
    gFiber = state.fiber;
    gStone = state.stone;
    if (state.schemaVersion >= 3) {
        gProgression.restoreState(state.level, state.experience, state.experienceToNext, state.totalExperience);
    } else {
        gProgression.restoreLegacyExperience(state.experience);
    }
    gProgression.gatheringActions = state.gatheringActions;
    gProgression.questStage = static_cast<forest::rpg::QuestStage>(std::clamp(state.questStage, 0, 3));
    gProgression.emberKitCrafted = state.emberKitCrafted;
    gProgression.wardenDefeated = state.wardenDefeated;
    gEmberling.trust = state.emberlingTrust;
    gEmberling.bonded = state.emberlingBonded;
    gEmberling.stay = state.emberlingStay;
    gEmberling.pulse = 0.0f;
    gEnemyHealth = gProgression.wardenDefeated ? 0.0f : kForestWardenMaxHealth;
    gDaysPlayed = state.day;
    gTime = state.worldTime;
    return JNI_TRUE;
}
