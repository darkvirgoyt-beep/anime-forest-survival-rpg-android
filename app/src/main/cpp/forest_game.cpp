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

namespace {
constexpr float PI = 3.14159265359f;
GLuint gProgram = 0;
GLint gPosition = -1;
GLint gColor = -1;
GLint gScale = -1;
GLint gOffset = -1;
float gWidth = 1.0f;
float gHeight = 1.0f;
float gTime = 0.0f;
float gPlayerX = 0.0f;
float gPlayerY = -0.08f;
float gMoveX = 0.0f;
float gMoveY = 0.0f;
bool gSprintHeld = false;
float gGyroX = 0.0f;
float gGyroY = 0.0f;
bool gGyroEnabled = false;
int gWood = 12;
int gFiber = 8;
int gStone = 4;
int gCraftPulse = 0;
int gAttackPulse = 0;
int gDodgePulse = 0;
int gLevelPulse = 0;
int gQuestPulse = 0;
float gHunger = 0.82f;
double gPhysicsAccumulator = 0.0;
constexpr float kPhysicsStep = 1.0f / 60.0f;
forest::controller::ThirdPersonController gController{};
forest::combat::CombatSystem gCombat{};
forest::rpg::Progression gProgression{};
bool gHitRegistered = false;
float gEnemyHealth = 1.0f;
float gEnemyHitFlash = 0.0f;
float gEnemyDefeatTimer = 0.0f;
float gEnemyX = 0.36f;
float gEnemyY = 0.03f;
const forest::physics::StaticObstacle gObstacles[] = {
    {{{-0.30f, -0.28f}, {0.07f, 0.04f}}},
    {{{0.60f, -0.32f}, {0.06f, 0.04f}}}
};

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

void drawPlayer() {
    // The hero is built from deliberately flat color planes. Drawing the ink silhouette
    // first and the lit/shadow planes second gives the 2D prototype a mobile-safe cel look.
    const float bob = std::sin(gTime * 4.0f) * 0.004f;
    const float px = gPlayerX;
    const float py = gPlayerY + bob;
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
    drawTriangle(px, py + 0.040f, 0.154f, 0.215f, inkR, inkG, inkB);
    drawTriangle(px, py + 0.043f, 0.128f, 0.190f, 0.43f, 0.12f, 0.29f);
    drawTriangle(px - 0.029f, py + 0.052f, 0.058f, 0.158f, 0.25f, 0.07f, 0.20f);
    drawTriangle(px + 0.018f, py + 0.073f, 0.044f, 0.112f, 0.68f, 0.18f, 0.34f);
    drawQuad(px, py + 0.004f, 0.114f, 0.024f, inkR, inkG, inkB);
    drawQuad(px, py + 0.009f, 0.096f, 0.012f, 0.06f, 0.38f, 0.38f);
    drawQuad(px + 0.024f, py + 0.040f, 0.022f, 0.096f, 0.10f, 0.55f, 0.51f);

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
    drawCircle(px + 0.006f, py + 0.177f, 0.079f, inkR, inkG, inkB);
    drawCircle(px + 0.010f, py + 0.181f, 0.068f, 0.12f, 0.06f, 0.22f);
    drawTriangle(px - 0.050f, py + 0.215f, 0.048f, 0.082f, inkR, inkG, inkB);
    drawTriangle(px - 0.050f, py + 0.215f, 0.030f, 0.061f, 0.17f, 0.08f, 0.30f);
    drawTriangle(px - 0.014f, py + 0.226f, 0.046f, 0.092f, inkR, inkG, inkB);
    drawTriangle(px - 0.014f, py + 0.226f, 0.027f, 0.070f, 0.20f, 0.10f, 0.35f);
    drawTriangle(px + 0.030f, py + 0.220f, 0.052f, 0.082f, inkR, inkG, inkB);
    drawTriangle(px + 0.030f, py + 0.220f, 0.032f, 0.061f, 0.17f, 0.08f, 0.29f);
    drawTriangle(px + 0.035f, py + 0.191f, 0.042f, 0.075f, 0.29f, 0.15f, 0.43f);
    drawTriangle(px - 0.021f, py + 0.170f, 0.035f, 0.055f, 0.09f, 0.04f, 0.17f);

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

void drawWarden() {
    if (gEnemyDefeatTimer > 0.0f) {
        const float fade = std::clamp(gEnemyDefeatTimer / 1.5f, 0.0f, 1.0f);
        drawCircle(gEnemyX, gEnemyY + 0.04f, 0.14f, 0.74f, 0.22f, 0.25f, 0.10f * fade);
        return;
    }
    const float flash = gEnemyHitFlash > 0.0f ? 0.28f : 0.0f;
    drawCircle(gEnemyX + 0.012f, gEnemyY - 0.025f, 0.095f, 0.02f, 0.03f, 0.04f, 0.5f);
    drawCircle(gEnemyX, gEnemyY, 0.10f, 0.30f + flash, 0.08f, 0.14f, 1.0f);
    drawTriangle(gEnemyX - 0.055f, gEnemyY + 0.095f, 0.06f, 0.09f, 0.58f, 0.14f, 0.18f);
    drawTriangle(gEnemyX + 0.055f, gEnemyY + 0.095f, 0.06f, 0.09f, 0.58f, 0.14f, 0.18f);
    drawCircle(gEnemyX - 0.035f, gEnemyY + 0.025f, 0.012f, 1.0f, 0.76f, 0.28f);
    drawCircle(gEnemyX + 0.035f, gEnemyY + 0.025f, 0.012f, 1.0f, 0.76f, 0.28f);
    drawQuad(gEnemyX, gEnemyY + 0.18f, 0.24f, 0.018f, 0.08f, 0.03f, 0.05f, 0.9f);
    drawQuad(gEnemyX - 0.12f + 0.12f * gEnemyHealth, gEnemyY + 0.18f,
             0.24f * std::clamp(gEnemyHealth, 0.0f, 1.0f), 0.012f, 0.86f, 0.18f, 0.24f, 0.95f);
}

void simulatePhysicsStep() {
    gTime += kPhysicsStep;
    if (gGyroEnabled) gController.camera.orbit(gGyroX * 0.012f, gGyroY * 0.008f);
    const forest::controller::InputFrame input{gMoveX, -gMoveY, gController.camera.yaw, gSprintHeld};
    gController.tick(input, kPhysicsStep, gObstacles, static_cast<int>(sizeof(gObstacles) / sizeof(gObstacles[0])));
    gCombat.tick(kPhysicsStep);
    gEnemyHitFlash = std::max(0.0f, gEnemyHitFlash - kPhysicsStep);
    gEnemyDefeatTimer = std::max(0.0f, gEnemyDefeatTimer - kPhysicsStep);
    const forest::combat::CombatEvent combatEvent = gCombat.consumeEvent();
    if (combatEvent.attackStarted) {
        gAttackPulse = 6 + combatEvent.comboIndex * 2;
        gHitRegistered = false;
    }
    if (gCombat.isHitActive() && !gHitRegistered) {
        const forest::physics::Vec2 facing{
            std::cos(gController.facingRadians),
            std::sin(gController.facingRadians)
        };
        const forest::combat::Hitbox hitbox = gCombat.currentHitbox(facing);
        const forest::physics::Aabb attackBox{
            gController.body.position + hitbox.offset,
            hitbox.halfExtents
        };
        const forest::physics::Aabb enemyBox{{gEnemyX, gEnemyY}, {0.07f, 0.06f}};
        if (forest::combat::intersects(attackBox, enemyBox)) {
            gEnemyHealth = std::max(0.0f, gEnemyHealth - hitbox.damage);
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
    gPlayerX = gController.body.position.x;
    gPlayerY = gController.body.position.y;
    gHunger = std::max(0.0f, gHunger - kPhysicsStep * 0.001f);
    if (gHunger < 0.20f) gController.health = std::max(0.0f, gController.health - kPhysicsStep * 0.004f);
    gLevelPulse = std::max(0, gLevelPulse - 1);
    gQuestPulse = std::max(0, gQuestPulse - 1);
}

void drawWorld() {
    const float night = 0.06f + 0.035f * (std::sin(gTime * 0.14f) * 0.5f + 0.5f);
    glClearColor(0.025f + night, 0.09f + night, 0.105f + night, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(gProgram);

    drawQuad(0.0f, 0.25f, 2.0f, 1.5f, 0.035f, 0.12f, 0.15f);
    drawTriangle(-0.58f, 0.45f, 1.35f, 0.85f, 0.08f, 0.21f, 0.22f);
    drawTriangle(0.48f, 0.46f, 1.50f, 0.92f, 0.065f, 0.17f, 0.20f);
    drawQuad(0.0f, -0.53f, 2.0f, 0.95f, 0.075f, 0.23f, 0.19f);
    drawQuad(-0.18f, -0.24f, 0.92f, 0.085f, 0.25f, 0.32f, 0.22f, 0.48f);
    drawQuad(0.34f, -0.11f, 0.54f, 0.07f, 0.25f, 0.32f, 0.22f, 0.38f);
    drawCircle(0.72f, 0.58f, 0.09f, 0.94f, 0.70f, 0.37f, 0.9f);
    drawCircle(0.72f, 0.58f, 0.125f, 0.96f, 0.82f, 0.43f, 0.12f);

    drawTree(-0.72f, 0.08f, 0.22f);
    drawTree(-0.47f, 0.28f, 0.18f);
    drawTree(0.55f, 0.20f, 0.24f);
    drawTree(0.82f, 0.02f, 0.18f);
    drawTree(-0.12f, 0.43f, 0.15f);
    drawRock(-0.30f, -0.28f, 0.13f);
    drawRock(0.60f, -0.32f, 0.11f);
    drawCircle(-0.05f, -0.30f, 0.035f, 0.81f, 0.66f, 0.25f);
    drawCircle(-0.00f, -0.27f, 0.035f, 0.66f, 0.84f, 0.36f);
    drawAnimal(-0.60f, -0.30f, 0.02f);
    if (gEnemyHealth > 0.0f || gEnemyDefeatTimer > 0.0f) drawWarden();
    drawPlayer();
}
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_init(JNIEnv*, jobject, jint width, jint height) {
    gPhysicsAccumulator = 0.0;
    gWidth = static_cast<float>(std::max(1, width));
    gHeight = static_cast<float>(std::max(1, height));
    gController = {};
    gCombat = {};
    gProgression = {};
    gWood = 12;
    gFiber = 8;
    gStone = 4;
    gHunger = 0.82f;
    gEnemyHealth = 1.0f;
    gEnemyHitFlash = 0.0f;
    gEnemyDefeatTimer = 0.0f;
    gLevelPulse = 0;
    gQuestPulse = 0;
    gController.body.position = {0.0f, -0.08f};
    gController.body.velocity = {0.0f, 0.0f};
    if (gProgram == 0) createProgram();
    glViewport(0, 0, width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_resize(JNIEnv*, jobject, jint width, jint height) {
    gWidth = static_cast<float>(std::max(1, width));
    gHeight = static_cast<float>(std::max(1, height));
    glViewport(0, 0, width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_render(JNIEnv*, jobject, jfloat delta) {
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
Java_com_darkvirgoyt_forestslice_NativeGameBridge_setMove(JNIEnv*, jobject, jfloat x, jfloat y) {
    gMoveX = std::clamp(static_cast<float>(x), -1.0f, 1.0f);
    gMoveY = std::clamp(static_cast<float>(y), -1.0f, 1.0f);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_setSprintHeld(JNIEnv*, jobject, jboolean held) {
    gSprintHeld = held == JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_orbitCamera(JNIEnv*, jobject, jfloat deltaYaw, jfloat deltaPitch) {
    gController.camera.orbit(static_cast<float>(deltaYaw), static_cast<float>(deltaPitch));
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_setGyroEnabled(JNIEnv*, jobject, jboolean enabled) {
    gGyroEnabled = enabled == JNI_TRUE;
    if (!gGyroEnabled) {
        gGyroX = 0.0f;
        gGyroY = 0.0f;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_setGyro(JNIEnv*, jobject, jfloat rotationX, jfloat rotationY, jfloat sensitivity) {
    gGyroX = static_cast<float>(rotationX) * static_cast<float>(sensitivity);
    gGyroY = static_cast<float>(rotationY) * static_cast<float>(sensitivity);
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_attack(JNIEnv*, jobject) {
    if (gCombat.requestAttack()) gController.state = forest::controller::LocomotionState::Attack;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_jump(JNIEnv*, jobject) {
    gController.jump();
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_dodge(JNIEnv*, jobject) {
    if (gCombat.requestDodge() && gController.dodge()) gDodgePulse = 8;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_slide(JNIEnv*, jobject) {
    if (gCombat.requestDodge() && gController.slide()) gDodgePulse = 6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_gather(JNIEnv*, jobject) {
    const float nearestResource = std::min(
        std::abs(gPlayerX + 0.05f) + std::abs(gPlayerY + 0.30f),
        std::abs(gPlayerX - 0.60f) + std::abs(gPlayerY + 0.32f)
    );
    if (nearestResource < 0.42f) {
        gWood += 1;
        gFiber += 1;
        gProgression.recordGather();
        gQuestPulse = 90;
        gHunger = std::min(1.0f, gHunger + 0.003f);
    } else {
        gStone += 1;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_craft(JNIEnv*, jobject) {
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_darkvirgoyt_forestslice_NativeGameBridge_getHudState(JNIEnv* env, jobject) {
    std::ostringstream state;
    state << gProgression.level << '|'
          << gProgression.experience << '|'
          << gProgression.experienceToNext << '|'
          << static_cast<int>(std::round(gController.health * 100.0f)) << '|'
          << static_cast<int>(std::round(gController.stamina * 100.0f)) << '|'
          << static_cast<int>(std::round(gHunger * 100.0f)) << '|'
          << gWood << '|'
          << gFiber << '|'
          << gStone << '|'
          << static_cast<int>(std::round(gEnemyHealth * 100.0f)) << '|'
          << gLevelPulse << '|'
          << gQuestPulse << '|'
          << gProgression.questObjective();
    const std::string value = state.str();
    return env->NewStringUTF(value.c_str());
}
