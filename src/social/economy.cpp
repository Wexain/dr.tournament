// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Diamond Economy Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "social/economy.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace drt {

void Economy::init() {
    initShopItems();
    load();
}

void Economy::initShopItems() {
    shop_.clear();

    // Body colors
    shop_.push_back({UnlockType::BODY_COLOR, "Crimson Red",     "color_crimson",    5, false});
    shop_.push_back({UnlockType::BODY_COLOR, "Midnight Purple", "color_midnight",   8, false});
    shop_.push_back({UnlockType::BODY_COLOR, "Chrome Silver",   "color_chrome",    12, false});
    shop_.push_back({UnlockType::BODY_COLOR, "Matte Black",     "color_matte_blk", 10, false});
    shop_.push_back({UnlockType::BODY_COLOR, "Pearl White",     "color_pearl_wht",  7, false});

    // Underglow
    shop_.push_back({UnlockType::UNDERGLOW, "Neon Blue",      "glow_blue",     15, false});
    shop_.push_back({UnlockType::UNDERGLOW, "Neon Green",     "glow_green",    15, false});
    shop_.push_back({UnlockType::UNDERGLOW, "Neon Purple",    "glow_purple",   15, false});
    shop_.push_back({UnlockType::UNDERGLOW, "RGB Cycle",      "glow_rgb",      25, false});
    shop_.push_back({UnlockType::UNDERGLOW, "Fire",           "glow_fire",     30, false});

    // Wheel styles
    shop_.push_back({UnlockType::WHEEL_STYLE, "Sport Alloy",  "wheel_sport",    8, false});
    shop_.push_back({UnlockType::WHEEL_STYLE, "Deep Dish",    "wheel_deep",    10, false});
    shop_.push_back({UnlockType::WHEEL_STYLE, "Carbon Fiber", "wheel_carbon",  20, false});

    // Horn sounds
    shop_.push_back({UnlockType::HORN_SOUND, "Classic",  "horn_classic",  0, true}); // Default
    shop_.push_back({UnlockType::HORN_SOUND, "Air Horn", "horn_air",      5, false});
    shop_.push_back({UnlockType::HORN_SOUND, "Musical",  "horn_musical", 10, false});

    // Car unlocks
    shop_.push_back({UnlockType::CAR_MODEL, "Sports (McLaren)", "car_sports", 20, false});
    shop_.push_back({UnlockType::CAR_MODEL, "Super (Lambo)",    "car_super",  35, false});
    shop_.push_back({UnlockType::CAR_MODEL, "Hyper (Bugatti)",  "car_hyper",  50, false});
}

void Economy::awardDiamonds(int amount, const std::string& reason) {
    diamonds_ += amount;
    purchase_log_.push_back("+" + std::to_string(amount) + " " + reason);
    save();
}

void Economy::processRaceResult(int position, int total_players, bool perfect_run) {
    int reward = 0;
    if (position == 1) reward += DiamondReward::WINNER;
    else if (position == 2) reward += DiamondReward::RUNNER_UP;
    if (perfect_run) reward += DiamondReward::PERFECT_RUN;

    if (reward > 0) {
        awardDiamonds(reward, "Race P" + std::to_string(position) +
                      (perfect_run ? " (Perfect)" : ""));
    }
}

bool Economy::purchaseItem(const std::string& item_id) {
    for (auto& item : shop_) {
        if (item.id == item_id && !item.unlocked) {
            if (diamonds_ >= item.cost_diamonds) {
                diamonds_ -= item.cost_diamonds;
                item.unlocked = true;
                purchase_log_.push_back("-" + std::to_string(item.cost_diamonds) +
                                        " " + item.name);
                save();
                return true;
            }
        }
    }
    return false;
}

bool Economy::isUnlocked(const std::string& item_id) const {
    for (const auto& item : shop_) {
        if (item.id == item_id) return item.unlocked;
    }
    return false;
}

bool Economy::save() const {
    try {
        json j;
        j["diamonds"] = diamonds_;

        json unlocked = json::array();
        for (const auto& item : shop_) {
            if (item.unlocked) unlocked.push_back(item.id);
        }
        j["unlocked"] = unlocked;
        j["log"] = purchase_log_;

        std::ofstream file("economy.json");
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool Economy::load() {
    try {
        std::ifstream file("economy.json");
        if (!file.is_open()) return false;
        json j = json::parse(file);

        diamonds_ = j.value("diamonds", 0);

        if (j.contains("unlocked")) {
            for (const auto& id : j["unlocked"]) {
                for (auto& item : shop_) {
                    if (item.id == id.get<std::string>()) {
                        item.unlocked = true;
                    }
                }
            }
        }

        if (j.contains("log")) {
            purchase_log_.clear();
            for (const auto& entry : j["log"]) {
                purchase_log_.push_back(entry.get<std::string>());
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

} // namespace drt
