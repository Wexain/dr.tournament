// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Raycast Vehicle Physics Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "physics/raycast_vehicle.h"
#include <algorithm>
#include <cmath>

namespace drt {

// ── Vehicle Config Factory ─────────────────────────────────────────────────
VehicleConfig makeVehicleConfig(CarModel model) {
    VehicleConfig cfg;
    switch (model) {
        case CarModel::SEDAN:
            cfg.mass = 1400.0f; cfg.wheelbase = 2.7f; cfg.track_width = 1.6f;
            cfg.cg_height = 0.50f; cfg.drag_coeff = 0.32f; cfg.frontal_area = 2.2f;
            cfg.max_steer_angle = 35.0f;
            break;
        case CarModel::HATCHBACK:
            cfg.mass = 1100.0f; cfg.wheelbase = 2.4f; cfg.track_width = 1.5f;
            cfg.cg_height = 0.48f; cfg.drag_coeff = 0.30f; cfg.frontal_area = 2.0f;
            cfg.max_steer_angle = 38.0f;
            break;
        case CarModel::SUV:
            cfg.mass = 2000.0f; cfg.wheelbase = 2.9f; cfg.track_width = 1.7f;
            cfg.cg_height = 0.65f; cfg.drag_coeff = 0.40f; cfg.frontal_area = 2.8f;
            cfg.max_steer_angle = 32.0f;
            break;
        case CarModel::SPORTS:  // McLaren style
            cfg.mass = 1450.0f; cfg.wheelbase = 2.67f; cfg.track_width = 1.7f;
            cfg.cg_height = 0.38f; cfg.drag_coeff = 0.33f; cfg.frontal_area = 1.9f;
            cfg.max_steer_angle = 30.0f;
            break;
        case CarModel::SUPER:   // Lambo style
            cfg.mass = 1550.0f; cfg.wheelbase = 2.7f; cfg.track_width = 1.72f;
            cfg.cg_height = 0.40f; cfg.drag_coeff = 0.35f; cfg.frontal_area = 2.0f;
            cfg.max_steer_angle = 28.0f;
            break;
        case CarModel::HYPER:   // Bugatti style
            cfg.mass = 1995.0f; cfg.wheelbase = 2.71f; cfg.track_width = 1.73f;
            cfg.cg_height = 0.42f; cfg.drag_coeff = 0.36f; cfg.frontal_area = 2.07f;
            cfg.max_steer_angle = 26.0f;
            break;
        default: break;
    }
    // Set wheel offsets based on wheelbase/track
    float hw = cfg.track_width * 0.5f;
    float hb = cfg.wheelbase * 0.5f;
    cfg.wheel_offsets = {{
        {-hw, -0.1f,  hb},  // FL
        { hw, -0.1f,  hb},  // FR
        {-hw, -0.1f, -hb},  // RL
        { hw, -0.1f, -hb},  // RR
    }};
    cfg.inertia_yaw = cfg.mass * (cfg.wheelbase * cfg.wheelbase +
                                   cfg.track_width * cfg.track_width) / 12.0f;
    return cfg;
}

void RaycastVehicle::init(CarModel model) {
    model_ = model;
    config_ = makeVehicleConfig(model);
    transmission_.init();

    // Initialize suspension for each wheel
    for (int i = 0; i < 4; ++i) {
        suspension_[i].local_offset = config_.wheel_offsets[i];
        suspension_[i].rest_length = 0.5f;
        suspension_[i].spring_k = 35000.0f;
        suspension_[i].damping_c = 4500.0f;
        suspension_[i].current_length = suspension_[i].rest_length;
        suspension_[i].prev_length = suspension_[i].rest_length;

        tires_[i].friction_coeff = 1.2f;
        tires_[i].wheel_radius = 0.35f;
    }

    // Slightly stiffer rear springs for stability
    suspension_[2].spring_k = 38000.0f;
    suspension_[3].spring_k = 38000.0f;
    suspension_[2].damping_c = 5000.0f;
    suspension_[3].damping_c = 5000.0f;

    position_ = {0, 1.0f, 0};
    rotation_ = QuaternionIdentity();
    velocity_ = {0, 0, 0};
    angular_velocity_ = {0, 0, 0};
}

void RaycastVehicle::reset(Vector3 position, float heading_deg) {
    position_ = position;
    rotation_ = QuaternionFromEuler(0, heading_deg * DEG2RAD, 0);
    velocity_ = {0, 0, 0};
    angular_velocity_ = {0, 0, 0};
    current_steer_angle_ = 0.0f;
    for (auto& s : suspension_) {
        s.current_length = s.rest_length;
        s.prev_length = s.rest_length;
        s.grounded = false;
    }
}

void RaycastVehicle::update(float dt, float gas, float brake, float steer_input,
                             bool handbrake, float surface_friction) {
    // Steering interpolation
    float target_steer = steer_input * config_.max_steer_angle;
    float steer_rate = config_.steer_speed * config_.max_steer_angle * dt;
    if (current_steer_angle_ < target_steer)
        current_steer_angle_ = std::min(current_steer_angle_ + steer_rate, target_steer);
    else
        current_steer_angle_ = std::max(current_steer_angle_ - steer_rate, target_steer);

    // Update transmission
    float avg_wheel_rpm = 0.0f;
    for (auto& t : tires_) avg_wheel_rpm += t.wheel_spin_rpm;
    avg_wheel_rpm /= 4.0f;
    transmission_.update(dt, avg_wheel_rpm, gas);

    // Physics pipeline
    applySuspensionForces(dt);
    applyTireForces(dt, gas, brake, handbrake, surface_friction);
    applyDragForce();

    // Park: angular wheel lock
    if (transmission_.isParked()) {
        applyParkingBrake();
    }

    integrateMotion(dt);
    updateIndicators(dt);
}

void RaycastVehicle::applySuspensionForces(float dt) {
    Matrix rot_matrix = QuaternionToMatrix(rotation_);

    for (int i = 0; i < 4; ++i) {
        auto& sp = suspension_[i];

        // Transform wheel attachment to world space
        Vector3 attach_world = Vector3Add(position_,
            Vector3Transform(sp.local_offset, rot_matrix));

        // Ray direction = local down in world space
        Vector3 ray_dir = Vector3Transform({0, -1, 0}, rot_matrix);
        ray_dir = Vector3Normalize(ray_dir);

        float max_ray = sp.rest_length + sp.max_travel;
        Vector3 hit_point, hit_normal;
        float hit_dist;

        if (raycastGround(attach_world, ray_dir, max_ray, hit_point, hit_normal, hit_dist)) {
            sp.grounded = true;
            sp.contact_point = hit_point;
            sp.contact_normal = hit_normal;
            sp.prev_length = sp.current_length;
            sp.current_length = std::clamp(hit_dist, sp.rest_length - sp.max_travel,
                                            sp.rest_length + sp.max_travel);

            // Spring force: F_s = k * (L0 - L) - c * v_suspension
            sp.compression = sp.rest_length - sp.current_length;
            sp.suspension_vel = (sp.current_length - sp.prev_length) / dt;
            sp.force_magnitude = sp.spring_k * sp.compression - sp.damping_c * sp.suspension_vel;
            sp.force_magnitude = std::max(sp.force_magnitude, 0.0f); // No pulling

            // Apply force at contact point
            Vector3 force = Vector3Scale(sp.contact_normal, sp.force_magnitude);

            // Apply to linear velocity
            Vector3 accel = Vector3Scale(force, 1.0f / config_.mass);
            velocity_ = Vector3Add(velocity_, Vector3Scale(accel, dt));

            // Apply torque (force creates angular acceleration)
            Vector3 r = Vector3Subtract(attach_world, position_);
            Vector3 torque = Vector3CrossProduct(r, force);
            // Simplified: only yaw and pitch torque
            angular_velocity_.x += torque.x / config_.inertia_yaw * dt;
            angular_velocity_.z += torque.z / config_.inertia_yaw * dt;

            // Tire normal force for friction
            tires_[i].normal_force = sp.force_magnitude;
        } else {
            sp.grounded = false;
            sp.current_length = sp.rest_length + sp.max_travel;
            sp.force_magnitude = 0.0f;
            tires_[i].normal_force = 0.0f;
        }
    }
}

void RaycastVehicle::applyTireForces(float dt, float gas, float brake,
                                      bool handbrake, float surface_friction) {
    Matrix rot_matrix = QuaternionToMatrix(rotation_);
    Vector3 fwd = forward();
    Vector3 rgt = right();

    float drive_torque = transmission_.outputTorque();

    for (int i = 0; i < 4; ++i) {
        auto& tire = tires_[i];
        auto& sp = suspension_[i];
        if (!sp.grounded) continue;

        // Tire world-space velocity at contact
        Vector3 r = Vector3Subtract(
            Vector3Add(position_, Vector3Transform(sp.local_offset, rot_matrix)),
            position_);
        Vector3 point_vel = Vector3Add(velocity_,
            Vector3CrossProduct(angular_velocity_, r));

        // Decompose into forward/lateral components
        // For front wheels, rotate by steering angle
        Vector3 tire_fwd = fwd;
        Vector3 tire_rgt = rgt;
        if (i < 2) {  // Front wheels steer
            float steer_rad = current_steer_angle_ * DEG2RAD;
            tire_fwd = Vector3Add(
                Vector3Scale(fwd, cosf(steer_rad)),
                Vector3Scale(rgt, sinf(steer_rad)));
            tire_fwd = Vector3Normalize(tire_fwd);
            tire_rgt = Vector3CrossProduct({0, 1, 0}, tire_fwd);
            tire_rgt = Vector3Normalize(tire_rgt);
        }

        float v_forward = Vector3DotProduct(point_vel, tire_fwd);
        float v_lateral = Vector3DotProduct(point_vel, tire_rgt);

        // Slip angle: α = atan2(v_lateral, |v_forward|)
        float v_fwd_abs = fabsf(v_forward) + 0.001f; // Prevent div/0
        tire.slip_angle = atan2f(v_lateral, v_fwd_abs);

        // Tire friction circle: max force = μ * Fz
        float mu = tire.friction_coeff * surface_friction * sp.ground_friction;
        float max_tire_force = mu * tire.normal_force;

        // Lateral force (cornering) — simplified Pacejka-like linear region
        float lateral_stiffness = 8.0f;  // Cornering stiffness
        tire.lateral_force = -lateral_stiffness * tire.slip_angle * tire.normal_force;

        // Clamp to friction circle
        tire.lateral_force = std::clamp(tire.lateral_force, -max_tire_force, max_tire_force);

        // Longitudinal force (drive/brake)
        tire.longitudinal_force = 0.0f;

        // Drive torque (rear-wheel drive: wheels 2, 3)
        if (i >= 2) {
            float wheel_force = drive_torque / tire.wheel_radius;
            // Apply gas modulation
            wheel_force *= gas;
            tire.longitudinal_force += wheel_force;
        }

        // Braking (all wheels)
        float brake_force = brake * 8000.0f * (tire.normal_force / (config_.mass * 9.81f / 4.0f));
        if (v_forward > 0.1f) tire.longitudinal_force -= brake_force;
        else if (v_forward < -0.1f) tire.longitudinal_force += brake_force;

        // Handbrake (rear wheels only)
        if (handbrake && i >= 2) {
            float hb_force = 12000.0f * (tire.normal_force / (config_.mass * 9.81f / 4.0f));
            if (v_forward > 0.1f) tire.longitudinal_force -= hb_force;
            else if (v_forward < -0.1f) tire.longitudinal_force += hb_force;
        }

        // Clamp total force to friction circle
        float total_force = sqrtf(tire.lateral_force * tire.lateral_force +
                                   tire.longitudinal_force * tire.longitudinal_force);
        if (total_force > max_tire_force && total_force > 0.001f) {
            float scale = max_tire_force / total_force;
            tire.lateral_force *= scale;
            tire.longitudinal_force *= scale;
        }

        // Apply forces
        Vector3 force_world = Vector3Add(
            Vector3Scale(tire_fwd, tire.longitudinal_force),
            Vector3Scale(tire_rgt, tire.lateral_force));

        Vector3 accel = Vector3Scale(force_world, dt / config_.mass);
        velocity_ = Vector3Add(velocity_, accel);

        // Yaw torque from lateral force
        float yaw_torque = tire.lateral_force * r.z + tire.longitudinal_force * r.x;
        angular_velocity_.y += yaw_torque / config_.inertia_yaw * dt;

        // Update wheel spin RPM
        tire.wheel_spin_rpm = fabsf(v_forward) / (2.0f * PI * tire.wheel_radius) * 60.0f;
    }
}

void RaycastVehicle::applyDragForce() {
    float spd = Vector3Length(velocity_);
    if (spd < 0.01f) return;

    // Aerodynamic drag: F = 0.5 * Cd * A * rho * v²
    float rho = 1.225f;  // Air density kg/m³
    float drag_mag = 0.5f * config_.drag_coeff * config_.frontal_area * rho * spd * spd;
    Vector3 drag_dir = Vector3Negate(Vector3Normalize(velocity_));
    Vector3 drag_force = Vector3Scale(drag_dir, drag_mag);

    velocity_ = Vector3Add(velocity_, Vector3Scale(drag_force, 1.0f / config_.mass * (1.0f / 60.0f)));

    // Rolling resistance (simplified)
    float roll_resist = 200.0f; // N
    Vector3 roll_force = Vector3Scale(drag_dir, roll_resist);
    velocity_ = Vector3Add(velocity_, Vector3Scale(roll_force, 1.0f / config_.mass * (1.0f / 60.0f)));
}

void RaycastVehicle::applyParkingBrake() {
    // Exponential decay to zero
    velocity_ = Vector3Scale(velocity_, 0.9f);
    angular_velocity_ = Vector3Scale(angular_velocity_, 0.9f);
    if (Vector3Length(velocity_) < 0.05f) velocity_ = {0, 0, 0};
}

void RaycastVehicle::integrateMotion(float dt) {
    // Gravity
    velocity_.y -= 9.81f * dt;

    // Integrate position
    position_ = Vector3Add(position_, Vector3Scale(velocity_, dt));

    // Ground clamp (prevent falling through)
    if (position_.y < 0.4f) {
        position_.y = 0.4f;
        if (velocity_.y < 0) velocity_.y = 0;
    }

    // Integrate rotation from angular velocity
    if (Vector3Length(angular_velocity_) > 0.001f) {
        float ang_speed = Vector3Length(angular_velocity_);
        Vector3 axis = Vector3Normalize(angular_velocity_);
        Quaternion dq = QuaternionFromAxisAngle(axis, ang_speed * dt);
        rotation_ = QuaternionMultiply(dq, rotation_);
        rotation_ = QuaternionNormalize(rotation_);
    }

    // Angular damping
    angular_velocity_ = Vector3Scale(angular_velocity_, 0.98f);

    // Lateral velocity damping (prevents infinite sliding)
    Vector3 fwd = forward();
    float v_fwd = Vector3DotProduct(velocity_, fwd);
    float v_lat = Vector3DotProduct(velocity_, right());
    float v_up = velocity_.y;
    // Dampen lateral more aggressively
    v_lat *= 0.96f;
    velocity_ = Vector3Add(
        Vector3Add(Vector3Scale(fwd, v_fwd), Vector3Scale(right(), v_lat)),
        {0, v_up, 0});
}

void RaycastVehicle::updateIndicators(float dt) {
    indicator_timer += dt;
    if (indicator_timer >= 0.5f) {
        indicator_timer -= 0.5f;
        indicator_on = !indicator_on;
    }
}

Vector3 RaycastVehicle::forward() const {
    Matrix m = QuaternionToMatrix(rotation_);
    return Vector3Normalize({m.m8, m.m9, m.m10});
}

Vector3 RaycastVehicle::right() const {
    Matrix m = QuaternionToMatrix(rotation_);
    return Vector3Normalize({m.m0, m.m1, m.m2});
}

Vector3 RaycastVehicle::up() const {
    Matrix m = QuaternionToMatrix(rotation_);
    return Vector3Normalize({m.m4, m.m5, m.m6});
}

float RaycastVehicle::speed() const {
    return Vector3Length(velocity_);
}

float RaycastVehicle::speedKmh() const {
    return speed() * 3.6f;
}

BoundingBox RaycastVehicle::boundingBox() const {
    float hw = config_.track_width * 0.6f;
    float hh = 0.8f;
    float hl = config_.wheelbase * 0.6f;
    return {
        Vector3Subtract(position_, {hw, hh, hl}),
        Vector3Add(position_, {hw, hh, hl})
    };
}

void RaycastVehicle::setNetState(Vector3 pos, Quaternion rot, Vector3 vel, float steer) {
    position_ = pos;
    rotation_ = rot;
    velocity_ = vel;
    current_steer_angle_ = steer;
}

bool RaycastVehicle::raycastGround(Vector3 origin, Vector3 dir, float max_dist,
                                    Vector3& hit_point, Vector3& hit_normal,
                                    float& hit_dist) const {
    // Simple ground plane raycast (Y = 0)
    // In full game, this would raycast against road mesh
    if (dir.y >= 0) return false;  // Ray pointing up

    float t = -origin.y / dir.y;
    if (t < 0 || t > max_dist) return false;

    hit_point = Vector3Add(origin, Vector3Scale(dir, t));
    hit_normal = {0, 1, 0};
    hit_dist = t;
    return true;
}

} // namespace drt
