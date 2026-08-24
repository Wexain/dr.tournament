// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Tournament Rules Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "tournament/rules.h"
#include <raylib.h>

namespace drt {

void TournamentRules::init() {
    applyDefaults();
}

void TournamentRules::applyDefaults() {
    lane_count_ = 3;
    spec_lock_ = false;
    transmission_lock_ = false;
    zero_tolerance_ = false;
    traffic_density_ = 0.5f;
    surface_friction_ = SurfacePreset::DRY;
    forced_car_ = 0xFF;
}

} // namespace drt
