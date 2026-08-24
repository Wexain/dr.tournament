// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — HUD Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "render/hud.h"
#include "core/engine.h"
#include <raymath.h>
#include <cmath>
#include <cstdio>

namespace drt {

void HUD::init(int screen_w, int screen_h) {
    screen_w_ = screen_w;
    screen_h_ = screen_h;
    scale_ = static_cast<float>(screen_h) / 720.0f;
}

void HUD::onResize(int screen_w, int screen_h) {
    screen_w_ = screen_w;
    screen_h_ = screen_h;
    scale_ = static_cast<float>(screen_h) / 720.0f;
}

void HUD::render(const Engine& engine) {
    if (engine.state() != GameState::RACING && engine.state() != GameState::SPECTATING)
        return;

    const auto& vehicle = engine.playerVehicle();
    float speed_kmh = vehicle.speedKmh();
    float rpm = vehicle.transmission().engineRPM();
    float redline = vehicle.transmission().torqueCurve().redline_rpm;
    uint8_t gear_pos = static_cast<uint8_t>(vehicle.transmission().position());
    int drive_gear = vehicle.transmission().currentDriveGear();

    // Speedometer (bottom-right)
    renderSpeedometer(speed_kmh, 200.0f);

    // Tachometer (left of speedo)
    renderTachometer(rpm, redline);

    // Gear indicator
    renderGearIndicator(gear_pos, drive_gear);

    // Connection info
    renderConnectionInfo(engine.netClient().pingMs(), engine.fps());

    // Leaderboard
    if (show_leaderboard_) {
        renderLeaderboard();
    }

    // Voice indicators
    renderVoiceIndicators();
}

void HUD::renderSpeedometer(float speed_kmh, float max_speed) {
    float radius = 80.0f * scale_;
    Vector2 center = {screen_w_ - radius - 30.0f * scale_,
                      screen_h_ - radius - 30.0f * scale_};

    // Background circle
    DrawCircleV(center, radius + 4, {0, 0, 0, 120});
    DrawCircleLinesV(center, radius, {80, 100, 140, 200});

    // Speed arc
    float angle = -225.0f + (speed_kmh / max_speed) * 270.0f;
    angle = std::clamp(angle, -225.0f, 45.0f);
    drawGaugeNeedle(center, radius * 0.85f, angle, {0, 180, 255, 255}, 2.5f * scale_);

    // Speed text
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", speed_kmh);
    int tw = MeasureText(buf, (int)(28 * scale_));
    DrawText(buf, (int)(center.x - tw / 2), (int)(center.y - 8 * scale_),
             (int)(28 * scale_), WHITE);

    DrawText("km/h", (int)(center.x - MeasureText("km/h", (int)(12 * scale_)) / 2),
             (int)(center.y + 16 * scale_), (int)(12 * scale_), {150, 150, 170, 200});
}

void HUD::renderTachometer(float rpm, float redline) {
    float radius = 55.0f * scale_;
    Vector2 center = {screen_w_ - 230.0f * scale_,
                      screen_h_ - radius - 50.0f * scale_};

    DrawCircleV(center, radius + 3, {0, 0, 0, 100});
    DrawCircleLinesV(center, radius, {80, 80, 100, 180});

    // RPM needle
    float angle = -225.0f + (rpm / redline) * 270.0f;
    Color needle_col = (rpm > redline * 0.85f) ? Color{255, 60, 60, 255}
                                                : Color{0, 200, 100, 255};
    drawGaugeNeedle(center, radius * 0.8f, angle, needle_col, 2.0f * scale_);

    // RPM text
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", rpm);
    int tw = MeasureText(buf, (int)(16 * scale_));
    DrawText(buf, (int)(center.x - tw / 2), (int)(center.y - 4 * scale_),
             (int)(16 * scale_), WHITE);

    DrawText("RPM", (int)(center.x - MeasureText("RPM", (int)(10 * scale_)) / 2),
             (int)(center.y + 12 * scale_), (int)(10 * scale_), {140, 140, 160, 180});
}

void HUD::renderGearIndicator(uint8_t gear_pos, int drive_gear) {
    float x = screen_w_ - 150.0f * scale_;
    float y = screen_h_ - 200.0f * scale_;

    Rectangle panel = {x - 25 * scale_, y - 5 * scale_, 50 * scale_, 120 * scale_};
    drawRoundedPanel(panel, {20, 20, 30, 180});

    const char* gears[] = {"P", "R", "N", "D"};
    Color colors[] = {
        {200, 200, 200, 255},  // P
        {255, 100, 100, 255},  // R
        {200, 200, 200, 255},  // N
        {100, 255, 100, 255}   // D
    };

    for (int i = 0; i < 4; ++i) {
        float gy = y + i * 28 * scale_;
        bool selected = (i == gear_pos);
        Color col = selected ? colors[i] : Color{80, 80, 100, 150};
        int fs = selected ? (int)(22 * scale_) : (int)(16 * scale_);
        int tw = MeasureText(gears[i], fs);
        DrawText(gears[i], (int)(x - tw / 2), (int)gy, fs, col);
    }

    // Drive gear number
    if (gear_pos == 3) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", drive_gear + 1);
        DrawText(buf, (int)(x + 22 * scale_), (int)(y + 3 * 28 * scale_),
                 (int)(14 * scale_), {80, 180, 80, 200});
    }
}

