#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Procedural Road Generator
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <array>
#include <cstdint>

namespace drt {

// ── Road Segment / Chunk ───────────────────────────────────────────────────
struct RoadChunk {
    Vector3       start_pos;
    Vector3       end_pos;
    Vector3       start_tangent;
    Vector3       end_tangent;
    float         width;
    int           lanes_per_side;
    float         lane_width     = 3.5f;
    float         shoulder_width = 1.5f;
    Model         road_mesh      = {};
    Model         barrier_left   = {};
    Model         barrier_right  = {};
    BoundingBox   bounds         = {};
    bool          mesh_built     = false;
    int           chunk_index    = 0;
    float         length         = 0.0f;  // Arc length
};

struct LaneInfo {
    int   lane_index;     // 0 = leftmost
    float center_offset;  // Lateral offset from road center
    float speed_limit;    // km/h
    bool  is_oncoming;    // true = opposite direction
};

// ── Road Generator ─────────────────────────────────────────────────────────
class RoadGenerator {
public:
    void init(int lanes_per_side = 3, float lane_width = 3.5f);
    void generate(int num_chunks = 64);
    void update(Vector3 camera_pos);  // Stream/unload chunks
    void render();
    void cleanup();

    // Configuration
    void setLanesPerSide(int n)    { lanes_per_side_ = Clamp(n, 1, 6); }
    void setLaneWidth(float w)     { lane_width_ = w; }
    void setShoulderWidth(float w) { shoulder_width_ = w; }
    
    [[nodiscard]] int   lanesPerSide() const { return lanes_per_side_; }
    [[nodiscard]] float laneWidth()    const { return lane_width_; }
    [[nodiscard]] float totalWidth()   const;

    // Lane queries
    [[nodiscard]] float getLaneCenterOffset(int lane, bool oncoming = false) const;
    [[nodiscard]] int   getTotalLaneCount() const { return lanes_per_side_ * 2; }
    
    // Spline evaluation
    [[nodiscard]] Vector3 evaluatePosition(float t) const;
    [[nodiscard]] Vector3 evaluateTangent(float t) const;
    [[nodiscard]] float   getTotalLength() const;
    
    // World-space road surface Y at position
    [[nodiscard]] float getRoadSurfaceY(Vector3 world_pos) const;
    
    // Barrier bounding boxes
    [[nodiscard]] const std::vector<BoundingBox>& barriers() const { return barriers_; }
    
    // Road chunks for rendering
    [[nodiscard]] const std::vector<RoadChunk>& chunks() const { return chunks_; }

private:
    void buildChunkMesh(RoadChunk& chunk);
    void buildBarriers(RoadChunk& chunk);
    Mesh generateRoadSurface(const RoadChunk& chunk, int subdivisions = 16);
    Mesh generateBarrierMesh(const RoadChunk& chunk, bool left_side);
    
    // Catmull-Rom helpers
    Vector3 catmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) const;
    
    std::vector<RoadChunk>   chunks_;
    std::vector<Vector3>     control_points_;
    std::vector<BoundingBox> barriers_;
    
    int   lanes_per_side_  = 3;
    float lane_width_      = 3.5f;
    float shoulder_width_  = 1.5f;
    float chunk_length_    = 50.0f;
    int   visible_chunks_  = 12;
    float road_y_          = 0.0f;
};

} // namespace drt
