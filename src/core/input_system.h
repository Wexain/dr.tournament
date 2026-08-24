#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Input System
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>
#include <array>

namespace drt {

// ── Input Actions (platform-agnostic) ──────────────────────────────────────
struct InputState {
    // Driving
    float steering    = 0.0f;   // -1.0 (left) to +1.0 (right)
    float gas         = 0.0f;   // 0.0 to 1.0
    float brake       = 0.0f;   // 0.0 to 1.0
    bool  handbrake   = false;

    // Gear selection
    bool  shift_park    = false;
    bool  shift_reverse = false;
    bool  shift_neutral = false;
    bool  shift_drive   = false;

    // Indicators & horn
    bool  indicator_left  = false;
    bool  indicator_right = false;
    bool  hazards         = false;
    bool  horn            = false;

    // Camera
    bool  camera_cycle    = false;
    bool  rear_view       = false;

    // Communication
    bool  push_to_talk    = false;

    // UI
    bool  show_leaderboard = false;
    bool  toggle_settings  = false;

    // Menu navigation
    bool  menu_up    = false;
    bool  menu_down  = false;
    bool  menu_left  = false;
    bool  menu_right = false;
    bool  menu_confirm = false;
    bool  menu_back   = false;
};

// ── Gamepad Config ─────────────────────────────────────────────────────────
struct GamepadConfig {
    float deadzone_left  = 0.15f;
    float deadzone_right = 0.15f;
    int   gamepad_id     = 0;
    bool  enabled        = true;
};

// ── Touch Steering Wheel State (Android) ───────────────────────────────────
struct TouchWheelState {
    bool  active        = false;
    float angle         = 0.0f;  // Current wheel angle in radians
    float target_angle  = 0.0f;
    float spring_back_speed = 5.0f;
    Vector2 center      = {0, 0};
    float radius        = 100.0f;
    int   touch_id      = -1;
};

struct TouchPedalState {
    bool  gas_pressed   = false;
    bool  brake_pressed = false;
    float gas_value     = 0.0f;
    float brake_value   = 0.0f;
    Rectangle gas_rect  = {};
    Rectangle brake_rect = {};
};

struct TouchGearLever {
    bool  active       = false;
    int   current_slot = 3;  // 0=P, 1=R, 2=N, 3=D
    float y_position   = 0.0f;
    Rectangle bounds   = {};
    int   touch_id     = -1;
};

// ── Input System ───────────────────────────────────────────────────────────
class InputSystem {
public:
    void init(int screen_w, int screen_h);
    void update(float dt);

    [[nodiscard]] const InputState& current() const { return state_; }
    [[nodiscard]] const InputState& previous() const { return prev_state_; }

    // Gamepad
    void setGamepadConfig(const GamepadConfig& cfg) { gamepad_cfg_ = cfg; }
    [[nodiscard]] const GamepadConfig& gamepadConfig() const { return gamepad_cfg_; }
    [[nodiscard]] bool gamepadConnected() const;

    // Touch (Android)
    [[nodiscard]] const TouchWheelState& touchWheel() const { return touch_wheel_; }
    [[nodiscard]] const TouchPedalState& touchPedals() const { return touch_pedals_; }
    [[nodiscard]] const TouchGearLever&  touchGearLever() const { return touch_gear_; }

    // Key rebinding
    void setSteerLeftKey(int key)  { key_steer_left_ = key; }
    void setSteerRightKey(int key) { key_steer_right_ = key; }
    void setGasKey(int key)        { key_gas_ = key; }
    void setBrakeKey(int key)      { key_brake_ = key; }

    // Screen resize
    void onResize(int screen_w, int screen_h);

    // Compact 3-byte input frame for anti-cheat logging
    struct CompactFrame {
        int8_t  steering;   // -128..127 mapped from -1..1
        uint8_t gas_brake;  // high nibble = gas (0-15), low nibble = brake (0-15)
        uint8_t gear;       // 0=P, 1=R, 2=N, 3=D | flags in upper bits
    };
    [[nodiscard]] CompactFrame compactFrame() const;

private:
    void updateKeyboard();
    void updateGamepad();
    void updateTouch(float dt);
    void updateTouchWheel(float dt);
    void updateTouchPedals();
    void updateTouchGearLever();
    void setupTouchLayout(int screen_w, int screen_h);

    InputState     state_{};
    InputState     prev_state_{};
    GamepadConfig  gamepad_cfg_{};

    // Touch state
    TouchWheelState touch_wheel_{};
    TouchPedalState touch_pedals_{};
    TouchGearLever  touch_gear_{};

    // Configurable keys (defaults)
    int key_gas_         = KEY_W;
    int key_brake_       = KEY_S;
    int key_steer_left_  = KEY_A;
    int key_steer_right_ = KEY_D;
    int key_handbrake_   = KEY_SPACE;

    int screen_w_ = 1280;
    int screen_h_ = 720;
};

} // namespace drt
