// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Vehicle Renderer Implementation (Dr. Driving Style)
// ═══════════════════════════════════════════════════════════════════════════
#include "render/vehicle_renderer.h"
#include <raymath.h>
#include <cmath>

namespace drt {

void VehicleRenderer::init() {
    // Pre-build all car meshes at all LOD levels
    for (int m = 0; m < static_cast<int>(CarModel::COUNT); ++m) {
        for (int l = 0; l < 3; ++l) {
            buildCarMesh(static_cast<CarModel>(m), static_cast<LODLevel>(l),
                         car_meshes_[m][l]);
        }
    }
}

void VehicleRenderer::cleanup() {
    for (auto& model_set : car_meshes_) {
        for (auto& lod : model_set) {
            if (lod.built) {
                UnloadModel(lod.body);
                UnloadModel(lod.wheel);
                lod.built = false;
            }
        }
    }
}

void VehicleRenderer::renderVehicle(const RaycastVehicle& vehicle, float alpha) {
    int model_idx = static_cast<int>(vehicle.model());
    LODLevel lod = LODLevel::HIGH;  // Default for player
    auto& mesh = car_meshes_[model_idx][static_cast<int>(lod)];
    if (!mesh.built) return;

    // Build transform
    Matrix rot_mat = QuaternionToMatrix(vehicle.rotation());
    Matrix trans = MatrixTranslate(vehicle.position().x,
                                    vehicle.position().y,
                                    vehicle.position().z);
    Matrix transform = MatrixMultiply(rot_mat, trans);

    // Draw body
    Color body_col = (alpha < 1.0f)
        ? Color{mesh.body_color.r, mesh.body_color.g, mesh.body_color.b,
                (unsigned char)(alpha * 255)}
        : mesh.body_color;

    mesh.body.transform = transform;
    DrawModel(mesh.body, {0, 0, 0}, 1.0f, body_col);

    // Draw wheels
    renderWheels(vehicle, mesh);

    // Draw indicators
    renderIndicators(vehicle, mesh);

    // Draw brake lights
    renderBrakeLights(vehicle, mesh);
}

void VehicleRenderer::renderGhost(Vector3 pos, Quaternion rot, float steer_angle,
                                   uint8_t gear, bool left_ind, bool right_ind,
                                   bool hazards, CarModel model, float alpha) {
    int model_idx = static_cast<int>(model);
    auto& mesh = car_meshes_[model_idx][static_cast<int>(LODLevel::MEDIUM)];
    if (!mesh.built) return;

    Matrix rot_mat = QuaternionToMatrix(rot);
    Matrix trans = MatrixTranslate(pos.x, pos.y, pos.z);
    mesh.body.transform = MatrixMultiply(rot_mat, trans);

    Color ghost_col = {100, 180, 255, (unsigned char)(alpha * 180)};
    DrawModel(mesh.body, {0, 0, 0}, 1.0f, ghost_col);
}

void VehicleRenderer::renderWheels(const RaycastVehicle& vehicle, const CarMeshSet& mesh) {
    const auto& susp = vehicle.suspension();
    const auto& tires = vehicle.tires();

    for (int i = 0; i < 4; ++i) {
        Vector3 wheel_world = Vector3Add(vehicle.position(),
            Vector3Transform(susp[i].local_offset, QuaternionToMatrix(vehicle.rotation())));

        // Lower wheel to ground contact
        if (susp[i].grounded) {
            wheel_world.y = susp[i].contact_point.y + tires[i].wheel_radius;
        }

        Matrix wheel_transform = MatrixIdentity();

        // Wheel rotation (spin)
        float spin_angle = tires[i].wheel_spin_rpm * GetFrameTime() * 6.0f * DEG2RAD;
        wheel_transform = MatrixMultiply(wheel_transform, MatrixRotateX(spin_angle));

        // Front wheels: steering rotation
        if (i < 2) {
            wheel_transform = MatrixMultiply(wheel_transform,
                MatrixRotateY(vehicle.steerAngle() * DEG2RAD));
        }

        // Mirror right-side wheels
        if (i == 1 || i == 3) {
            wheel_transform = MatrixMultiply(wheel_transform, MatrixRotateY(PI));
        }

        wheel_transform = MatrixMultiply(wheel_transform,
            MatrixTranslate(wheel_world.x, wheel_world.y, wheel_world.z));

        DrawCylinder(wheel_world, tires[i].wheel_radius, tires[i].wheel_radius,
                     0.2f, 12, {30, 30, 35, 255});
        // Hubcap
        DrawCylinder(wheel_world, tires[i].wheel_radius * 0.6f,
                     tires[i].wheel_radius * 0.6f, 0.22f, 8, {80, 80, 90, 255});
    }
}

void VehicleRenderer::renderIndicators(const RaycastVehicle& vehicle, const CarMeshSet& mesh) {
    if (!vehicle.indicator_on) return;

    Vector3 pos = vehicle.position();
    Vector3 fwd = vehicle.forward();
    Vector3 rgt = vehicle.right();
    float hw = vehicle.config().track_width * 0.5f;
    float hl = vehicle.config().wheelbase * 0.5f;

    Color amber = {255, 160, 0, 255};

    if (vehicle.left_indicator || vehicle.hazards) {
        Vector3 fl = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, hl),
                     Vector3Scale(rgt, -hw)));
        Vector3 rl = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, -hl),
                     Vector3Scale(rgt, -hw)));
        fl.y += 0.5f; rl.y += 0.5f;
        DrawSphere(fl, 0.08f, amber);
        DrawSphere(rl, 0.08f, amber);
    }

    if (vehicle.right_indicator || vehicle.hazards) {
        Vector3 fr = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, hl),
                     Vector3Scale(rgt, hw)));
        Vector3 rr = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, -hl),
                     Vector3Scale(rgt, hw)));
        fr.y += 0.5f; rr.y += 0.5f;
        DrawSphere(fr, 0.08f, amber);
        DrawSphere(rr, 0.08f, amber);
    }
}

