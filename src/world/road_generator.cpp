// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Procedural Road Generator Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "world/road_generator.h"
#include <cmath>
#include <algorithm>

namespace drt {

void RoadGenerator::init(int lanes_per_side, float lane_width) {
    lanes_per_side_ = lanes_per_side;
    lane_width_ = lane_width;
    shoulder_width_ = 1.5f;
    chunk_length_ = 50.0f;
    road_y_ = 0.01f;  // Slightly above ground
}

void RoadGenerator::generate(int num_chunks) {
    cleanup();

    // Generate control points along Z axis with gentle curves
    control_points_.clear();
    float z = -chunk_length_;
    float x = 0.0f;
    float curve_freq = 0.003f;  // How frequently curves occur
    float curve_amp = 30.0f;    // Curve amplitude

    for (int i = 0; i < num_chunks + 4; ++i) {
        // Gentle sinusoidal curves
        x = sinf(z * curve_freq) * curve_amp;
        control_points_.push_back({x, road_y_, z});
        z += chunk_length_;
    }

    // Build chunks
    chunks_.resize(num_chunks);
    for (int i = 0; i < num_chunks; ++i) {
        auto& chunk = chunks_[i];
        chunk.chunk_index = i;
        chunk.lanes_per_side = lanes_per_side_;
        chunk.lane_width = lane_width_;
        chunk.shoulder_width = shoulder_width_;

        // Use control points for Catmull-Rom
        int cp = i + 1;  // Offset by 1 since we need p[i-1]
        chunk.start_pos = control_points_[cp];
        chunk.end_pos = control_points_[cp + 1];

        if (cp > 0 && cp + 2 < (int)control_points_.size()) {
            chunk.start_tangent = Vector3Scale(
                Vector3Subtract(control_points_[cp + 1], control_points_[cp - 1]), 0.5f);
            chunk.end_tangent = Vector3Scale(
                Vector3Subtract(control_points_[cp + 2], control_points_[cp]), 0.5f);
        }

        chunk.width = totalWidth();
        chunk.length = Vector3Distance(chunk.start_pos, chunk.end_pos);
        buildChunkMesh(chunk);
    }
}

void RoadGenerator::buildChunkMesh(RoadChunk& chunk) {
    // Generate road surface mesh
    Mesh road_mesh = generateRoadSurface(chunk, 16);
    chunk.road_mesh = LoadModelFromMesh(road_mesh);

    // Set road material color (dark asphalt)
    chunk.road_mesh.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {60, 60, 65, 255};

    // Generate barriers
    buildBarriers(chunk);

    // Compute bounding box
    float hw = chunk.width * 0.5f + 2.0f;
    chunk.bounds = {
        {std::min(chunk.start_pos.x, chunk.end_pos.x) - hw, -1.0f,
         std::min(chunk.start_pos.z, chunk.end_pos.z) - 1.0f},
        {std::max(chunk.start_pos.x, chunk.end_pos.x) + hw, 3.0f,
         std::max(chunk.start_pos.z, chunk.end_pos.z) + 1.0f}
    };

    chunk.mesh_built = true;
}

Mesh RoadGenerator::generateRoadSurface(const RoadChunk& chunk, int subdivisions) {
    float hw = totalWidth() * 0.5f;
    int verts_per_row = 2;  // Left and right edge
    int rows = subdivisions + 1;
    int vertex_count = rows * verts_per_row;
    int triangle_count = subdivisions * 2;

    Mesh mesh = {0};
    mesh.vertexCount = vertex_count;
    mesh.triangleCount = triangle_count;
    mesh.vertices = (float*)MemAlloc(vertex_count * 3 * sizeof(float));
    mesh.normals = (float*)MemAlloc(vertex_count * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(vertex_count * 2 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(triangle_count * 3 * sizeof(unsigned short));

    // Generate vertices along the spline
    for (int row = 0; row < rows; ++row) {
        float t = static_cast<float>(row) / subdivisions;
        Vector3 center = Vector3Lerp(chunk.start_pos, chunk.end_pos, t);

        // Calculate road direction
        Vector3 dir = Vector3Normalize(Vector3Subtract(chunk.end_pos, chunk.start_pos));
        Vector3 road_right = Vector3Normalize(Vector3CrossProduct(dir, {0, 1, 0}));

        // Left edge
        int vi = (row * 2) * 3;
        Vector3 left = Vector3Subtract(center, Vector3Scale(road_right, hw));
        mesh.vertices[vi + 0] = left.x;
        mesh.vertices[vi + 1] = left.y;
        mesh.vertices[vi + 2] = left.z;

        // Right edge
        int vi2 = (row * 2 + 1) * 3;
        Vector3 right_pt = Vector3Add(center, Vector3Scale(road_right, hw));
        mesh.vertices[vi2 + 0] = right_pt.x;
        mesh.vertices[vi2 + 1] = right_pt.y;
        mesh.vertices[vi2 + 2] = right_pt.z;

        // Normals (up)
        for (int v = 0; v < 2; ++v) {
            int ni = (row * 2 + v) * 3;
            mesh.normals[ni + 0] = 0.0f;
            mesh.normals[ni + 1] = 1.0f;
            mesh.normals[ni + 2] = 0.0f;
        }

        // UVs
        int ti = (row * 2) * 2;
        mesh.texcoords[ti + 0] = 0.0f;
        mesh.texcoords[ti + 1] = t;
        mesh.texcoords[ti + 2] = 1.0f;
        mesh.texcoords[ti + 3] = t;
    }

    // Generate indices
    int idx = 0;
    for (int row = 0; row < subdivisions; ++row) {
        int bl = row * 2;
        int br = row * 2 + 1;
        int tl = (row + 1) * 2;
        int tr = (row + 1) * 2 + 1;

        mesh.indices[idx++] = bl;
        mesh.indices[idx++] = tl;
        mesh.indices[idx++] = br;

        mesh.indices[idx++] = br;
        mesh.indices[idx++] = tl;
        mesh.indices[idx++] = tr;
    }

    UploadMesh(&mesh, false);
    return mesh;
}

void RoadGenerator::buildBarriers(RoadChunk& chunk) {
    float hw = totalWidth() * 0.5f + shoulder_width_;
    float barrier_h = 0.8f;

    // Left barrier
    Vector3 dir = Vector3Normalize(Vector3Subtract(chunk.end_pos, chunk.start_pos));
    Vector3 road_right = Vector3Normalize(Vector3CrossProduct(dir, {0, 1, 0}));

    // Create barrier bounding boxes
    Vector3 left_center = Vector3Subtract(
        Vector3Scale(Vector3Add(chunk.start_pos, chunk.end_pos), 0.5f),
        Vector3Scale(road_right, hw));
    BoundingBox left_bb = {
        Vector3Subtract(left_center, {0.3f, 0, chunk.length * 0.5f}),
        Vector3Add(left_center, {0.3f, barrier_h, chunk.length * 0.5f})
    };
    barriers_.push_back(left_bb);

    Vector3 right_center = Vector3Add(
        Vector3Scale(Vector3Add(chunk.start_pos, chunk.end_pos), 0.5f),
        Vector3Scale(road_right, hw));
    BoundingBox right_bb = {
        Vector3Subtract(right_center, {0.3f, 0, chunk.length * 0.5f}),
        Vector3Add(right_center, {0.3f, barrier_h, chunk.length * 0.5f})
    };
    barriers_.push_back(right_bb);
}

Mesh RoadGenerator::generateBarrierMesh(const RoadChunk& chunk, bool left_side) {
    // Stub — barriers use AABB collision, rendered as simple boxes
    Mesh mesh = {0};
    return mesh;
}

void RoadGenerator::update(Vector3 camera_pos) {
    // In a full implementation, this would stream/unload distant chunks
    // For now, all chunks remain loaded
}

void RoadGenerator::render() {
    for (auto& chunk : chunks_) {
        if (!chunk.mesh_built) continue;
        DrawModel(chunk.road_mesh, {0, 0, 0}, 1.0f, WHITE);

        // Draw lane markings
        Vector3 dir = Vector3Normalize(Vector3Subtract(chunk.end_pos, chunk.start_pos));
        Vector3 road_right = Vector3Normalize(Vector3CrossProduct(dir, {0, 1, 0}));
        float hw = totalWidth() * 0.5f;

        // Center divider (double yellow)
        Vector3 center_start = chunk.start_pos;
        Vector3 center_end = chunk.end_pos;
        center_start.y += 0.02f;
        center_end.y += 0.02f;
        DrawLine3D(center_start, center_end, YELLOW);

        // Lane dividers (dashed white)
        for (int lane = 1; lane < lanes_per_side_; ++lane) {
            float offset = lane * lane_width_;

            // Right side lanes
            Vector3 ls = Vector3Add(chunk.start_pos, Vector3Scale(road_right, offset));
            Vector3 le = Vector3Add(chunk.end_pos, Vector3Scale(road_right, offset));
            ls.y += 0.02f; le.y += 0.02f;

            // Dashed: draw segments
            int dashes = static_cast<int>(chunk.length / 3.0f);
            for (int d = 0; d < dashes; d += 2) {
                float t0 = (float)d / dashes;
                float t1 = (float)(d + 1) / dashes;
                Vector3 p0 = Vector3Lerp(ls, le, t0);
                Vector3 p1 = Vector3Lerp(ls, le, t1);
                DrawLine3D(p0, p1, WHITE);
            }

            // Left side lanes (mirrored)
            ls = Vector3Subtract(chunk.start_pos, Vector3Scale(road_right, offset));
            le = Vector3Subtract(chunk.end_pos, Vector3Scale(road_right, offset));
            ls.y += 0.02f; le.y += 0.02f;
            for (int d = 0; d < dashes; d += 2) {
                float t0 = (float)d / dashes;
                float t1 = (float)(d + 1) / dashes;
                DrawLine3D(Vector3Lerp(ls, le, t0), Vector3Lerp(ls, le, t1), WHITE);
            }
        }

        // Road edges (solid white)
        Vector3 edge_l_s = Vector3Subtract(chunk.start_pos, Vector3Scale(road_right, hw));
        Vector3 edge_l_e = Vector3Subtract(chunk.end_pos, Vector3Scale(road_right, hw));
        Vector3 edge_r_s = Vector3Add(chunk.start_pos, Vector3Scale(road_right, hw));
        Vector3 edge_r_e = Vector3Add(chunk.end_pos, Vector3Scale(road_right, hw));
        edge_l_s.y += 0.02f; edge_l_e.y += 0.02f;
        edge_r_s.y += 0.02f; edge_r_e.y += 0.02f;
        DrawLine3D(edge_l_s, edge_l_e, WHITE);
        DrawLine3D(edge_r_s, edge_r_e, WHITE);

        // Barriers (simple colored boxes)
        for (const auto& bb : barriers_) {
            Vector3 size = Vector3Subtract(bb.max, bb.min);
            Vector3 center = Vector3Scale(Vector3Add(bb.min, bb.max), 0.5f);
            DrawCubeV(center, size, {100, 100, 110, 180});
            DrawCubeWiresV(center, size, {140, 140, 150, 200});
        }
    }
}

void RoadGenerator::cleanup() {
    for (auto& chunk : chunks_) {
        if (chunk.mesh_built) {
            UnloadModel(chunk.road_mesh);
        }
    }
    chunks_.clear();
    control_points_.clear();
    barriers_.clear();
}

float RoadGenerator::totalWidth() const {
    return lanes_per_side_ * 2 * lane_width_ + shoulder_width_ * 2;
}

float RoadGenerator::getLaneCenterOffset(int lane, bool oncoming) const {
    float offset = (lane + 0.5f) * lane_width_;
    return oncoming ? -offset : offset;
}

Vector3 RoadGenerator::evaluatePosition(float t) const {
    if (chunks_.empty()) return {0, 0, 0};
    float chunk_t = t * chunks_.size();
    int ci = static_cast<int>(chunk_t);
    ci = std::clamp(ci, 0, (int)chunks_.size() - 1);
    float local_t = chunk_t - ci;
    return Vector3Lerp(chunks_[ci].start_pos, chunks_[ci].end_pos, local_t);
}

Vector3 RoadGenerator::evaluateTangent(float t) const {
    if (chunks_.empty()) return {0, 0, 1};
    float chunk_t = t * chunks_.size();
    int ci = static_cast<int>(chunk_t);
    ci = std::clamp(ci, 0, (int)chunks_.size() - 1);
    return Vector3Normalize(Vector3Subtract(chunks_[ci].end_pos, chunks_[ci].start_pos));
}

float RoadGenerator::getTotalLength() const {
    float total = 0;
    for (const auto& c : chunks_) total += c.length;
    return total;
}

float RoadGenerator::getRoadSurfaceY(Vector3 world_pos) const {
    return road_y_;
}

Vector3 RoadGenerator::catmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) const {
    float t2 = t * t;
    float t3 = t2 * t;
    return {
        0.5f * (2*p1.x + (-p0.x+p2.x)*t + (2*p0.x-5*p1.x+4*p2.x-p3.x)*t2 + (-p0.x+3*p1.x-3*p2.x+p3.x)*t3),
        0.5f * (2*p1.y + (-p0.y+p2.y)*t + (2*p0.y-5*p1.y+4*p2.y-p3.y)*t2 + (-p0.y+3*p1.y-3*p2.y+p3.y)*t3),
        0.5f * (2*p1.z + (-p0.z+p2.z)*t + (2*p0.z-5*p1.z+4*p2.z-p3.z)*t2 + (-p0.z+3*p1.z-3*p2.z+p3.z)*t3)
    };
}

} // namespace drt
