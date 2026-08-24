#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Hardware Benchmark Engine
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>
#include <string>
#include <vector>

namespace drt {

enum class BenchmarkBadge : uint8_t {
    TOURNAMENT_READY,   // >= 60 FPS (green)
    PLAYABLE,           // 45-59 FPS (yellow)
    LAG_WARNING         // < 45 FPS (red)
};

struct BenchmarkResults {
    float  avg_fps      = 0.0f;
    float  low_1pct_fps = 0.0f;
    float  peak_ram_mb  = 0.0f;
    BenchmarkBadge badge = BenchmarkBadge::LAG_WARNING;
    std::string    badge_text;
    std::string    badge_emoji;
    bool   completed    = false;
};

class BenchmarkEngine {
public:
    void init();
    void startBenchmark();
    void update(float dt);
    void cleanup();

    [[nodiscard]] bool isRunning() const { return running_; }
    [[nodiscard]] float progress() const { return elapsed_ / duration_; }
    [[nodiscard]] const BenchmarkResults& results() const { return results_; }

    // Auto-detect best settings based on results
    void autoDetectSettings();

private:
    void spawnStressObjects();
    void updateStressScene(float dt);
    void finalizeBenchmark();
    float estimateRAMUsageMB() const;

    bool   running_   = false;
    float  elapsed_   = 0.0f;
    float  duration_  = 5.0f;    // 5-second benchmark

    // Frame time tracking
    std::vector<float> frame_times_;
    int    frame_count_ = 0;

    BenchmarkResults results_;
    
    // Stress test: 30+ AI cars
    int    stress_car_count_ = 30;
    bool   stress_scene_built_ = false;
};

} // namespace drt
