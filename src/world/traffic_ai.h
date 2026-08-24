#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — NPC Traffic AI
// ═══════════════════════════════════════════════════════════════════════════
#include "core/memory_pool.h"
#include "physics/raycast_vehicle.h"
#include "world/road_generator.h"
#include <array>
#include <cstdint>

namespace drt {

// ── Traffic AI States ──────────────────────────────────────────────────────
enum class TrafficState : uint8_t {
    CRUISING,
    FOLLOWING_LEAD,
    SIGNALING_INTENT,
    LANE_CHANGING,
    EMERGENCY_BRAKING
};

// ── NPC Vehicle Data ───────────────────────────────────────────────────────
struct NPCVehicle {
    RaycastVehicle  vehicle;
    TrafficState    state          = TrafficState::CRUISING;
    int             current_lane   = 0;
    int             target_lane    = 0;
    float           target_speed   = 60.0f;   // km/h
    float           road_t         = 0.0f;    // Position along road spline [0,1]
    bool            oncoming       = false;    // Opposite direction
    
    // Lane change state
    float           signal_timer   = 0.0f;    // Time spent signaling
    float           lane_change_t  = 0.0f;    // Interpolation 0→1 during lane change
    Vector3         lc_start_pos   = {};
    Vector3         lc_end_pos     = {};
    Vector3         lc_start_tan   = {};
    Vector3         lc_end_tan     = {};
    
    // Sensor results
    float           forward_dist   = 999.0f;
    float           left_dist      = 999.0f;
    float           right_dist     = 999.0f;
    float           rear_dist      = 999.0f;
    bool            forward_blocked = false;
    
    // Behavior tuning (randomized per NPC)
    float           follow_distance = 15.0f;
    float           aggression      = 0.5f;   // 0 = passive, 1 = aggressive
    float           speed_variance  = 0.0f;   // ± km/h from target
    
    bool            active = false;
};

// ── Traffic AI System ──────────────────────────────────────────────────────
class TrafficAI {
public:
    void init(RoadGenerator* road);
    void update(float dt, const Vector3& player_pos, float player_speed);
    void render();  // Debug visualization
    void cleanup();
    
    // Configuration
    void setDensity(float pct) { density_ = Clamp(pct, 0.0f, 1.0f); }
    [[nodiscard]] float density() const { return density_; }
    void setMaxNPCs(int n) { max_active_ = n; }
    
    // Spawn/despawn
    void spawnTraffic(const Vector3& around_pos, int count);
    void despawnDistant(const Vector3& player_pos, float max_dist = 200.0f);
    
    // Access NPC data
    [[nodiscard]] NPCVehicle* npcs() { return npcs_.data(); }
    [[nodiscard]] const NPCVehicle* npcs() const { return npcs_.data(); }
    [[nodiscard]] int activeCount() const { return active_count_; }
    
    // Deterministic seeding for multiplayer sync
    void setSeed(uint32_t seed) { rng_seed_ = seed; }

private:
    void updateNPC(NPCVehicle& npc, float dt, const Vector3& player_pos);
    void performRaycastSensors(NPCVehicle& npc);
    void stateCruising(NPCVehicle& npc, float dt);
    void stateFollowingLead(NPCVehicle& npc, float dt);
    void stateSignalingIntent(NPCVehicle& npc, float dt);
    void stateLaneChanging(NPCVehicle& npc, float dt);
    void stateEmergencyBraking(NPCVehicle& npc, float dt);
    
    // Cubic Hermite interpolation for lane changes
    Vector3 hermiteInterpolate(Vector3 p0, Vector3 t0, Vector3 p1, Vector3 t1, float t) const;
    
    // Simple LCG for deterministic NPC behavior
    float randomFloat();
    int   randomInt(int min, int max);
    
    std::array<NPCVehicle, MAX_NPC_CARS> npcs_;
    int            active_count_ = 0;
    int            max_active_   = 32;
    float          density_      = 0.5f;
    RoadGenerator* road_         = nullptr;
    uint32_t       rng_seed_     = 42;
    
    // Spawn control
    float          spawn_timer_  = 0.0f;
    float          spawn_interval_ = 1.0f;

    // Sensor config
    static constexpr float SENSOR_FORWARD_DIST = 50.0f;
    static constexpr float SENSOR_SIDE_DIST    = 8.0f;
    static constexpr float SENSOR_REAR_DIST    = 15.0f;
    static constexpr float SIGNAL_DURATION     = 1.5f;
    static constexpr float LANE_CHANGE_TIME    = 2.0f;
    static constexpr float MIN_GAP_METERS      = 15.0f;
    static constexpr float SLOW_THRESHOLD_KMH  = 10.0f;
};

} // namespace drt
