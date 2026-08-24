#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Moderator System
// ═══════════════════════════════════════════════════════════════════════════
#include <string>
#include <cstdint>

namespace drt {

class Engine; // forward decl

class ModeratorSystem {
public:
    void init();
    
    // Badge rendering (called by HUD)
    void renderModBadge(float x, float y, float scale = 1.0f);
    
    // Free-cam spectator
    void enableSpectatorMode(Engine& engine);
    void disableSpectatorMode(Engine& engine);
    [[nodiscard]] bool isSpectating() const { return spectating_; }
    
    // Verification
    void setVerified(bool v) { verified_ = v; }
    [[nodiscard]] bool isVerified() const { return verified_; }

private:
    bool verified_   = false;
    bool spectating_ = false;
};

} // namespace drt
