#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Tournament Rules
// ═══════════════════════════════════════════════════════════════════════════
#include <cstdint>
#include <string>
#include <algorithm>

namespace drt {

// Surface friction presets
struct SurfacePreset {
    static constexpr float DRY = 1.0f;
    static constexpr float WET = 0.65f;
    static constexpr float OIL = 0.35f;
};

class TournamentRules {
public:
    void init();
    void applyDefaults();

    // Host rule configuration
    void setLaneCount(int lanes)           { lane_count_ = std::clamp(lanes, 1, 6); }
    void setSpecLock(bool on)              { spec_lock_ = on; }
    void setTransmissionLock(bool on)      { transmission_lock_ = on; }
    void setZeroToleranceDNF(bool on)      { zero_tolerance_ = on; }
    void setTrafficDensity(float pct)      { traffic_density_ = std::clamp(pct, 0.0f, 1.0f); }
    void setSurfaceFriction(float f)       { surface_friction_ = f; }
    void setForcedCar(uint8_t model)       { forced_car_ = model; }

    // Accessors
    [[nodiscard]] int   laneCount()        const { return lane_count_; }
    [[nodiscard]] bool  specLock()         const { return spec_lock_; }
    [[nodiscard]] bool  transmissionLock() const { return transmission_lock_; }
    [[nodiscard]] bool  zeroTolerance()    const { return zero_tolerance_; }
    [[nodiscard]] float trafficDensity()   const { return traffic_density_; }
    [[nodiscard]] float surfaceFriction()  const { return surface_friction_; }
    [[nodiscard]] uint8_t forcedCar()      const { return forced_car_; }

    // Surface preset helpers
    [[nodiscard]] const char* surfaceName() const {
        if (surface_friction_ >= 0.9f) return "Dry";
        if (surface_friction_ >= 0.5f) return "Wet";
        return "Oil";
    }

private:
    int     lane_count_       = 3;
    bool    spec_lock_        = false;
    bool    transmission_lock_ = false;
    bool    zero_tolerance_   = false;
    float   traffic_density_  = 0.5f;
    float   surface_friction_ = 1.0f;
    uint8_t forced_car_       = 0xFF;  // 0xFF = player choice
};

} // namespace drt