void VehicleRenderer::renderBrakeLights(const RaycastVehicle& vehicle, const CarMeshSet& mesh) {
    if (!vehicle.transmission().isReverse()) return;

    Vector3 pos = vehicle.position();
    Vector3 fwd = vehicle.forward();
    Vector3 rgt = vehicle.right();
    float hw = vehicle.config().track_width * 0.45f;
    float hl = vehicle.config().wheelbase * 0.5f;

    Color white_light = {255, 255, 255, 255};
    Vector3 rl = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, -hl), Vector3Scale(rgt, -hw)));
    Vector3 rr = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, -hl), Vector3Scale(rgt, hw)));
    rl.y += 0.45f; rr.y += 0.45f;
    DrawSphere(rl, 0.06f, white_light);
    DrawSphere(rr, 0.06f, white_light);
}

LODLevel VehicleRenderer::selectLOD(float distance) const {
    if (distance < lod_med_dist_) return LODLevel::HIGH;
    if (distance < lod_low_dist_) return LODLevel::MEDIUM;
    return LODLevel::LOW;
}

void VehicleRenderer::setPlayerColor(Color body, Color accent) {
    player_body_color_ = body;
    player_accent_color_ = accent;
}

// ── Procedural Mesh Builders (Dr. Driving Style) ───────────────────────────
// All cars are built from simple box/trapezoid shapes — the Dr. Driving 
// aesthetic: clean, recognizable silhouettes without excessive detail.

void VehicleRenderer::buildCarMesh(CarModel model, LODLevel lod, CarMeshSet& out) {
    int detail = (lod == LODLevel::HIGH) ? 2 : (lod == LODLevel::MEDIUM) ? 1 : 0;

    switch (model) {
        case CarModel::SEDAN:     buildSedanMesh(out, detail); break;
        case CarModel::HATCHBACK: buildHatchbackMesh(out, detail); break;
        case CarModel::SUV:       buildSUVMesh(out, detail); break;
        case CarModel::SPORTS:    buildSportsMesh(out, detail); break;
        case CarModel::SUPER:     buildSuperMesh(out, detail); break;
        case CarModel::HYPER:     buildHyperMesh(out, detail); break;
        default:                  buildSedanMesh(out, detail); break;
    }

    buildWheelMesh(out.wheel, detail);
    out.built = true;
}

void VehicleRenderer::buildSedanMesh(CarMeshSet& out, int detail) {
    // Dr. Driving sedan: classic proportioned 4-door sedan
    float length = 4.5f, width = 1.8f, height = 1.4f;
    float hood_len = 1.2f, cabin_len = 1.8f, trunk_len = 1.0f;

    // Main body: slightly tapered box
    Mesh body = GenMeshCube(width, height * 0.55f, length);
    out.body = LoadModelFromMesh(body);
    out.body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {45, 85, 255, 255};
    out.body_color = {45, 85, 255, 255};
    out.accent_color = {30, 30, 35, 255};
}

void VehicleRenderer::buildHatchbackMesh(CarMeshSet& out, int detail) {
    float length = 3.8f, width = 1.7f;
    Mesh body = GenMeshCube(width, 0.7f, length);
    out.body = LoadModelFromMesh(body);
    out.body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {200, 50, 50, 255};
    out.body_color = {200, 50, 50, 255};
}

void VehicleRenderer::buildSUVMesh(CarMeshSet& out, int detail) {
    float length = 4.8f, width = 2.0f;
    Mesh body = GenMeshCube(width, 1.0f, length);
    out.body = LoadModelFromMesh(body);
    out.body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {40, 40, 45, 255};
    out.body_color = {40, 40, 45, 255};
}

void VehicleRenderer::buildSportsMesh(CarMeshSet& out, int detail) {
    // McLaren silhouette: low, wide, aggressive
    float length = 4.5f, width = 2.0f;
    Mesh body = GenMeshCube(width, 0.5f, length);
    out.body = LoadModelFromMesh(body);
    out.body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {255, 120, 0, 255};
    out.body_color = {255, 120, 0, 255};
}

void VehicleRenderer::buildSuperMesh(CarMeshSet& out, int detail) {
    // Lamborghini silhouette: angular, low, sharp edges
    float length = 4.6f, width = 2.05f;
    Mesh body = GenMeshCube(width, 0.48f, length);
    out.body = LoadModelFromMesh(body);
    out.body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {220, 200, 0, 255};
    out.body_color = {220, 200, 0, 255};
}

void VehicleRenderer::buildHyperMesh(CarMeshSet& out, int detail) {
    // Bugatti silhouette: rounded, wide, luxurious proportions
    float length = 4.7f, width = 2.1f;
    Mesh body = GenMeshCube(width, 0.52f, length);
    out.body = LoadModelFromMesh(body);
    out.body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {20, 20, 60, 255};
    out.body_color = {20, 20, 60, 255};
}

void VehicleRenderer::buildWheelMesh(Model& out, int detail) {
    int slices = (detail >= 2) ? 16 : (detail == 1) ? 12 : 8;
    Mesh wheel = GenMeshCylinder(0.35f, 0.2f, slices);
    out = LoadModelFromMesh(wheel);
    out.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {30, 30, 35, 255};
}

} // namespace drt
