#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Raycast Vehicle Physics
// ═══════════════════════════════════════════════════════════════════════════
#include "physics/transmission.h"
#include <raylib.h>
#include <raymath.h>
#include <array>
#include <cstdint>

namespace drt {

// ── Suspension Point ───────────────────────────────────────────────────────
struct SuspensionPoint {
    // Configuration
    Vector3 local_offset;         // Attachment point relative to chassis center
    float   rest_length   = 0.5f; // L0 — natural spring length
    float   spring_k      = 35000.0f;  // Spring stiffness (N/m)
    float   damping_c     = 4500.0f;   // Damping coefficient (N·s/m)
    float   max_travel    = 0.3f; // Max compression/extension
    
    // Runtime state
    float   current_length = 0.5f;
    float   prev_length    = 0.5f;
    float   compression    = 0.0f;
    float   suspension_vel = 0.0f;
    float   force_magnitude = 0.0f;
    bool    grounded       = false;
    Vector3 contact_point  = {};
    Vector3 contact_normal = {0, 1, 0};
    float   ground_friction = 1.0f;
};

// ── Tire Model ─────────────────────────────────────────────────────────────
struct TireState {
    float slip_angle     = 0.0f;  // α = atan2(v_lat, |v_fwd|)
    float lateral_force  = 0.0f;
    float longitudinal_force = 0.0f;
    float normal_force   = 0.0f;  // Fz
    float friction_coeff = 1.2f;  // μ (tire friction)
    float wheel_spin_rpm = 0.0f;
    float wheel_radius   = 0.35f;
};

// ── Vehicle Configuration ──────────────────────────────────────────────────
struct VehicleConfig {
    float mass           = 1400.0f;   // kg
    float inertia_yaw    = 2800.0f;   // kg·m² (yaw moment of inertia)
    float wheelbase      = 2.7f;      // m (front to rear axle)
    float track_width    = 1.6f;      // m (left to right wheel)
    float cg_height      = 0.45f;     // Center of gravity height
    float drag_coeff     = 0.35f;     // Aerodynamic drag
    float frontal_area   = 2.2f;      // m²
    float max_steer_angle = 35.0f;    // degrees
    float steer_speed    = 3.5f;      // degrees per second multiplier
    
    // Wheel offsets (FL, FR, RL, RR)
    std::array<Vector3, 4> wheel_offsets = {{
        {-0.8f, -0.1f,  1.35f},  // Front-Left
        { 0.8f, -0.1f,  1.35f},  // Front-Right
        {-0.8f, -0.1f, -1.35f},  // Rear-Left
        { 0.8f, -0.1f, -1.35f},  // Rear-Right
    }};
};

// ── Vehicle Type (Dr. Driving style) ───────────────────────────────────────
enum class CarModel : uint8_t {
    SEDAN,       // Default family sedan
    HATCHBACK,   // Compact city car
    SUV,         // Larger, higher CG
    SPORTS,      // Low, wide — McLaren silhouette
    SUPER,       // Lamborghini silhouette
    HYPER,       // Bugatti silhouette
    COUNT
};

// ── Raycast Vehicle ────────────────────────────────────────────────────────
class RaycastVehicle {
public:
    void init(CarModel model = CarModel::SEDAN);
    void update(float dt, float gas, float brake, float steer_input,
                bool handbrake, float surface_friction = 1.0f);
    void reset(Vector3 position, float heading_deg = 0.0f);
    
    // State queries
    [[nodiscard]] Vector3    position() const { return position_; }
    [[nodiscard]] Quaternion rotation() const { return rotation_; }
    [[nodiscard]] Vector3    velocity() const { return velocity_; }
    [[nodiscard]] Vector3    forward()  const;
    [[nodiscard]] Vector3    right()    const;
    [[nodiscard]] Vector3    up()       const;
    [[nodiscard]] float      speed()    const;      // m/s
    [[nodiscard]] float      speedKmh() const;      // km/h
    [[nodiscard]] float      steerAngle() const { return current_steer_angle_; }
    
    // Wheel access
    [[nodiscard]] const std::array<SuspensionPoint, 4>& suspension() const { return suspension_; }
    [[nodiscard]] const std::array<TireState, 4>&       tires() const { return tires_; }
    
    // Transmission
    Transmission&       transmission()       { return transmission_; }
    const Transmission& transmission() const { return transmission_; }
    
    // Configuration
    VehicleConfig&       config()       { return config_; }
    const VehicleConfig& config() const { return config_; }
    
    [[nodiscard]] CarModel model() const { return model_; }
    
    // Visual state
    bool left_indicator  = false;
    bool right_indicator = false;
    bool hazards         = false;
    bool horn_active     = false;
    float indicator_timer = 0.0f;
    bool  indicator_on    = false;  // Blink state

    // Bounding box for collisions
    [[nodiscard]] BoundingBox boundingBox() const;
    
    // Network sync: set state from remote
    void setNetState(Vector3 pos, Quaternion rot, Vector3 vel, float steer);

private:
    void applySuspensionForces(float dt);
    void applyTireForces(float dt, float gas, float brake, bool handbrake,
                         float surface_friction);
    void applyDragForce();
    void integrateMotion(float dt);
    void updateIndicators(float dt);
    void applyParkingBrake();
    
    // Raycast against ground plane (or world mesh)
    bool raycastGround(Vector3 origin, Vector3 dir, float max_dist,
                       Vector3& hit_point, Vector3& hit_normal, float& hit_dist) const;
    
    // State
    Vector3    position_  = {0, 1.0f, 0};
    Quaternion rotation_  = QuaternionIdentity();
    Vector3    velocity_  = {0, 0, 0};
    Vector3    angular_velocity_ = {0, 0, 0};
    float      current_steer_angle_ = 0.0f;
    
    // Subsystems
    std::array<SuspensionPoint, 4> suspension_;
    std::array<TireState, 4>       tires_;
    Transmission                   transmission_;
    VehicleConfig                  config_;
    CarModel                       model_ = CarModel::SEDAN;
};

// ── Factory: configure vehicle from CarModel ───────────────────────────────
VehicleConfig makeVehicleConfig(CarModel model);

} // namespace drt
