// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Input System Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "core/input_system.h"
#include <cmath>
#include <algorithm>

namespace drt {

void InputSystem::init(int screen_w, int screen_h) {
    screen_w_ = screen_w;
    screen_h_ = screen_h;
    setupTouchLayout(screen_w, screen_h);
}

void InputSystem::onResize(int screen_w, int screen_h) {
    screen_w_ = screen_w;
    screen_h_ = screen_h;
    setupTouchLayout(screen_w, screen_h);
}

void InputSystem::update(float dt) {
    prev_state_ = state_;
    state_ = {};  // Reset

    updateKeyboard();
    updateGamepad();

    #if defined(DRT_PLATFORM_ANDROID)
    updateTouch(dt);
    #endif
}

void InputSystem::updateKeyboard() {
    // Steering
    if (IsKeyDown(key_steer_left_))  state_.steering -= 1.0f;
    if (IsKeyDown(key_steer_right_)) state_.steering += 1.0f;
    state_.steering = std::clamp(state_.steering, -1.0f, 1.0f);

    // Gas / Brake
    if (IsKeyDown(key_gas_))   state_.gas = 1.0f;
    if (IsKeyDown(key_brake_)) state_.brake = 1.0f;
    state_.handbrake = IsKeyDown(key_handbrake_);

    // Gear shifting (number keys)
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_P))  state_.shift_park = true;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_R))  state_.shift_reverse = true;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_N)) state_.shift_neutral = true;
    if (IsKeyPressed(KEY_FOUR))                         state_.shift_drive = true;
    // KEY_D conflicts with steer, so use KEY_FOUR for drive or dedicated button

    // Indicators
    if (IsKeyPressed(KEY_Q)) state_.indicator_left = true;
    if (IsKeyPressed(KEY_E)) state_.indicator_right = true;
    if (IsKeyPressed(KEY_H)) state_.hazards = true;

    // Horn
    state_.horn = IsKeyDown(KEY_F);

    // Camera
    if (IsKeyPressed(KEY_C)) state_.camera_cycle = true;
    state_.rear_view = IsKeyDown(KEY_B);

    // Voice
    state_.push_to_talk = IsKeyDown(KEY_V);

    // UI
    state_.show_leaderboard = IsKeyDown(KEY_TAB);
    if (IsKeyPressed(KEY_ESCAPE)) state_.toggle_settings = true;

    // Menu
    if (IsKeyPressed(KEY_UP))    state_.menu_up = true;
    if (IsKeyPressed(KEY_DOWN))  state_.menu_down = true;
    if (IsKeyPressed(KEY_LEFT))  state_.menu_left = true;
    if (IsKeyPressed(KEY_RIGHT)) state_.menu_right = true;
    if (IsKeyPressed(KEY_ENTER)) state_.menu_confirm = true;
    if (IsKeyPressed(KEY_ESCAPE)) state_.menu_back = true;
}

void InputSystem::updateGamepad() {
    if (!gamepad_cfg_.enabled) return;
    int gp = gamepad_cfg_.gamepad_id;
    if (!IsGamepadAvailable(gp)) return;

    // Left stick X = steering
    float axis_x = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_X);
    if (fabsf(axis_x) > gamepad_cfg_.deadzone_left) {
        float normalized = (fabsf(axis_x) - gamepad_cfg_.deadzone_left) /
                           (1.0f - gamepad_cfg_.deadzone_left);
        state_.steering += (axis_x > 0 ? 1.0f : -1.0f) * normalized;
    }
    state_.steering = std::clamp(state_.steering, -1.0f, 1.0f);

    // Right trigger = gas, Left trigger = brake
    float rt = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_RIGHT_TRIGGER);
    float lt = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_TRIGGER);
    // Triggers go from -1 (released) to 1 (pressed) on many controllers
    float gas_val = std::clamp((rt + 1.0f) * 0.5f, 0.0f, 1.0f);
    float brake_val = std::clamp((lt + 1.0f) * 0.5f, 0.0f, 1.0f);
    state_.gas = std::max(state_.gas, gas_val);
    state_.brake = std::max(state_.brake, brake_val);

    // Buttons
    if (IsGamepadButtonDown(gp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))  state_.handbrake = true;
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) state_.camera_cycle = true;
    if (IsGamepadButtonDown(gp, GAMEPAD_BUTTON_RIGHT_FACE_UP))    state_.rear_view = true;
    if (IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN))   state_.push_to_talk = true;
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_MIDDLE_RIGHT))  state_.toggle_settings = true;
    if (IsGamepadButtonDown(gp, GAMEPAD_BUTTON_MIDDLE_LEFT))      state_.show_leaderboard = true;
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) state_.horn = true;

    // D-pad for gear shifting
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_UP))    state_.shift_park = true;
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  state_.shift_reverse = true;
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) state_.shift_neutral = true;
    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  state_.shift_drive = true;
}

void InputSystem::updateTouch(float dt) {
    updateTouchWheel(dt);
    updateTouchPedals();
    updateTouchGearLever();
}

