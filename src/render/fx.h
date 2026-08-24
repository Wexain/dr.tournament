#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Post-Processing FX
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>

namespace drt {

class PostFX {
public:
    void init(int screen_w, int screen_h);
    void beginCapture();
    void endCaptureAndRender();
    void cleanup();
    void onResize(int screen_w, int screen_h);

    // Toggle effects
    void setFXAA(bool on)   { fxaa_enabled_ = on; }
    void setMSAA(int level) { msaa_level_ = level; }

    [[nodiscard]] bool fxaaEnabled() const { return fxaa_enabled_; }
    [[nodiscard]] int  msaaLevel()  const { return msaa_level_; }

private:
    RenderTexture2D target_ = {};
    Shader          fxaa_shader_ = {};
    bool            fxaa_enabled_ = false;
    int             msaa_level_ = 0;
    int             screen_w_ = 1280;
    int             screen_h_ = 720;
    bool            initialized_ = false;
};

} // namespace drt
