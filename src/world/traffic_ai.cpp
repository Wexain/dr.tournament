// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — NPC Traffic AI Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "world/traffic_ai.h"
#include <cmath>
#include <algorithm>

namespace drt {

void TrafficAI::init(RoadGenerator* road) {
    road_ = road;
    for (auto& npc : npcs_) {
        npc.active = false;
    }
    active_count_ = 0;
}

void TrafficAI::update(float dt, const Vector3& player_pos, float player_speed) {
    // Spawn new traffic if needed
    spawn_timer_ += dt;
    if (spawn_timer_ >= spawn_interval_ && active_count_ < max_active_ * density_) {
        spawnTraffic(player_pos, 2);
        spawn_timer_ = 0.0f;
    }

    // Update each active NPC
    for (int i = 0; i < (int)npcs_.size(); ++i) {
        if (!npcs_[i].active) continue;
        updateNPC(npcs_[i], dt, player_pos);
    }

    // Despawn distant NPCs
    despawnDistant(player_pos, 250.0f);
}

void TrafficAI::updateNPC(NPCVehicle& npc, float dt, const Vector3& player_pos) {
    // Perform raycast sensors
    performRaycastSensors(npc);

    // State machine
    switch (npc.state) {
        case TrafficState::CRUISING:
            stateCruising(npc, dt);
            break;
        case TrafficState::FOLLOWING_LEAD:
            stateFollowingLead(npc, dt);
            break;
        case TrafficState::SIGNALING_INTENT:
            stateSignalingIntent(npc, dt);
            break;
        case TrafficState::LANE_CHANGING:
            stateLaneChanging(npc, dt);
            break;
        case TrafficState::EMERGENCY_BRAKING:
            stateEmergencyBraking(npc, dt);
            break;
    }

    // Update vehicle physics
    float target_speed_ms = npc.target_speed / 3.6f;
    float current_speed = npc.vehicle.speedKmh();
    float gas = 0.0f, brake = 0.0f;

    if (current_speed < npc.target_speed - 2.0f) {
        gas = std::min(1.0f, (npc.target_speed - current_speed) / 20.0f);
    } else if (current_speed > npc.target_speed + 5.0f) {
        brake = std::min(1.0f, (current_speed - npc.target_speed) / 30.0f);
    }

    // Steering toward lane center
    float steer = 0.0f;
    if (road_) {
        float lane_offset = road_->getLaneCenterOffset(npc.current_lane, npc.oncoming);
        Vector3 road_dir = road_->evaluateTangent(npc.road_t);
        Vector3 road_right = Vector3Normalize(Vector3CrossProduct(road_dir, {0, 1, 0}));
        Vector3 target_pos = Vector3Add(
            road_->evaluatePosition(npc.road_t),
            Vector3Scale(road_right, lane_offset));

        Vector3 to_target = Vector3Subtract(target_pos, npc.vehicle.position());
        float lateral_err = Vector3DotProduct(to_target, npc.vehicle.right());
        steer = std::clamp(lateral_err * 0.5f, -1.0f, 1.0f);
    }

    npc.vehicle.update(dt, gas, brake, steer, false, 1.0f);

    // Advance road_t
    float speed_ms = npc.vehicle.speed();
    float road_len = road_ ? road_->getTotalLength() : 3200.0f;
    npc.road_t += speed_ms * dt / road_len;
    if (npc.road_t > 1.0f) npc.road_t -= 1.0f;

    // Update indicators
    npc.vehicle.left_indicator = (npc.state == TrafficState::SIGNALING_INTENT ||
                                   npc.state == TrafficState::LANE_CHANGING) &&
                                  npc.target_lane < npc.current_lane;
    npc.vehicle.right_indicator = (npc.state == TrafficState::SIGNALING_INTENT ||
                                    npc.state == TrafficState::LANE_CHANGING) &&
                                   npc.target_lane > npc.current_lane;
}

void TrafficAI::performRaycastSensors(NPCVehicle& npc) {
    Vector3 fwd = npc.vehicle.forward();
    Vector3 rgt = npc.vehicle.right();
    Vector3 pos = npc.vehicle.position();

    npc.forward_dist = SENSOR_FORWARD_DIST;
    npc.left_dist = SENSOR_SIDE_DIST;
    npc.right_dist = SENSOR_SIDE_DIST;
    npc.rear_dist = SENSOR_REAR_DIST;
    npc.forward_blocked = false;

    // Check distance to all other active NPCs
    for (int i = 0; i < (int)npcs_.size(); ++i) {
        if (!npcs_[i].active || &npcs_[i] == &npc) continue;

        Vector3 to_other = Vector3Subtract(npcs_[i].vehicle.position(), pos);
        float dist = Vector3Length(to_other);
        if (dist > SENSOR_FORWARD_DIST * 1.5f) continue;

        // Forward sensor
        float fwd_dot = Vector3DotProduct(Vector3Normalize(to_other), fwd);
        float side_dot = Vector3DotProduct(Vector3Normalize(to_other), rgt);

        if (fwd_dot > 0.7f && dist < npc.forward_dist) {
            npc.forward_dist = dist;
            npc.forward_blocked = true;
        }

        // Side sensors
        if (fabsf(fwd_dot) < 0.5f) {
            if (side_dot < -0.3f && dist < npc.left_dist) npc.left_dist = dist;
            if (side_dot > 0.3f && dist < npc.right_dist) npc.right_dist = dist;
        }

        // Rear sensor
        if (fwd_dot < -0.7f && dist < npc.rear_dist) {
            npc.rear_dist = dist;
        }
    }
}

void TrafficAI::stateCruising(NPCVehicle& npc, float dt) {
    if (npc.forward_blocked && npc.forward_dist < npc.follow_distance) {
        npc.state = TrafficState::FOLLOWING_LEAD;
    }
}

void TrafficAI::stateFollowingLead(NPCVehicle& npc, float dt) {
    // Match lead vehicle speed (slow down)
    npc.target_speed = std::max(20.0f, npc.target_speed - 5.0f * dt);

    // Emergency brake if too close
    if (npc.forward_dist < 5.0f) {
        npc.state = TrafficState::EMERGENCY_BRAKING;
        return;
    }

    // Check if can change lanes
    float speed_diff = npc.vehicle.speedKmh();
    if (speed_diff < npc.target_speed - SLOW_THRESHOLD_KMH) {
        // Want to overtake — check adjacent lanes
        int total_lanes = road_ ? road_->lanesPerSide() : 3;
        bool can_go_left = npc.current_lane > 0 && npc.left_dist > MIN_GAP_METERS;
        bool can_go_right = npc.current_lane < total_lanes - 1 && npc.right_dist > MIN_GAP_METERS;

        if (can_go_right) {
            npc.target_lane = npc.current_lane + 1;
            npc.state = TrafficState::SIGNALING_INTENT;
            npc.signal_timer = 0.0f;
        } else if (can_go_left) {
            npc.target_lane = npc.current_lane - 1;
            npc.state = TrafficState::SIGNALING_INTENT;
            npc.signal_timer = 0.0f;
        }
    }

    // If lead cleared, resume cruising
    if (!npc.forward_blocked || npc.forward_dist > npc.follow_distance * 1.5f) {
        npc.state = TrafficState::CRUISING;
        npc.target_speed = 60.0f + npc.speed_variance;
    }
}

void TrafficAI::stateSignalingIntent(NPCVehicle& npc, float dt) {
    npc.signal_timer += dt;

    // Mandatory 1.5s signal duration
    if (npc.signal_timer >= SIGNAL_DURATION) {
        // Re-check gap before committing
        bool gap_clear = (npc.target_lane > npc.current_lane)
            ? npc.right_dist > MIN_GAP_METERS
            : npc.left_dist > MIN_GAP_METERS;

        if (gap_clear) {
            npc.state = TrafficState::LANE_CHANGING;
            npc.lane_change_t = 0.0f;

            // Set up Hermite interpolation endpoints
            npc.lc_start_pos = npc.vehicle.position();
            npc.lc_start_tan = Vector3Scale(npc.vehicle.forward(), 10.0f);

            float target_offset = road_ ?
                road_->getLaneCenterOffset(npc.target_lane, npc.oncoming) : 0.0f;
            Vector3 road_right = Vector3Normalize(
                Vector3CrossProduct(npc.vehicle.forward(), {0, 1, 0}));
            npc.lc_end_pos = Vector3Add(
                Vector3Add(npc.vehicle.position(), Vector3Scale(npc.vehicle.forward(), 30.0f)),
                Vector3Scale(road_right, (npc.target_lane - npc.current_lane) * (road_ ? road_->laneWidth() : 3.5f)));
            npc.lc_end_tan = Vector3Scale(npc.vehicle.forward(), 10.0f);
        } else {
            // Gap not clear — abort
            npc.state = TrafficState::FOLLOWING_LEAD;
        }
    }
}

void TrafficAI::stateLaneChanging(NPCVehicle& npc, float dt) {
    npc.lane_change_t += dt / LANE_CHANGE_TIME;

    if (npc.lane_change_t >= 1.0f) {
        // Lane change complete
        npc.current_lane = npc.target_lane;
        npc.state = TrafficState::CRUISING;
        npc.target_speed = 60.0f + npc.speed_variance;
        npc.vehicle.left_indicator = false;
        npc.vehicle.right_indicator = false;
        return;
    }

    // Cubic Hermite interpolation for smooth lane change path
    // The vehicle steering handles this via lane-center seeking in updateNPC
}

void TrafficAI::stateEmergencyBraking(NPCVehicle& npc, float dt) {
    npc.target_speed = std::max(0.0f, npc.target_speed - 40.0f * dt);

    if (npc.forward_dist > 10.0f || !npc.forward_blocked) {
        npc.state = TrafficState::FOLLOWING_LEAD;
        npc.target_speed = 40.0f;
    }
}

void TrafficAI::spawnTraffic(const Vector3& around_pos, int count) {
    for (int c = 0; c < count; ++c) {
        if (active_count_ >= max_active_) break;

        // Find free slot
        int slot = -1;
        for (int i = 0; i < (int)npcs_.size(); ++i) {
            if (!npcs_[i].active) { slot = i; break; }
        }
        if (slot < 0) break;

        auto& npc = npcs_[slot];
        npc.active = true;
        active_count_++;

        // Randomize properties
        int model_idx = randomInt(0, static_cast<int>(CarModel::COUNT) - 1);
        npc.vehicle.init(static_cast<CarModel>(model_idx));

        int total_lanes = road_ ? road_->lanesPerSide() : 3;
        npc.current_lane = randomInt(0, total_lanes - 1);
        npc.target_lane = npc.current_lane;
        npc.oncoming = randomFloat() > 0.5f;
        npc.target_speed = 50.0f + randomFloat() * 30.0f;
        npc.speed_variance = (randomFloat() - 0.5f) * 20.0f;
        npc.follow_distance = 10.0f + randomFloat() * 15.0f;
        npc.aggression = randomFloat();
        npc.state = TrafficState::CRUISING;

        // Position ahead/behind player
        float spawn_offset = 80.0f + randomFloat() * 120.0f;
        if (randomFloat() > 0.5f) spawn_offset = -spawn_offset;

        float lane_offset = road_ ?
            road_->getLaneCenterOffset(npc.current_lane, npc.oncoming) : 0.0f;

        Vector3 spawn_pos = {
            around_pos.x + lane_offset,
            0.5f,
            around_pos.z + spawn_offset
        };
        npc.vehicle.reset(spawn_pos, npc.oncoming ? 180.0f : 0.0f);
        npc.vehicle.transmission().shiftTo(GearPosition::DRIVE);

        // Set road_t based on Z position
        float road_len = road_ ? road_->getTotalLength() : 3200.0f;
        npc.road_t = std::clamp(spawn_pos.z / road_len, 0.0f, 1.0f);
    }
}

void TrafficAI::despawnDistant(const Vector3& player_pos, float max_dist) {
    for (auto& npc : npcs_) {
        if (!npc.active) continue;
        float dist = Vector3Distance(npc.vehicle.position(), player_pos);
        if (dist > max_dist) {
            npc.active = false;
            active_count_--;
        }
    }
}

void TrafficAI::render() {
    // Debug visualization (optional)
    #ifndef NDEBUG
    for (const auto& npc : npcs_) {
        if (!npc.active) continue;
        Color state_color;
        switch (npc.state) {
            case TrafficState::CRUISING:          state_color = GREEN; break;
            case TrafficState::FOLLOWING_LEAD:     state_color = YELLOW; break;
            case TrafficState::SIGNALING_INTENT:   state_color = ORANGE; break;
            case TrafficState::LANE_CHANGING:      state_color = BLUE; break;
            case TrafficState::EMERGENCY_BRAKING:  state_color = RED; break;
        }
        DrawSphere(Vector3Add(npc.vehicle.position(), {0, 2, 0}), 0.2f, state_color);
    }
    #endif
}

void TrafficAI::cleanup() {
    for (auto& npc : npcs_) npc.active = false;
    active_count_ = 0;
}

Vector3 TrafficAI::hermiteInterpolate(Vector3 p0, Vector3 t0, Vector3 p1, Vector3 t1, float t) const {
    float t2 = t * t;
    float t3 = t2 * t;
    float h00 = 2*t3 - 3*t2 + 1;
    float h10 = t3 - 2*t2 + t;
    float h01 = -2*t3 + 3*t2;
    float h11 = t3 - t2;
    return {
        h00*p0.x + h10*t0.x + h01*p1.x + h11*t1.x,
        h00*p0.y + h10*t0.y + h01*p1.y + h11*t1.y,
        h00*p0.z + h10*t0.z + h01*p1.z + h11*t1.z
    };
}

// ── Deterministic RNG ──────────────────────────────────────────────────────
float TrafficAI::randomFloat() {
    rng_seed_ = rng_seed_ * 1103515245 + 12345;
    return ((rng_seed_ >> 16) & 0x7FFF) / 32767.0f;
}

int TrafficAI::randomInt(int min, int max) {
    return min + static_cast<int>(randomFloat() * (max - min + 1));
}

} // namespace drt
