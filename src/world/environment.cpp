// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Environment Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "world/environment.h"
#include <raymath.h>

namespace drt {

void Environment::init() {
    updateLighting();
}

void Environment::update(float dt) {
    // Could animate sun position, clouds, etc.
}

void Environment::render() {
    renderSkyGradient();
}

void Environment::renderGround() {
    // Infinite ground plane
    DrawPlane({0, 0, 0}, {2000, 2000}, {45, 55, 40, 255});
}

void Environment::renderSkyGradient() {
    // Rendered as background — the ClearBackground in engine handles sky color
    // For more advanced, use a fullscreen quad with gradient shader
}

void Environment::updateLighting() {
    switch (time_) {
        case TimeOfDay::MORNING:
            sky_top_ = {135, 180, 220, 255};
            sky_bottom_ = {220, 200, 180, 255};
            ambient_ = {100, 90, 80, 255};
            sun_dir_ = {-0.3f, -0.7f, -0.5f};
            sun_color_ = {255, 220, 180, 255};
            break;
        case TimeOfDay::NOON:
            sky_top_ = {100, 160, 230, 255};
            sky_bottom_ = {180, 210, 240, 255};
            ambient_ = {120, 120, 130, 255};
            sun_dir_ = {-0.1f, -1.0f, -0.2f};
            sun_color_ = {255, 250, 240, 255};
            break;
        case TimeOfDay::EVENING:
            sky_top_ = {60, 40, 90, 255};
            sky_bottom_ = {200, 100, 50, 255};
            ambient_ = {80, 60, 50, 255};
            sun_dir_ = {-0.8f, -0.3f, -0.4f};
            sun_color_ = {255, 160, 80, 255};
            break;
        case TimeOfDay::NIGHT:
            sky_top_ = {10, 10, 30, 255};
            sky_bottom_ = {20, 20, 50, 255};
            ambient_ = {30, 30, 45, 255};
            sun_dir_ = {0, -1, 0};
            sun_color_ = {80, 80, 120, 255};
            break;
    }
}

void Environment::renderRoadsideProps(Vector3 camera_pos) {
    // Procedural trees alongside the road
    // Simple cone + cylinder trees
    for (int i = -10; i < 10; ++i) {
        float z = camera_pos.z + i * 30.0f;
        // Left side
        DrawCylinder({-20.0f, 0, z}, 0.3f, 0.3f, 2.0f, 6, {80, 50, 30, 255});
        DrawCone({-20.0f, 2.0f, z}, 2.0f, 3.0f, 6, {30, 100, 30, 255});
        // Right side
        DrawCylinder({20.0f, 0, z}, 0.3f, 0.3f, 2.0f, 6, {80, 50, 30, 255});
        DrawCone({20.0f, 2.0f, z}, 2.0f, 3.0f, 6, {30, 100, 30, 255});
    }
}

void Environment::cleanup() {
    if (models_loaded_) {
        UnloadModel(tree_model_);
        models_loaded_ = false;
    }
}

} // namespace drt
