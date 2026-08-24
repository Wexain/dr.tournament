// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Camera System Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "core/camera_system.h"
#include <raymath.h>
#include <cmath>

namespace drt {

void CameraSystem::init() {
    camera_.position = {0, 5, -10};
    camera_.target = {0, 0, 0};
    camera_.up = {0, 1, 0};
    camera_.fovy = 55.0f;
    camera_.projection = CAMERA_PERSPECTIVE;
    current_pos_ = camera_.position;
    current_target_ = camera_.target;
}

void CameraSystem::update(Vector3 car_pos, Vector3 car_forward, Vector3 car_up, float dt) {
    Vector3 desired_pos, desired_target;

    Vector3 actual_forward = rear_view_ ? Vector3Negate(car_forward) : car_forward;

    switch (mode_) {
        case CameraMode::CHASE: {
            Vector3 offset = Vector3Scale(actual_forward, -chase_distance);
            offset.y += chase_height;
            desired_pos = Vector3Add(car_pos, offset);
            desired_target = Vector3Add(car_pos, {0, 1.0f, 0});
            break;
        }
        case CameraMode::BUMPER: {
            Vector3 offset = Vector3Scale(actual_forward, 0.5f);
            offset.y += bumper_height;
            desired_pos = Vector3Add(car_pos, offset);
            desired_target = Vector3Add(car_pos, Vector3Scale(actual_forward, 20.0f));
            desired_target.y += 0.5f;
            break;
        }
        case CameraMode::HOOD: {
            Vector3 offset = Vector3Scale(actual_forward, hood_forward);
            offset.y += hood_height;
            desired_pos = Vector3Add(car_pos, offset);
            desired_target = Vector3Add(car_pos, Vector3Scale(actual_forward, 30.0f));
            desired_target.y += hood_height;
            break;
        }
        case CameraMode::FREE_CAM: {
            // Free cam uses its own position — no car tracking
            desired_pos = current_pos_;
            float yaw_rad = free_yaw_ * DEG2RAD;
            float pitch_rad = free_pitch_ * DEG2RAD;
            Vector3 dir = {
                cosf(pitch_rad) * sinf(yaw_rad),
                sinf(pitch_rad),
                cosf(pitch_rad) * cosf(yaw_rad)
            };
            desired_target = Vector3Add(current_pos_, dir);
            camera_.position = current_pos_;
            camera_.target = desired_target;
            return;
        }
        default:
            desired_pos = current_pos_;
            desired_target = current_target_;
            break;
    }

    // Smooth interpolation
    float smooth = 1.0f - expf(-chase_smoothing * dt);
    current_pos_ = Vector3Lerp(current_pos_, desired_pos, smooth);
    current_target_ = Vector3Lerp(current_target_, desired_target, smooth);

    camera_.position = current_pos_;
    camera_.target = current_target_;
    camera_.up = {0, 1, 0};
}

void CameraSystem::cycleMode() {
    int next = (static_cast<int>(mode_) + 1) % static_cast<int>(CameraMode::COUNT);
    mode_ = static_cast<CameraMode>(next);
}

void CameraSystem::freeCamMove(Vector3 delta) {
    current_pos_ = Vector3Add(current_pos_, delta);
}

void CameraSystem::freeCamRotate(float yaw, float pitch) {
    free_yaw_ += yaw;
    free_pitch_ += pitch;
    free_pitch_ = Clamp(free_pitch_, -85.0f, 85.0f);
}

} // namespace drt
