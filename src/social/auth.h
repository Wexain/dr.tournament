#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Auth System
// ═══════════════════════════════════════════════════════════════════════════
#include <string>
#include <cstdint>
#include <functional>

namespace drt {

enum class AuthTier : uint8_t {
    GUEST    = 0,   // Local UUID + Nickname
    PIN_AUTH = 1,   // Username + 4-digit PIN (bcrypt/SQLite)
    DISCORD  = 2    // Discord OAuth2 linked
};

struct UserProfile {
    std::string uuid;
    std::string nickname;
    std::string username;      // Tier 2+
    std::string discord_id;    // Tier 3
    AuthTier    tier = AuthTier::GUEST;
    int         diamonds = 0;
    int         wins = 0;
    int         races = 0;
    uint8_t     car_model = 0;
    bool        is_mod = false;
};

class AuthSystem {
public:
    void init();
    
    // Tier 1: Guest
    void loginAsGuest(const std::string& nickname);
    
    // Tier 2: PIN auth
    bool registerPIN(const std::string& username, const std::string& pin);
    bool loginPIN(const std::string& username, const std::string& pin);
    
    // Tier 3: Discord (scaffolded)
    void beginDiscordOAuth(const std::string& client_id, const std::string& redirect_uri);
    void completeDiscordOAuth(const std::string& auth_code);
    
    // Profile access
    [[nodiscard]] const UserProfile& profile() const { return profile_; }
    [[nodiscard]] UserProfile& profile() { return profile_; }
    [[nodiscard]] bool isLoggedIn() const { return logged_in_; }
    [[nodiscard]] AuthTier tier() const { return profile_.tier; }
    
    // Persistence
    bool saveLocal() const;
    bool loadLocal();
    
private:
    std::string generateUUID() const;
    std::string hashPIN(const std::string& pin) const;  // Simple hash (bcrypt scaffolded)
    
    UserProfile profile_;
    bool logged_in_ = false;
};

} // namespace drt
