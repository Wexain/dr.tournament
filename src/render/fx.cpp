// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Post-Processing FX Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "render/fx.h"
#include <raylib.h>

namespace drt {

void PostFX::init(int screen_w, int screen_h) {
    screen_w_ = screen_w;
    screen_h_ = screen_h;

    target_ = LoadRenderTexture(screen_w, screen_h);

    // Try to load FXAA shader
    fxaa_shader_ = LoadShader(nullptr, "assets/shaders/fxaa.fs");
    if (fxaa_shader_.id > 0) {
        // Set resolution uniform
        float resolution[2] = {(float)screen_w, (float)screen_h};
        SetShaderValue(fxaa_shader_, GetShaderLocation(fxaa_shader_, "resolution"),
                       resolution, SHADER_UNIFORM_VEC2);
    }

    initialized_ = true;
}

void PostFX::beginCapture() {
    if (!initialized_ || !fxaa_enabled_) return;
    BeginTextureMode(target_);
}

void PostFX::endCaptureAndRender() {
    if (!initialized_ || !fxaa_enabled_) return;
    EndTextureMode();

    // Draw the render texture with FXAA shader
    BeginShaderMode(fxaa_shader_);
    DrawTextureRec(target_.texture,
                   {0, 0, (float)target_.texture.width, -(float)target_.texture.height},
                   {0, 0}, WHITE);
    EndShaderMode();
}

void PostFX::cleanup() {
    if (initialized_) {
        UnloadRenderTexture(target_);
        if (fxaa_shader_.id > 0) UnloadShader(fxaa_shader_);
        initialized_ = false;
    }
}

void PostFX::onResize(int screen_w, int screen_h) {
    if (initialized_) {
        UnloadRenderTexture(target_);
        target_ = LoadRenderTexture(screen_w, screen_h);
        screen_w_ = screen_w;
        screen_h_ = screen_h;

        if (fxaa_shader_.id > 0) {
            float resolution[2] = {(float)screen_w, (float)screen_h};
            SetShaderValue(fxaa_shader_, GetShaderLocation(fxaa_shader_, "resolution"),
                           resolution, SHADER_UNIFORM_VEC2);
        }
    }
}

} // namespace drt
