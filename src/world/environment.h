#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Environment (Sky, Props, Lighting)
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>

namespace drt {

enum class TimeOfDay : uint8_t {
    MORNING,
    NOON,
    EVENING,
    NIGHT
};

class Environment {
public:
    void init();
    void update(float dt);
    void render();
    void cleanup();

    void setTimeOfDay(TimeOfDay t) { time_ = t; updateLighting(); }
    [[nodiscard]] TimeOfDay timeOfDay() const { return time_; }

    [[nodiscard]] Color skyColorTop() const { return sky_top_; }
    [[nodiscard]] Color skyColorBottom() const { return sky_bottom_; }
    [[nodiscard]] Color ambientColor() const { return ambient_; }
    [[nodiscard]] Vector3 sunDirection() const { return sun_dir_; }

    // Ground plane
    void renderGround();

private:
    void updateLighting();
    void renderSkyGradient();
    void renderRoadsideProps(Vector3 camera_pos);

    TimeOfDay time_       = TimeOfDay::NOON;
    Color     sky_top_    = {135, 206, 235, 255};
    Color     sky_bottom_ = {200, 230, 255, 255};
    Color     ambient_    = {80, 80, 90, 255};
    Vector3   sun_dir_    = {-0.5f, -1.0f, -0.3f};
    Color     sun_color_  = {255, 250, 230, 255};

    // Procedural props
    Model     tree_model_ = {};
    bool      models_loaded_ = false;
};

} // namespace drt