void InputSystem::updateTouchWheel(float dt) {
    int touch_count = GetTouchPointCount();

    // Find touch on steering wheel
    bool found_touch = false;
    for (int i = 0; i < touch_count; ++i) {
        Vector2 pos = GetTouchPosition(i);
        int tid = GetTouchPointId(i);

        // Check if touch is in wheel area
        float dx = pos.x - touch_wheel_.center.x;
        float dy = pos.y - touch_wheel_.center.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < touch_wheel_.radius * 1.5f) {
            // Calculate angle using atan2 delta
            float angle = atan2f(dy, dx);
            if (touch_wheel_.active && touch_wheel_.touch_id == tid) {
                // Delta from previous angle
                float delta = angle - touch_wheel_.angle;
                // Normalize delta
                while (delta > 3.14159f)  delta -= 6.28318f;
                while (delta < -3.14159f) delta += 6.28318f;
                touch_wheel_.target_angle += delta;
                // Clamp rotation
                touch_wheel_.target_angle = std::clamp(touch_wheel_.target_angle,
                                                        -1.57f, 1.57f);  // ±90°
            }
            touch_wheel_.angle = angle;
            touch_wheel_.touch_id = tid;
            touch_wheel_.active = true;
            found_touch = true;
            break;
        }
    }

    if (!found_touch) {
        touch_wheel_.active = false;
        touch_wheel_.touch_id = -1;
        // Spring-back to center
        touch_wheel_.target_angle *= (1.0f - touch_wheel_.spring_back_speed * dt);
        if (fabsf(touch_wheel_.target_angle) < 0.01f) touch_wheel_.target_angle = 0.0f;
    }

    // Map wheel angle to steering (-1..1)
    state_.steering += touch_wheel_.target_angle / 1.57f;
    state_.steering = std::clamp(state_.steering, -1.0f, 1.0f);
}

void InputSystem::updateTouchPedals() {
    int touch_count = GetTouchPointCount();

    touch_pedals_.gas_pressed = false;
    touch_pedals_.brake_pressed = false;
    touch_pedals_.gas_value = 0.0f;
    touch_pedals_.brake_value = 0.0f;

    for (int i = 0; i < touch_count; ++i) {
        Vector2 pos = GetTouchPosition(i);

        if (CheckCollisionPointRec(pos, touch_pedals_.gas_rect)) {
            touch_pedals_.gas_pressed = true;
            // Analog: vertical position in pedal
            float norm = 1.0f - (pos.y - touch_pedals_.gas_rect.y) / touch_pedals_.gas_rect.height;
            touch_pedals_.gas_value = std::clamp(norm, 0.0f, 1.0f);
        }
        if (CheckCollisionPointRec(pos, touch_pedals_.brake_rect)) {
            touch_pedals_.brake_pressed = true;
            float norm = 1.0f - (pos.y - touch_pedals_.brake_rect.y) / touch_pedals_.brake_rect.height;
            touch_pedals_.brake_value = std::clamp(norm, 0.0f, 1.0f);
        }
    }

    state_.gas = std::max(state_.gas, touch_pedals_.gas_value);
    state_.brake = std::max(state_.brake, touch_pedals_.brake_value);
}

void InputSystem::updateTouchGearLever() {
    int touch_count = GetTouchPointCount();

    for (int i = 0; i < touch_count; ++i) {
        Vector2 pos = GetTouchPosition(i);

        if (CheckCollisionPointRec(pos, touch_gear_.bounds)) {
            // Map Y position to gear slot (0=P at top, 3=D at bottom)
            float relative_y = (pos.y - touch_gear_.bounds.y) / touch_gear_.bounds.height;
            int slot = static_cast<int>(relative_y * 4.0f);
            slot = std::clamp(slot, 0, 3);

            if (slot != touch_gear_.current_slot) {
                touch_gear_.current_slot = slot;
                switch (slot) {
                    case 0: state_.shift_park = true; break;
                    case 1: state_.shift_reverse = true; break;
                    case 2: state_.shift_neutral = true; break;
                    case 3: state_.shift_drive = true; break;
                }
            }
            break;
        }
    }
}

void InputSystem::setupTouchLayout(int screen_w, int screen_h) {
    float scale = static_cast<float>(screen_h) / 720.0f;

    // Steering wheel — bottom left
    touch_wheel_.center = {130 * scale, screen_h - 180.0f * scale};
    touch_wheel_.radius = 100.0f * scale;

    // Gas pedal — bottom right
    float pedal_w = 80 * scale;
    float pedal_h = 160 * scale;
    touch_pedals_.gas_rect = {
        screen_w - pedal_w - 20 * scale,
        screen_h - pedal_h - 30 * scale,
        pedal_w, pedal_h
    };
    // Brake pedal — left of gas
    touch_pedals_.brake_rect = {
        screen_w - 2 * pedal_w - 40 * scale,
        screen_h - pedal_h - 30 * scale,
        pedal_w, pedal_h
    };

    // Gear lever — right of steering wheel
    touch_gear_.bounds = {
        280 * scale,
        screen_h - 200.0f * scale,
        50 * scale,
        170 * scale
    };
}

bool InputSystem::gamepadConnected() const {
    return IsGamepadAvailable(gamepad_cfg_.gamepad_id);
}

InputSystem::CompactFrame InputSystem::compactFrame() const {
    CompactFrame f;
    f.steering = static_cast<int8_t>(state_.steering * 127.0f);
    uint8_t gas_nib = static_cast<uint8_t>(state_.gas * 15.0f) & 0x0F;
    uint8_t brk_nib = static_cast<uint8_t>(state_.brake * 15.0f) & 0x0F;
    f.gas_brake = (gas_nib << 4) | brk_nib;
    f.gear = 3; // Default Drive
    if (state_.shift_park)    f.gear = 0;
    if (state_.shift_reverse) f.gear = 1;
    if (state_.shift_neutral) f.gear = 2;
    if (state_.handbrake) f.gear |= 0x04;
    if (state_.indicator_left) f.gear |= 0x08;
    if (state_.indicator_right) f.gear |= 0x10;
    if (state_.hazards) f.gear |= 0x20;
    return f;
}

} // namespace drt
