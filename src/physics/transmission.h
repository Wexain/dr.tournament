#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Transmission State Machine (P-R-N-D)
// ═══════════════════════════════════════════════════════════════════════════
#include <cstdint>
#include <cmath>

namespace drt {

enum class GearPosition : uint8_t {
    PARK    = 0,
    REVERSE = 1,
    NEUTRAL = 2,
    DRIVE   = 3
};

// Torque curve parameters for the engine
struct EngineTorqueCurve {
    float idle_rpm     = 800.0f;
    float peak_rpm     = 5500.0f;
    float redline_rpm  = 7000.0f;
    float max_torque   = 350.0f;    // Nm at peak_rpm
    float idle_torque  = 100.0f;
    
    // Evaluate torque at given RPM
    [[nodiscard]] float evaluate(float rpm) const {
        if (rpm < idle_rpm) return idle_torque;
        if (rpm > redline_rpm) return 0.0f;
        // Parabolic curve peaking at peak_rpm
        float t = (rpm - idle_rpm) / (peak_rpm - idle_rpm);
        if (t <= 1.0f) {
            return idle_torque + (max_torque - idle_torque) * (2.0f * t - t * t);
        }
        // Fall-off after peak
        float f = (rpm - peak_rpm) / (redline_rpm - peak_rpm);
        return max_torque * (1.0f - f * f);
    }
};

struct GearRatios {
    float reverse  = -3.2f;
    float drive[6] = {3.8f, 2.5f, 1.7f, 1.2f, 0.9f, 0.75f};
    float final_drive = 3.5f;
    int   num_forward_gears = 6;
};

class Transmission {
public:
    void init();
    void update(float dt, float wheel_rpm, float gas_input);
    
    // Shift commands
    void shiftTo(GearPosition gear);
    void shiftUp();
    void shiftDown();
    
    // Current state
    [[nodiscard]] GearPosition position() const { return position_; }
    [[nodiscard]] int          currentDriveGear() const { return drive_gear_; }
    [[nodiscard]] float        engineRPM() const { return engine_rpm_; }
    [[nodiscard]] float        outputTorque() const { return output_torque_; }
    [[nodiscard]] float        currentGearRatio() const;
    [[nodiscard]] bool         isParked() const { return position_ == GearPosition::PARK; }
    [[nodiscard]] bool         isReverse() const { return position_ == GearPosition::REVERSE; }
    [[nodiscard]] bool         isNeutral() const { return position_ == GearPosition::NEUTRAL; }
    [[nodiscard]] bool         isDrive() const { return position_ == GearPosition::DRIVE; }
    
    // Configuration
    EngineTorqueCurve& torqueCurve() { return torque_curve_; }
    GearRatios&        gearRatios()  { return gear_ratios_; }

    // For display
    [[nodiscard]] const char* positionString() const;
    [[nodiscard]] const char* driveGearString() const;

private:
    void autoShiftLogic(float wheel_rpm, float gas_input);
    
    GearPosition      position_     = GearPosition::PARK;
    int               drive_gear_   = 0;  // 0-based index into gear_ratios_.drive[]
    float             engine_rpm_   = 800.0f;
    float             output_torque_ = 0.0f;
    float             clutch_engagement_ = 1.0f;
    
    EngineTorqueCurve torque_curve_;
    GearRatios        gear_ratios_;
    
    // Auto-shift thresholds
    float shift_up_rpm_   = 6000.0f;
    float shift_down_rpm_ = 2000.0f;
    float shift_cooldown_ = 0.0f;
};

} // namespace drt
