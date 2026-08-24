#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Vehicle Renderer (Dr. Driving Style)
// ═══════════════════════════════════════════════════════════════════════════
#include "physics/raycast_vehicle.h"
#include <raylib.h>
#include <array>
#include <cstdint>

namespace drt {

// ── Dr. Driving-style car mesh data ────────────────────────────────────────
struct CarMeshSet {
    Model body;
    Model wheel;
    Model headlights;
    Model taillights;
    Model indicator_left;
    Model indicator_right;
    Color body_color       = WHITE;
    Color accent_color     = DARKGRAY;
    bool  built            = false;
};

// ── LOD Level ──────────────────────────────────────────────────────────────
enum class LODLevel : uint8_t {
    HIGH   = 0,  // < 30m
    MEDIUM = 1,  // 30-80m
    LOW    = 2,  // > 80m
    COUNT  = 3
};

class VehicleRenderer {
public:
    void init();
    void cleanup();
    
    // Render a vehicle
    void renderVehicle(const RaycastVehicle& vehicle, float alpha = 1.0f);
    
    // Render ghost (translucent opponent)
    void renderGhost(Vector3 pos, Quaternion rot, float steer_angle,
                     uint8_t gear, bool left_ind, bool right_ind,
                     bool hazards, CarModel model, float alpha = 0.5f);

    // LOD selection based on distance
    [[nodiscard]] LODLevel selectLOD(float distance) const;
    void setLODDistances(float med, float low) { 
        lod_med_dist_ = med; 
        lod_low_dist_ = low; 
    }

    // Color customization
    void setPlayerColor(Color body, Color accent);

private:
    // Procedural mesh builders (Dr. Driving style)
    void buildCarMesh(CarModel model, LODLevel lod, CarMeshSet& out);
    void buildSedanMesh(CarMeshSet& out, int detail);
    void buildHatchbackMesh(CarMeshSet& out, int detail);
    void buildSUVMesh(CarMeshSet& out, int detail);
    void buildSportsMesh(CarMeshSet& out, int detail);   // McLaren silhouette
    void buildSuperMesh(CarMeshSet& out, int detail);    // Lambo silhouette
    void buildHyperMesh(CarMeshSet& out, int detail);    // Bugatti silhouette
    void buildWheelMesh(Model& out, int detail);
    
    void renderWheels(const RaycastVehicle& vehicle, const CarMeshSet& mesh);
    void renderIndicators(const RaycastVehicle& vehicle, const CarMeshSet& mesh);
    void renderBrakeLights(const RaycastVehicle& vehicle, const CarMeshSet& mesh);
    
    // Car meshes: [model][lod_level]
    std::array<std::array<CarMeshSet, 3>, static_cast<int>(CarModel::COUNT)> car_meshes_;
    
    Color player_body_color_   = {45, 85, 255, 255};   // Deep blue
    Color player_accent_color_ = {30, 30, 35, 255};
    
    float lod_med_dist_ = 30.0f;
    float lod_low_dist_ = 80.0f;
};

} // namespace drt
