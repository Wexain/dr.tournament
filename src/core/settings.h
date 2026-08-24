#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Settings & Configuration
// ═══════════════════════════════════════════════════════════════════════════
#include <string>
#include <cstdint>

namespace drt {

enum class QualityPreset : uint8_t {
    LOW,     // Integrated GPU / low-end
    MEDIUM,  // Mid-range
    HIGH,    // High-end / dedicated GPU
    CUSTOM
};

enum class ShadowQuality : uint8_t {
    OFF,
    BLOB,
    SIMPLE_DYNAMIC,
    SOFT
};

struct GraphicsSettings {
    int              resolution_w    = 1280;
    int              resolution_h    = 720;
    float            render_scale    = 1.0f;     // 0.5 to 1.0
    bool             fullscreen      = false;
    bool             vsync           = true;
    ShadowQuality    shadows         = ShadowQuality::BLOB;
    int              lod_level       = 1;        // 0=low, 1=medium, 2=high
    bool             mirrors         = false;
    bool             fxaa            = false;
    int              msaa            = 0;        // 0, 2, 4
    float            view_distance   = 300.0f;
};

struct AudioSettings {
    float master_volume = 0.8f;
    float sfx_volume    = 1.0f;
    float music_volume  = 0.7f;
    float voice_volume  = 1.0f;
    bool  push_to_talk  = true;
    float voice_gain    = 1.0f;   // 0.0 to 2.0 (0-200%)
};

struct ControlSettings {
    float steering_sensitivity = 1.0f;
    float gamepad_deadzone     = 0.15f;
    bool  gamepad_vibration    = true;
    bool  auto_steer_return    = true;  // Android: spring-back wheel
};

class Settings {
public:
    void init();
    bool load(const std::string& path = "settings.json");
    bool save(const std::string& path = "settings.json") const;

    void applyGraphics();     // Apply current graphics settings to Raylib
    void autoDetectQuality(); // Probe hardware and set optimal preset

    // Accessors
    GraphicsSettings& graphics() { return graphics_; }
    AudioSettings&    audio()    { return audio_; }
    ControlSettings&  controls() { return controls_; }

    const GraphicsSettings& graphics() const { return graphics_; }
    const AudioSettings&    audio()    const { return audio_; }
    const ControlSettings&  controls() const { return controls_; }

    QualityPreset preset() const { return preset_; }
    void setPreset(QualityPreset p);

    // Auth / player info
    std::string& nickname()       { return nickname_; }
    const std::string& nickname() const { return nickname_; }
    std::string& uuid()           { return uuid_; }

private:
    void applyPresetLow();
    void applyPresetMedium();
    void applyPresetHigh();

    GraphicsSettings graphics_;
    AudioSettings    audio_;
    ControlSettings  controls_;
    QualityPreset    preset_ = QualityPreset::MEDIUM;

    std::string nickname_ = "Driver";
    std::string uuid_;
};

} // namespace drt
