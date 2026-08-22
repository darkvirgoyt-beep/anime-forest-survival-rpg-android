#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "physics/physics.h"
#include "controller/third_person_controller.h"
#include "combat/combat_system.h"

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
int gWood = 12;
int gFiber = 8;
int gStone = 4;
int gCraftPulse = 0;
int gAttackPulse = 0;
int gDodgePulse = 0;
float gHunger = 0.82f;
double gPhysicsAccumulator = 0.0;
constexpr float kPhysicsStep = 1.0f / 60.0f;
forest::controller::ThirdPersonController gController{};
forest::combat::CombatSystem gCombat{};
bool gHitRegistered = false;
float gEnemyHealth = 1.0f;
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
    drawCircle(gPlayerX + 0.012f, gPlayerY - 0.035f, 0.065f, 0.02f, 0.05f, 0.05f, 0.45f);
    drawTriangle(gPlayerX, gPlayerY + 0.03f, 0.12f, 0.22f, 0.72f, 0.16f, 0.28f);
    drawCircle(gPlayerX, gPlayerY + 0.13f, 0.07f, 0.96f, 0.75f, 0.57f);
    drawCircle(gPlayerX + 0.028f, gPlayerY + 0.155f, 0.062f, 0.14f, 0.08f, 0.19f);
    drawQuad(gPlayerX + 0.087f, gPlayerY + 0.02f, 0.015f, 0.16f, 0.94f, 0.77f, 0.35f);
    if (gAttackPulse > 0) {
        drawCircle(gPlayerX + 0.14f, gPlayerY + 0.03f, 0.06f, 0.97f, 0.85f, 0.42f, 0.70f);
        --gAttackPulse;
    }
    if (gDodgePulse > 0) {
        drawCircle(gPlayerX, gPlayerY + 0.03f, 0.13f, 0.40f, 0.85f, 0.95f, 0.22f);
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

void simulatePhysicsStep() {
    gTime += kPhysicsStep;
    const forest::controller::InputFrame input{gMoveX, -gMoveY, gController.camera.yaw, false};
    gController.tick(input, kPhysicsStep, gObstacles, static_cast<int>(sizeof(gObstacles) / sizeof(gObstacles[0])));
    gCombat.tick(kPhysicsStep);
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
            gHitRegistered = true;
            gCombat.confirmHit();
        }
    }
    gPlayerX = gController.body.position.x;
    gPlayerY = gController.body.position.y;
    gHunger = std::max(0.0f, gHunger - kPhysicsStep * 0.001f);
    if (gHunger < 0.20f) gController.health = std::max(0.0f, gController.health - kPhysicsStep * 0.004f);
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
    drawAnimal(0.36f, 0.03f, 0.10f);
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
Java_com_darkvirgoyt_forestslice_NativeGameBridge_orbitCamera(JNIEnv*, jobject, jfloat deltaYaw, jfloat deltaPitch) {
    gController.camera.orbit(static_cast<float>(deltaYaw), static_cast<float>(deltaPitch));
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
Java_com_darkvirgoyt_forestslice_NativeGameBridge_gather(JNIEnv*, jobject) {
    const float nearestResource = std::min(
        std::abs(gPlayerX + 0.05f) + std::abs(gPlayerY + 0.30f),
        std::abs(gPlayerX - 0.60f) + std::abs(gPlayerY + 0.32f)
    );
    if (nearestResource < 0.42f) {
        gWood += 1;
        gFiber += 1;
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
    } else {
        gStone += 1;
    }
}
