#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Android Touch UI Overlay
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>

namespace drt {

class AndroidUI {
public:
    void init(int screen_w, int screen_h);
    void render();
    void onResize(int screen_w, int screen_h);

    // Steering wheel visual
    void renderSteeringWheel(float angle, bool active);
    
    // Gas/brake pedals
    void renderPedals(float gas_value, float brake_value);
    
    // P-R-N-D gear lever
    void renderGearLever(int current_slot); // 0=P,1=R,2=N,3=D
    
    // Settings button
    bool renderSettingsButton();

    // UI element bounds (for touch hit testing)
    [[nodiscard]] Rectangle steeringWheelBounds() const;
    [[nodiscard]] Rectangle gasPedalBounds() const;
    [[nodiscard]] Rectangle brakePedalBounds() const;
    [[nodiscard]] Rectangle gearLeverBounds() const;

private:
    void drawSteeringWheelGraphic(Vector2 center, float radius, float angle);
    void drawPedalGraphic(Rectangle rect, float value, Color color, const char* label);
    void drawGearSlot(Rectangle rect, const char* label, bool selected, bool locked);

    int screen_w_ = 1280;
    int screen_h_ = 720;
    float scale_  = 1.0f;
    
    // Layout positions (calculated from screen size)
    Vector2   wheel_center_ = {};
    float     wheel_radius_ = 100.0f;
    Rectangle gas_rect_     = {};
    Rectangle brake_rect_   = {};
    Rectangle gear_rect_    = {};
};

} // namespace drt