void HUD::renderMinimap(Vector3 player_pos, float heading) {
    float size = 120.0f * scale_;
    float x = 20.0f * scale_;
    float y = 20.0f * scale_;

    DrawRectangle((int)x, (int)y, (int)size, (int)size, {0, 0, 0, 100});
    DrawRectangleLines((int)x, (int)y, (int)size, (int)size, {80, 80, 100, 180});

    // Player dot
    Vector2 center = {x + size * 0.5f, y + size * 0.5f};
    DrawCircleV(center, 4 * scale_, {0, 200, 255, 255});

    // Direction arrow
    float rad = heading * DEG2RAD;
    Vector2 arrow_tip = {center.x + sinf(rad) * 12 * scale_,
                         center.y - cosf(rad) * 12 * scale_};
    DrawLineEx(center, arrow_tip, 2 * scale_, {0, 200, 255, 255});
}

void HUD::renderLeaderboard() {
    float panel_w = 320 * scale_;
    float panel_h = (leaderboard_count_ + 1) * 30 * scale_ + 20 * scale_;
    float x = screen_w_ / 2 - panel_w / 2;
    float y = 60 * scale_;

    drawRoundedPanel({x, y, panel_w, panel_h}, {10, 10, 20, 200});

    // Header
    DrawText("LEADERBOARD", (int)(x + 10 * scale_), (int)(y + 8 * scale_),
             (int)(16 * scale_), {100, 180, 255, 255});
    DrawText("PING", (int)(x + panel_w - 60 * scale_), (int)(y + 8 * scale_),
             (int)(12 * scale_), {100, 100, 120, 200});

    for (int i = 0; i < leaderboard_count_; ++i) {
        float ly = y + (i + 1) * 30 * scale_ + 10 * scale_;
        const auto& entry = leaderboard_[i];

        Color name_col = entry.is_local ? Color{0, 220, 255, 255} :
                          entry.is_mod ? Color{0, 255, 200, 255} :
                          Color{200, 200, 210, 255};

        // Rank
        DrawText(TextFormat("%d.", i + 1), (int)(x + 10 * scale_), (int)ly,
                 (int)(16 * scale_), {150, 150, 170, 200});

        // Mod badge
        if (entry.is_mod) {
            DrawText("[MOD]", (int)(x + 35 * scale_), (int)ly,
                     (int)(12 * scale_), {0, 220, 200, 255});
        }

        // Name
        float name_x = entry.is_mod ? x + 80 * scale_ : x + 35 * scale_;
        DrawText(entry.nickname.c_str(), (int)name_x, (int)ly,
                 (int)(16 * scale_), name_col);

        // Ping
        Color ping_col = entry.ping_ms < 60 ? Color{0, 200, 100, 255} :
                          entry.ping_ms < 120 ? Color{255, 200, 0, 255} :
                          Color{255, 60, 60, 255};
        DrawText(TextFormat("%dms", entry.ping_ms),
                 (int)(x + panel_w - 60 * scale_), (int)ly,
                 (int)(12 * scale_), ping_col);
    }
}

void HUD::renderNotifications(float dt) {
    float y = screen_h_ * 0.3f;
    for (auto& notif : notifications_) {
        if (notif.timer <= 0) continue;
        notif.timer -= dt;
        float alpha = std::min(notif.timer, 1.0f);
        int a = (int)(alpha * 255);
        int tw = MeasureText(notif.text.c_str(), 20);
        DrawText(notif.text.c_str(), screen_w_ / 2 - tw / 2, (int)y,
                 20, {255, 255, 255, (unsigned char)a});
        y += 30;
    }
}

void HUD::renderVoiceIndicators() {
    float x = 20 * scale_;
    float y = screen_h_ - 200 * scale_;

    for (int i = 0; i < 16; ++i) {
        if (!voice_speaking_[i]) continue;

        // Speaking indicator (pulsing green dot)
        float pulse = sinf(GetTime() * 8.0f) * 0.3f + 0.7f;
        DrawCircle((int)x, (int)(y + i * 20 * scale_), 5 * scale_,
                   {0, (unsigned char)(200 * pulse), 0, 255});
    }
}

void HUD::renderConnectionInfo(int ping, int fps) {
    Color ping_col = ping < 60 ? Color{0, 200, 100, 200} :
                     ping < 120 ? Color{255, 200, 0, 200} :
                     Color{255, 60, 60, 200};

    DrawText(TextFormat("FPS: %d", fps), 10, screen_h_ - 20, 14, {100, 100, 120, 200});
    if (ping > 0) {
        DrawText(TextFormat("Ping: %dms", ping), 100, screen_h_ - 20, 14, ping_col);
    }
}

void HUD::drawGaugeNeedle(Vector2 center, float radius, float angle,
                           Color color, float thickness) {
    float rad = angle * DEG2RAD;
    Vector2 tip = {center.x + cosf(rad) * radius,
                   center.y + sinf(rad) * radius};
    DrawLineEx(center, tip, thickness, color);
}

void HUD::drawRoundedPanel(Rectangle rect, Color bg, float roundness) {
    DrawRectangleRounded(rect, roundness, 8, bg);
    DrawRectangleRoundedLines(rect, roundness, 8, {60, 60, 80, 100});
}

void HUD::showNotification(const std::string& text, float duration) {
    for (auto& notif : notifications_) {
        if (notif.timer <= 0) {
            notif.text = text;
            notif.timer = duration;
            notif.duration = duration;
            return;
        }
    }
}

void HUD::setLeaderboard(const LeaderboardEntry* entries, int count) {
    leaderboard_count_ = std::min(count, 16);
    for (int i = 0; i < leaderboard_count_; ++i) {
        leaderboard_[i] = entries[i];
    }
}

void HUD::setVoiceSpeaking(int player_index, bool speaking) {
    if (player_index >= 0 && player_index < 16) {
        voice_speaking_[player_index] = speaking;
    }
}

} // namespace drt
