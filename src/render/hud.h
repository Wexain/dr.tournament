#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — HUD & UI System
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <string>
#include <array>
#include <cstdint>

namespace drt {

class Engine; // forward decl

// ── Leaderboard Entry ──────────────────────────────────────────────────────
struct LeaderboardEntry {
    std::string nickname;
    float       distance    = 0.0f;
    int         ping_ms     = 0;
    bool        is_mod      = false;
    bool        is_local    = false;
    int         diamonds    = 0;
    uint8_t     gear        = 3;
};

class HUD {
public:
    void init(int screen_w, int screen_h);
    void render(const Engine& engine);
    void onResize(int screen_w, int screen_h);

    // Leaderboard
    void setLeaderboard(const LeaderboardEntry* entries, int count);
    void setShowLeaderboard(bool show) { show_leaderboard_ = show; }

    // Notifications
    void showNotification(const std::string& text, float duration = 3.0f);

    // Voice chat indicators
    void setVoiceSpeaking(int player_index, bool speaking);

private:
    void renderSpeedometer(float speed_kmh, float max_speed);
    void renderTachometer(float rpm, float redline);
    void renderGearIndicator(uint8_t gear_pos, int drive_gear);
    void renderMinimap(Vector3 player_pos, float heading);
    void renderLeaderboard();
    void renderNotifications(float dt);
    void renderVoiceIndicators();
    void renderConnectionInfo(int ping, int fps);

    // Draw helpers
    void drawGaugeNeedle(Vector2 center, float radius, float angle,
                         Color color, float thickness);
    void drawRoundedPanel(Rectangle rect, Color bg, float roundness = 0.1f);

    int screen_w_ = 1280;
    int screen_h_ = 720;
    float scale_  = 1.0f;

    // Leaderboard
    std::array<LeaderboardEntry, 16> leaderboard_{};
    int  leaderboard_count_ = 0;
    bool show_leaderboard_  = false;

    // Notifications
    struct Notification {
        std::string text;
        float       timer = 0.0f;
        float       duration = 3.0f;
    };
    std::array<Notification, 5> notifications_{};

    // Voice indicators
    std::array<bool, 16> voice_speaking_{};

    Font ui_font_ = {};
    bool font_loaded_ = false;
};

} // namespace drt
