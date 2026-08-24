// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Collision System Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "physics/collision.h"
#include "physics/raycast_vehicle.h"
#include <cmath>
#include <algorithm>
#include <array>

namespace drt {

void CollisionSystem::init() {
    zero_tolerance_ = false;
    had_contact_ = false;
}

void CollisionSystem::update(RaycastVehicle& player,
                              RaycastVehicle* npcs, int npc_count,
                              const BoundingBox* barriers, int barrier_count) {
    // Player vs NPC vehicles
    OBB player_obb = vehicleToOBB(player);

    for (int i = 0; i < npc_count; ++i) {
        OBB npc_obb = vehicleToOBB(npcs[i]);
        Vector3 mtv;
        if (testOBBvsOBB(player_obb, npc_obb, mtv)) {
            resolveCollision(player, npcs[i], mtv, CollisionType::VEHICLE_NPC);
            had_contact_ = true;

            if (callback_) {
                CollisionEvent evt;
                evt.type = CollisionType::VEHICLE_NPC;
                evt.point = player.position();
                evt.normal = Vector3Normalize(mtv);
                evt.impulse = Vector3Length(mtv);
                evt.other_id = static_cast<uint32_t>(i);
                callback_(evt);
            }
        }
    }

    // Player vs barriers (AABB test since barriers are axis-aligned)
    BoundingBox player_aabb = player.boundingBox();
    for (int i = 0; i < barrier_count; ++i) {
        if (testAABBvsAABB(player_aabb, barriers[i])) {
            // Simple push-out
            Vector3 center_diff = Vector3Subtract(
                Vector3Add(player.position(), {0, 0, 0}),
                Vector3Scale(Vector3Add(barriers[i].min, barriers[i].max), 0.5f));
            Vector3 mtv = Vector3Normalize(center_diff);
            mtv = Vector3Scale(mtv, 0.2f);  // Push out amount

            resolveBarrierCollision(player, barriers[i], mtv);
            had_contact_ = true;

            if (callback_) {
                CollisionEvent evt;
                evt.type = CollisionType::VEHICLE_BARRIER;
                evt.point = player.position();
                evt.normal = Vector3Normalize(mtv);
                evt.impulse = Vector3Length(player.velocity()) * 0.5f;
                callback_(evt);
            }
        }
    }
}

OBB CollisionSystem::vehicleToOBB(const RaycastVehicle& v) {
    OBB obb;
    obb.center = v.position();
    obb.half_extents = {
        v.config().track_width * 0.55f,
        0.7f,
        v.config().wheelbase * 0.55f
    };
    obb.orientation = QuaternionToMatrix(v.rotation());
    return obb;
}

bool CollisionSystem::testAABBvsAABB(BoundingBox a, BoundingBox b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool CollisionSystem::testOBBvsOBB(const OBB& a, const OBB& b, Vector3& mtv) {
    // SAT (Separating Axis Theorem) for OBB vs OBB
    // Extract axes from orientation matrices
    std::array<Vector3, 3> axes_a = {{
        {a.orientation.m0, a.orientation.m1, a.orientation.m2},
        {a.orientation.m4, a.orientation.m5, a.orientation.m6},
        {a.orientation.m8, a.orientation.m9, a.orientation.m10}
    }};
    std::array<Vector3, 3> axes_b = {{
        {b.orientation.m0, b.orientation.m1, b.orientation.m2},
        {b.orientation.m4, b.orientation.m5, b.orientation.m6},
        {b.orientation.m8, b.orientation.m9, b.orientation.m10}
    }};

    Vector3 d = Vector3Subtract(b.center, a.center);
    float min_overlap = 1e10f;
    Vector3 min_axis = {0, 0, 0};

    auto testAxis = [&](Vector3 axis) -> bool {
        float len = Vector3Length(axis);
        if (len < 0.001f) return true;  // Skip degenerate axes
        axis = Vector3Scale(axis, 1.0f / len);

        float ra = fabsf(Vector3DotProduct(axes_a[0], axis)) * a.half_extents.x +
                    fabsf(Vector3DotProduct(axes_a[1], axis)) * a.half_extents.y +
                    fabsf(Vector3DotProduct(axes_a[2], axis)) * a.half_extents.z;
        float rb = fabsf(Vector3DotProduct(axes_b[0], axis)) * b.half_extents.x +
                    fabsf(Vector3DotProduct(axes_b[1], axis)) * b.half_extents.y +
                    fabsf(Vector3DotProduct(axes_b[2], axis)) * b.half_extents.z;
        float dist = fabsf(Vector3DotProduct(d, axis));

        float overlap = ra + rb - dist;
        if (overlap < 0) return false;  // Separating axis found

        if (overlap < min_overlap) {
            min_overlap = overlap;
            min_axis = axis;
            if (Vector3DotProduct(d, axis) < 0) min_axis = Vector3Negate(min_axis);
        }
        return true;
    };

    // Test 15 axes (3 from A, 3 from B, 9 cross products)
    for (int i = 0; i < 3; ++i) {
        if (!testAxis(axes_a[i])) return false;
        if (!testAxis(axes_b[i])) return false;
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (!testAxis(Vector3CrossProduct(axes_a[i], axes_b[j]))) return false;
        }
    }

    mtv = Vector3Scale(min_axis, min_overlap);
    return true;
}

void CollisionSystem::resolveCollision(RaycastVehicle& a, RaycastVehicle& b,
                                        const Vector3& mtv, CollisionType type) {
    // Separate objects
    Vector3 half_mtv = Vector3Scale(mtv, 0.5f);
    Vector3 pos_a = Vector3Add(a.position(), half_mtv);
    Vector3 pos_b = Vector3Subtract(b.position(), half_mtv);

    // Impulse-based velocity resolution
    Vector3 normal = Vector3Normalize(mtv);
    Vector3 rel_vel = Vector3Subtract(a.velocity(), b.velocity());
    float vel_along_normal = Vector3DotProduct(rel_vel, normal);

    if (vel_along_normal > 0) return;  // Already separating

    float restitution = 0.3f;  // Bounciness
    float inv_mass_a = 1.0f / a.config().mass;
    float inv_mass_b = 1.0f / b.config().mass;

    float j = -(1.0f + restitution) * vel_along_normal / (inv_mass_a + inv_mass_b);
    Vector3 impulse = Vector3Scale(normal, j);

    Vector3 vel_a = Vector3Add(a.velocity(), Vector3Scale(impulse, inv_mass_a));
    Vector3 vel_b = Vector3Subtract(b.velocity(), Vector3Scale(impulse, inv_mass_b));

    a.setNetState(pos_a, a.rotation(), vel_a, a.steerAngle());
    b.setNetState(pos_b, b.rotation(), vel_b, b.steerAngle());
}

void CollisionSystem::resolveBarrierCollision(RaycastVehicle& v,
                                               const BoundingBox& barrier,
                                               const Vector3& mtv) {
    Vector3 pos = Vector3Add(v.position(), mtv);
    Vector3 normal = Vector3Normalize(mtv);

    // Reflect velocity
    Vector3 vel = v.velocity();
    float vn = Vector3DotProduct(vel, normal);
    if (vn < 0) {
        vel = Vector3Subtract(vel, Vector3Scale(normal, 1.5f * vn));
        vel = Vector3Scale(vel, 0.6f);  // Energy loss
    }

    v.setNetState(pos, v.rotation(), vel, v.steerAngle());
}

} // namespace drt
