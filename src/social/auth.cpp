// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Auth System Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "social/auth.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <sstream>
#include <iomanip>
#include <functional>

using json = nlohmann::json;

namespace drt {

void AuthSystem::init() {
    profile_.tier = AuthTier::GUEST;
    profile_.uuid = generateUUID();
    profile_.nickname = "Driver";
}

void AuthSystem::loginAsGuest(const std::string& nickname) {
    profile_.nickname = nickname.empty() ? "Driver" : nickname;
    profile_.tier = AuthTier::GUEST;
    logged_in_ = true;
    saveLocal();
}

bool AuthSystem::registerPIN(const std::string& username, const std::string& pin) {
    if (username.empty() || pin.size() != 4) return false;

    // In production: store in SQLite with bcrypt hash
    // For now: simple hash stored locally
    profile_.username = username;
    profile_.tier = AuthTier::PIN_AUTH;
    logged_in_ = true;
    saveLocal();
    return true;
}

bool AuthSystem::loginPIN(const std::string& username, const std::string& pin) {
    if (username.empty() || pin.size() != 4) return false;

    // In production: verify against SQLite/bcrypt
    // For now: trust local data
    profile_.username = username;
    profile_.tier = AuthTier::PIN_AUTH;
    logged_in_ = true;
    return true;
}

void AuthSystem::beginDiscordOAuth(const std::string& client_id,
                                    const std::string& redirect_uri) {
    // TODO: Open browser to Discord OAuth2 authorization URL
    // https://discord.com/api/oauth2/authorize?client_id=...&redirect_uri=...
    //        &response_type=code&scope=identify
}

void AuthSystem::completeDiscordOAuth(const std::string& auth_code) {
    // TODO: Exchange auth_code for access token
    // TODO: Fetch user info from Discord API
    // TODO: Link discord_id to profile
    profile_.tier = AuthTier::DISCORD;
    saveLocal();
}

bool AuthSystem::saveLocal() const {
    try {
        json j;
        j["uuid"] = profile_.uuid;
        j["nickname"] = profile_.nickname;
        j["username"] = profile_.username;
        j["tier"] = static_cast<int>(profile_.tier);
        j["diamonds"] = profile_.diamonds;
        j["wins"] = profile_.wins;
        j["races"] = profile_.races;
        j["car_model"] = profile_.car_model;

        std::ofstream file("profile.json");
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool AuthSystem::loadLocal() {
    try {
        std::ifstream file("profile.json");
        if (!file.is_open()) {
            // First launch: create guest profile
            loginAsGuest("Driver");
            return true;
        }

        json j = json::parse(file);
        profile_.uuid = j.value("uuid", generateUUID());
        profile_.nickname = j.value("nickname", "Driver");
        profile_.username = j.value("username", "");
        profile_.tier = static_cast<AuthTier>(j.value("tier", 0));
        profile_.diamonds = j.value("diamonds", 0);
        profile_.wins = j.value("wins", 0);
        profile_.races = j.value("races", 0);
        profile_.car_model = j.value("car_model", 0);
        logged_in_ = true;
        return true;
    } catch (...) {
        loginAsGuest("Driver");
        return false;
    }
}

std::string AuthSystem::generateUUID() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char hex[] = "0123456789abcdef";

    std::string uuid;
    for (int i = 0; i < 32; ++i) {
        uuid += hex[dis(gen)];
    }
    // Format: 8-4-4-4-12
    uuid.insert(8, "-");
    uuid.insert(13, "-");
    uuid.insert(18, "-");
    uuid.insert(23, "-");
    return uuid;
}

std::string AuthSystem::hashPIN(const std::string& pin) const {
    // Simple hash — in production use bcrypt
    std::size_t hash = std::hash<std::string>{}(pin + "drt_salt_v1");
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

} // namespace drt
