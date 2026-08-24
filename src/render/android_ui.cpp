// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Android Touch UI Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "render/android_ui.h"
#include <raymath.h>
#include <cmath>
#include <cstdio>

namespace drt {

void AndroidUI::init(int screen_w, int screen_h) {
    onResize(screen_w, screen_h);
}

void AndroidUI::onResize(int screen_w, int screen_h) {
    screen_w_ = screen_w;
    screen_h_ = screen_h;
    scale_ = static_cast<float>(screen_h) / 720.0f;

    wheel_radius_ = 100.0f * scale_;
    wheel_center_ = {130 * scale_, screen_h - 180.0f * scale_};

    float pedal_w = 80 * scale_;
    float pedal_h = 160 * scale_;
    gas_rect_ = {screen_w - pedal_w - 20 * scale_, screen_h - pedal_h - 30 * scale_,
                 pedal_w, pedal_h};
    brake_rect_ = {screen_w - 2 * pedal_w - 40 * scale_, screen_h - pedal_h - 30 * scale_,
                   pedal_w, pedal_h};
    gear_rect_ = {280 * scale_, screen_h - 200.0f * scale_, 50 * scale_, 170 * scale_};
}

void AndroidUI::render() {
    // This is called only on Android builds
    // Rendering is done via the specific render methods called by the engine
}

void AndroidUI::renderSteeringWheel(float angle, bool active) {
    drawSteeringWheelGraphic(wheel_center_, wheel_radius_, angle);
}

void AndroidUI::renderPedals(float gas_value, float brake_value) {
    drawPedalGraphic(gas_rect_, gas_value, {0, 180, 80, 180}, "GAS");
    drawPedalGraphic(brake_rect_, brake_value, {200, 50, 50, 180}, "BRAKE");
}

void AndroidUI::renderGearLever(int current_slot) {
    const char* labels[] = {"P", "R", "N", "D"};
    float slot_h = gear_rect_.height / 4.0f;

    DrawRectangleRounded(gear_rect_, 0.2f, 8, {20, 20, 30, 160});
    DrawRectangleRoundedLines(gear_rect_, 0.2f, 8, {60, 60, 80, 200});

    for (int i = 0; i < 4; ++i) {
        Rectangle slot = {gear_rect_.x, gear_rect_.y + i * slot_h, gear_rect_.width, slot_h};
        drawGearSlot(slot, labels[i], i == current_slot, false);
    }
}

bool AndroidUI::renderSettingsButton() {
    float btn_size = 40 * scale_;
    Rectangle btn = {screen_w_ - btn_size - 10 * scale_, 10 * scale_, btn_size, btn_size};
    DrawRectangleRounded(btn, 0.3f, 8, {30, 30, 40, 160});

    // Gear icon (simple)
    float cx = btn.x + btn_size / 2;
    float cy = btn.y + btn_size / 2;
    DrawCircle((int)cx, (int)cy, 8 * scale_, {150, 150, 170, 200});
    DrawCircle((int)cx, (int)cy, 4 * scale_, {30, 30, 40, 200});

    // Check touch
    int touch_count = GetTouchPointCount();
    for (int i = 0; i < touch_count; ++i) {
        if (CheckCollisionPointRec(GetTouchPosition(i), btn)) return true;
    }
    return false;
}

void AndroidUI::drawSteeringWheelGraphic(Vector2 center, float radius, float angle) {
    // Outer ring
    DrawCircleLinesV(center, radius, {100, 120, 150, 200});
    DrawCircleLinesV(center, radius - 8 * scale_, {80, 90, 110, 150});

    // Active glow
    DrawCircleV(center, radius, {30, 40, 55, 80});

    // Steering indicator line (rotates with wheel)
    float rad = angle;  // Already in radians
    Vector2 top = {center.x + sinf(rad) * (radius - 15 * scale_),
                   center.y - cosf(rad) * (radius - 15 * scale_)};
    DrawLineEx(center, top, 4 * scale_, {0, 180, 255, 220});
    DrawCircleV(top, 6 * scale_, {0, 180, 255, 200});

    // Center dot
    DrawCircleV(center, 10 * scale_, {50, 60, 70, 150});
}

void AndroidUI::drawPedalGraphic(Rectangle rect, float value, Color color, const char* label) {
    DrawRectangleRounded(rect, 0.15f, 8, {20, 20, 30, 140});
    DrawRectangleRoundedLines(rect, 0.15f, 8, {60, 60, 80, 180});

    // Fill based on value
    float fill_h = rect.height * value;
    Rectangle fill = {rect.x + 3, rect.y + rect.height - fill_h, rect.width - 6, fill_h};
    DrawRectangleRounded(fill, 0.1f, 8, color);

    // Label
    int tw = MeasureText(label, (int)(14 * scale_));
    DrawText(label, (int)(rect.x + rect.width / 2 - tw / 2),
             (int)(rect.y + rect.height + 5 * scale_), (int)(14 * scale_), {150, 150, 170, 200});
}

void AndroidUI::drawGearSlot(Rectangle rect, const char* label, bool selected, bool locked) {
    if (selected) {
        DrawRectangleRounded(rect, 0.3f, 8, {0, 100, 200, 120});
    }

    Color text_col = selected ? Color{255, 255, 255, 255} :
                     locked ? Color{80, 80, 80, 200} :
                     Color{150, 150, 170, 200};

    int fs = selected ? (int)(20 * scale_) : (int)(16 * scale_);
    int tw = MeasureText(label, fs);
    DrawText(label, (int)(rect.x + rect.width / 2 - tw / 2),
             (int)(rect.y + rect.height / 2 - fs / 2), fs, text_col);
}

Rectangle AndroidUI::steeringWheelBounds() const {
    return {wheel_center_.x - wheel_radius_, wheel_center_.y - wheel_radius_,
            wheel_radius_ * 2, wheel_radius_ * 2};
}

Rectangle AndroidUI::gasPedalBounds() const { return gas_rect_; }
Rectangle AndroidUI::brakePedalBounds() const { return brake_rect_; }
Rectangle AndroidUI::gearLeverBounds() const { return gear_rect_; }

} // namespace drt
