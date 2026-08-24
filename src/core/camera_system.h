#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Camera System
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>

namespace drt {

enum class CameraMode : uint8_t {
    CHASE,      // Third-person behind car
    BUMPER,     // Low, front-of-car view
    HOOD,       // On the hood
    FREE_CAM,   // Spectator free-look
    COUNT
};

class CameraSystem {
public:
    void init();
    void update(Vector3 car_pos, Vector3 car_forward, Vector3 car_up, float dt);

    void cycleMode();
    void setMode(CameraMode m) { mode_ = m; }
    [[nodiscard]] CameraMode mode() const { return mode_; }

    void setRearView(bool rear) { rear_view_ = rear; }

    [[nodiscard]] Camera3D& camera() { return camera_; }
    [[nodiscard]] const Camera3D& camera() const { return camera_; }

    // Spectator free-cam controls
    void freeCamMove(Vector3 delta);
    void freeCamRotate(float yaw, float pitch);

    // Configuration
    float chase_distance   = 8.0f;
    float chase_height     = 3.5f;
    float chase_smoothing  = 6.0f;
    float bumper_height    = 0.8f;
    float hood_height      = 1.2f;
    float hood_forward     = 1.5f;

private:
    Camera3D   camera_{};
    CameraMode mode_     = CameraMode::CHASE;
    bool       rear_view_ = false;

    // Smooth interpolation state
    Vector3 current_pos_    = {0, 5, -10};
    Vector3 current_target_ = {0, 0, 0};

    // Free-cam state
    float free_yaw_   = 0.0f;
    float free_pitch_ = -20.0f;
};

} // namespace drt
