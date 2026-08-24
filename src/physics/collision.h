#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Collision System
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <raymath.h>
#include <cstdint>
#include <functional>

namespace drt {

class RaycastVehicle;

// ── Collision Event ────────────────────────────────────────────────────────
enum class CollisionType : uint8_t {
    NONE,
    VEHICLE_BARRIER,
    VEHICLE_NPC,
    VEHICLE_VEHICLE,
    VEHICLE_CURB
};

struct CollisionEvent {
    CollisionType type     = CollisionType::NONE;
    Vector3       point    = {};
    Vector3       normal   = {};
    float         impulse  = 0.0f;
    uint32_t      other_id = UINT32_MAX;
};

// ── OBB (Oriented Bounding Box) ───────────────────────────────────────────
struct OBB {
    Vector3 center;
    Vector3 half_extents;   // half-widths along local axes
    Matrix  orientation;    // 3x3 rotation (use full Matrix, ignore translation)
};

// ── Collision System ───────────────────────────────────────────────────────
class CollisionSystem {
public:
    using CollisionCallback = std::function<void(const CollisionEvent&)>;
    
    void init();
    void update(RaycastVehicle& player,
                RaycastVehicle* npcs, int npc_count,
                const BoundingBox* barriers, int barrier_count);
    
    void setCallback(CollisionCallback cb) { callback_ = std::move(cb); }
    
    // Zero-tolerance DNF mode
    void setZeroToleranceDNF(bool enabled) { zero_tolerance_ = enabled; }
    [[nodiscard]] bool hasContact() const { return had_contact_; }
    void resetContact() { had_contact_ = false; }
    
    // Static helpers
    static bool testOBBvsOBB(const OBB& a, const OBB& b, Vector3& mtv);
    static bool testAABBvsAABB(BoundingBox a, BoundingBox b);
    static OBB  vehicleToOBB(const RaycastVehicle& v);

private:
    void resolveCollision(RaycastVehicle& a, RaycastVehicle& b,
                          const Vector3& mtv, CollisionType type);
    void resolveBarrierCollision(RaycastVehicle& v, const BoundingBox& barrier,
                                 const Vector3& mtv);

    CollisionCallback callback_;
    bool              zero_tolerance_ = false;
    bool              had_contact_    = false;
};

} // namespace drt
