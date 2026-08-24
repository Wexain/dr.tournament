#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Diamond Economy
// ═══════════════════════════════════════════════════════════════════════════
#include <cstdint>
#include <string>
#include <vector>

namespace drt {

// ── Diamond Rewards ────────────────────────────────────────────────────────
struct DiamondReward {
    static constexpr int WINNER      = 3;
    static constexpr int RUNNER_UP   = 1;
    static constexpr int PERFECT_RUN = 1;  // No contacts
};

// ── Unlock Categories ──────────────────────────────────────────────────────
enum class UnlockType : uint8_t {
    CAR_MODEL,
    BODY_COLOR,
    UNDERGLOW,
    WHEEL_STYLE,
    HORN_SOUND
};

struct UnlockItem {
    UnlockType  type;
    std::string name;
    std::string id;
    int         cost_diamonds = 0;
    bool        unlocked = false;
};

class Economy {
public:
    void init();
    
    // Diamond management
    void awardDiamonds(int amount, const std::string& reason);
    [[nodiscard]] int diamonds() const { return diamonds_; }
    
    // Race results
    void processRaceResult(int position, int total_players, bool perfect_run);
    
    // Black Market
    [[nodiscard]] const std::vector<UnlockItem>& shopItems() const { return shop_; }
    bool purchaseItem(const std::string& item_id);
    [[nodiscard]] bool isUnlocked(const std::string& item_id) const;
    
    // Persistence
    bool save() const;
    bool load();

private:
    int diamonds_ = 0;
    std::vector<UnlockItem> shop_;
    std::vector<std::string> purchase_log_;
    
    void initShopItems();
};

} // namespace drt
