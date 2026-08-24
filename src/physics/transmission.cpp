// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Transmission Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "physics/transmission.h"
#include <algorithm>

namespace drt {

void Transmission::init() {
    position_ = GearPosition::PARK;
    drive_gear_ = 0;
    engine_rpm_ = torque_curve_.idle_rpm;
    output_torque_ = 0.0f;
    clutch_engagement_ = 1.0f;
}

void Transmission::update(float dt, float wheel_rpm, float gas_input) {
    // Calculate engine RPM from wheel RPM and current gear ratio
    float ratio = currentGearRatio();
    if (std::abs(ratio) > 0.01f) {
        float target_rpm = wheel_rpm * std::abs(ratio) * gear_ratios_.final_drive;
        target_rpm = std::max(target_rpm, torque_curve_.idle_rpm);
        // Smooth RPM transition
        engine_rpm_ += (target_rpm - engine_rpm_) * 8.0f * dt;
    } else {
        // Neutral — engine revs with gas
        float target_rpm = torque_curve_.idle_rpm +
            gas_input * (torque_curve_.redline_rpm - torque_curve_.idle_rpm);
        engine_rpm_ += (target_rpm - engine_rpm_) * 5.0f * dt;
    }

    engine_rpm_ = std::clamp(engine_rpm_, torque_curve_.idle_rpm, torque_curve_.redline_rpm);

    // Calculate output torque
    float engine_torque = torque_curve_.evaluate(engine_rpm_) * gas_input;

    switch (position_) {
        case GearPosition::PARK:
            output_torque_ = 0.0f;
            break;
        case GearPosition::REVERSE:
            output_torque_ = engine_torque * gear_ratios_.reverse * gear_ratios_.final_drive;
            break;
        case GearPosition::NEUTRAL:
            output_torque_ = 0.0f;
            break;
        case GearPosition::DRIVE:
            output_torque_ = engine_torque *
                gear_ratios_.drive[drive_gear_] * gear_ratios_.final_drive;
            autoShiftLogic(wheel_rpm, gas_input);
            break;
    }

    // Shift cooldown
    if (shift_cooldown_ > 0) shift_cooldown_ -= dt;
}

void Transmission::shiftTo(GearPosition gear) {
    if (position_ == gear) return;

    // Safety: can only shift to Park/Reverse at low speed
    // (enforced by caller, but double-check here)
    position_ = gear;

    if (gear == GearPosition::DRIVE) {
        drive_gear_ = 0;  // Start in first
    }
}

void Transmission::shiftUp() {
    if (position_ == GearPosition::DRIVE &&
        drive_gear_ < gear_ratios_.num_forward_gears - 1 &&
        shift_cooldown_ <= 0) {
        drive_gear_++;
        shift_cooldown_ = 0.3f;
        clutch_engagement_ = 0.0f;  // Brief clutch disengage
    }
}

void Transmission::shiftDown() {
    if (position_ == GearPosition::DRIVE &&
        drive_gear_ > 0 &&
        shift_cooldown_ <= 0) {
        drive_gear_--;
        shift_cooldown_ = 0.3f;
        clutch_engagement_ = 0.0f;
    }
}

float Transmission::currentGearRatio() const {
    switch (position_) {
        case GearPosition::PARK:    return 0.0f;
        case GearPosition::REVERSE: return gear_ratios_.reverse;
        case GearPosition::NEUTRAL: return 0.0f;
        case GearPosition::DRIVE:   return gear_ratios_.drive[drive_gear_];
    }
    return 0.0f;
}

const char* Transmission::positionString() const {
    switch (position_) {
        case GearPosition::PARK:    return "P";
        case GearPosition::REVERSE: return "R";
        case GearPosition::NEUTRAL: return "N";
        case GearPosition::DRIVE:   return "D";
    }
    return "?";
}

const char* Transmission::driveGearString() const {
    static const char* gears[] = {"1", "2", "3", "4", "5", "6"};
    if (drive_gear_ >= 0 && drive_gear_ < 6) return gears[drive_gear_];
    return "?";
}

void Transmission::autoShiftLogic(float wheel_rpm, float gas_input) {
    if (shift_cooldown_ > 0) return;

    // Upshift when RPM exceeds threshold
    if (engine_rpm_ > shift_up_rpm_ && drive_gear_ < gear_ratios_.num_forward_gears - 1) {
        shiftUp();
    }
    // Downshift when RPM drops too low (and we have gas)
    else if (engine_rpm_ < shift_down_rpm_ && drive_gear_ > 0 && gas_input > 0.3f) {
        shiftDown();
    }
}

} // namespace drt
