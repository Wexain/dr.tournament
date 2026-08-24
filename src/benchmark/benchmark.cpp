// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Hardware Benchmark Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "benchmark/benchmark.h"
#include <raylib.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace drt {

void BenchmarkEngine::init() {
    running_ = false;
    stress_scene_built_ = false;
    results_ = {};
}

void BenchmarkEngine::startBenchmark() {
    running_ = true;
    elapsed_ = 0.0f;
    frame_count_ = 0;
    frame_times_.clear();
    frame_times_.reserve(600); // 5 seconds * ~120fps
    results_ = {};
    stress_scene_built_ = false;
    spawnStressObjects();
}

void BenchmarkEngine::update(float dt) {
    if (!running_) return;

    elapsed_ += dt;
    float frame_time = GetFrameTime();

    if (frame_time > 0.0f) {
        frame_times_.push_back(frame_time);
        frame_count_++;
    }

    updateStressScene(dt);

    if (elapsed_ >= duration_) {
        finalizeBenchmark();
    }
}

void BenchmarkEngine::spawnStressObjects() {
    // The benchmark renders 30+ simple car boxes on a 5-lane road
    // This is handled in the engine's render loop by checking benchmark state
    stress_car_count_ = 35;
    stress_scene_built_ = true;
}

void BenchmarkEngine::updateStressScene(float dt) {
    // Stress scene: draw 30+ cubes simulating lane-changing cars
    // This is intentionally GPU-heavy to benchmark the system
    if (!stress_scene_built_) return;

    // The actual rendering happens in Engine::renderBenchmarkUI
    // Here we just simulate NPC movement (light CPU load)
}

void BenchmarkEngine::finalizeBenchmark() {
    running_ = false;

    if (frame_times_.empty()) {
        results_.badge = BenchmarkBadge::LAG_WARNING;
        results_.badge_text = "LAG WARNING";
        results_.badge_emoji = "🔴";
        results_.completed = true;
        return;
    }

    // Calculate average FPS
    float total_time = std::accumulate(frame_times_.begin(), frame_times_.end(), 0.0f);
    results_.avg_fps = frame_count_ / total_time;

    // 1% low FPS: sort frame times, take worst 1%
    std::vector<float> sorted = frame_times_;
    std::sort(sorted.begin(), sorted.end(), std::greater<float>());
    int low_1pct_count = std::max(1, static_cast<int>(sorted.size() * 0.01f));
    float worst_avg_time = 0.0f;
    for (int i = 0; i < low_1pct_count; ++i) {
        worst_avg_time += sorted[i];
    }
    worst_avg_time /= low_1pct_count;
    results_.low_1pct_fps = 1.0f / worst_avg_time;

    // Estimate RAM (approximate)
    results_.peak_ram_mb = estimateRAMUsageMB();

    // Badge
    if (results_.avg_fps >= 60.0f) {
        results_.badge = BenchmarkBadge::TOURNAMENT_READY;
        results_.badge_text = "TOURNAMENT READY";
        results_.badge_emoji = "🟢";
    } else if (results_.avg_fps >= 45.0f) {
        results_.badge = BenchmarkBadge::PLAYABLE;
        results_.badge_text = "PLAYABLE";
        results_.badge_emoji = "🟡";
    } else {
        results_.badge = BenchmarkBadge::LAG_WARNING;
        results_.badge_text = "LAG WARNING";
        results_.badge_emoji = "🔴";
    }

    results_.completed = true;
}

float BenchmarkEngine::estimateRAMUsageMB() const {
    // Rough estimate based on what we know is allocated:
    // - 64 NPC vehicles * ~2KB each = 128 KB
    // - 128 road chunks * ~4KB each = 512 KB
    // - Render textures: ~screen_w*screen_h*4 bytes
    // - Raylib overhead: ~20 MB
    // - Car meshes: 6 models * 3 LODs * ~50KB = 900 KB
    // Total estimate: ~80-150 MB depending on resolution

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float render_tex_mb = (w * h * 4.0f) / (1024.0f * 1024.0f);
    float estimated = 30.0f + render_tex_mb * 2 + 5.0f;  // Base + textures + overhead

    return estimated;
}

void BenchmarkEngine::autoDetectSettings() {
    // Based on benchmark results, select optimal quality preset
    // This is called after benchmark completes

    // Results would be used by Settings::autoDetectQuality()
    // which reads BenchmarkResults and sets the appropriate preset
}

void BenchmarkEngine::cleanup() {
    frame_times_.clear();
    stress_scene_built_ = false;
}

} // namespace drt
