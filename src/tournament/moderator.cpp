// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Moderator System Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "tournament/moderator.h"
#include "core/engine.h"
#include <raylib.h>
#include <cmath>

namespace drt {

void ModeratorSystem::init() {
    verified_ = false;
    spectating_ = false;
}

void ModeratorSystem::renderModBadge(float x, float y, float scale) {
    if (!verified_) return;

    // Glowing cyan/gold shield badge
    float pulse = sinf(static_cast<float>(GetTime()) * 3.0f) * 0.2f + 0.8f;

    // Shield shape (simplified as rounded rectangle)
    float w = 50 * scale;
    float h = 24 * scale;
    Rectangle badge = {x, y, w, h};

    // Glow
    DrawRectangleRounded({x - 2, y - 2, w + 4, h + 4}, 0.3f, 8,
                         {0, (unsigned char)(200 * pulse), (unsigned char)(200 * pulse), 80});

    // Badge background
    DrawRectangleRounded(badge, 0.3f, 8, {10, 30, 50, 220});
    DrawRectangleRoundedLines(badge, 0.3f, 8,
                              {0, (unsigned char)(220 * pulse), (unsigned char)(180 * pulse), 255});

    // Text
    int fs = static_cast<int>(14 * scale);
    DrawText("[MOD]", static_cast<int>(x + 6 * scale), static_cast<int>(y + 4 * scale),
             fs, {(unsigned char)(200 * pulse + 55), 220, 180, 255});
}

void ModeratorSystem::enableSpectatorMode(Engine& engine) {
    if (!verified_) return;
    spectating_ = true;
    engine.camera().setMode(CameraMode::FREE_CAM);
}

void ModeratorSystem::disableSpectatorMode(Engine& engine) {
    spectating_ = false;
    engine.camera().setMode(CameraMode::CHASE);
}

} // namespace drt
