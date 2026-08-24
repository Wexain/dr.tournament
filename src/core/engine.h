#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Core Engine
// ═══════════════════════════════════════════════════════════════════════════
#include "core/memory_pool.h"
#include "core/input_system.h"
#include "core/camera_system.h"
#include "core/settings.h"
#include "physics/raycast_vehicle.h"
#include "physics/collision.h"
#include "world/road_generator.h"
#include "world/traffic_ai.h"
#include "world/environment.h"
#include "render/vehicle_renderer.h"
#include "render/hud.h"
#include "render/fx.h"
#include "render/android_ui.h"
#include "net/enet_client.h"
#include "net/protocol.h"
#include "social/auth.h"
#include "social/economy.h"
#include "benchmark/benchmark.h"
#include "tournament/rules.h"
#include "tournament/moderator.h"

#include <raylib.h>
#include <cstdint>
#include <string>
#include <array>

namespace drt {

// ── Game States ────────────────────────────────────────────────────────────
enum class GameState : uint8_t {
    SPLASH,
    MAIN_MENU,
    SETTINGS_MENU,
    BENCHMARK_RUNNING,
    BENCHMARK_RESULTS,
    GARAGE,
    MULTIPLAYER_LOBBY,
    CONNECTING,
    RACING,
    RACE_RESULTS,
    SPECTATING
};

// ── Engine ─────────────────────────────────────────────────────────────────
class Engine {
public:
    Engine() = default;
    ~Engine() = default;

    // Non-copyable, non-movable (singleton-ish lifetime)
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool init(int argc = 0, char** argv = nullptr);
    void run();          // Main loop
    void shutdown();

    [[nodiscard]] bool shouldClose() const;

    // Subsystem access
    InputSystem&       input()       { return input_; }
    CameraSystem&      camera()      { return camera_; }
    Settings&          settings()    { return settings_; }
    const Settings&    settings() const  { return settings_; }
    RoadGenerator&     road()        { return road_; }
    TrafficAI&         traffic()     { return traffic_; }
    Environment&       env()         { return env_; }
    VehicleRenderer&   vehRenderer() { return veh_renderer_; }
    HUD&               hud()         { return hud_; }
    PostFX&            fx()          { return fx_; }
    ENetClient&        netClient()       { return net_client_; }
    const ENetClient&  netClient() const { return net_client_; }
    AuthSystem&        auth()        { return auth_; }
    Economy&           economy()     { return economy_; }
    TournamentRules&   rules()       { return rules_; }
    BenchmarkEngine&   benchmark()   { return benchmark_; }

    // State management
    void              setState(GameState s) { state_ = s; }
    [[nodiscard]] GameState state() const   { return state_; }

    // Player vehicle access
    RaycastVehicle&       playerVehicle()       { return player_vehicle_; }
    const RaycastVehicle& playerVehicle() const { return player_vehicle_; }

    // Object pools
    FixedPool<RaycastVehicle, MAX_NPC_CARS>& npcPool() { return npc_pool_; }

    // Timing
    [[nodiscard]] float dt()        const { return FIXED_DT; }
    [[nodiscard]] float gameTime()  const { return game_time_; }
    [[nodiscard]] float renderDt()  const { return render_dt_; }
    [[nodiscard]] int   fps()       const { return current_fps_; }

    // Multiplayer ghost vehicles
    struct GhostVehicle {
        Vector3    position = {0, 0, 0};
        Quaternion rotation = {0, 0, 0, 0};
        Vector3    velocity = {0, 0, 0};
        uint8_t    gear = 0;
        bool       left_indicator  = false;
        bool       right_indicator = false;
        bool       hazards = false;
        bool       reverse_light   = false;
        float      steer_angle = 0.0f;
        std::string nickname;
        bool       active = false;
    };
    static constexpr int MAX_GHOSTS = 15;
    std::array<GhostVehicle, MAX_GHOSTS> ghosts{};

private:
    void processInput();
    void fixedUpdate(float dt);
    void render();
    void renderUI();

    // State handlers
    void updateSplash();
    void updateMainMenu();
    void updateSettingsMenu();
    void updateBenchmark();
    void updateGarage();
    void updateMultiplayerLobby();
    void updateConnecting();
    void updateRacing();
    void updateRaceResults();

    void renderSplash();
    void renderMainMenu();
    void renderSettingsMenu();
    void renderBenchmarkUI();
    void renderGarage();
    void renderMultiplayerLobby();
    void renderRacing();
    void renderRaceResults();

    // Fixed timestep
    static constexpr float FIXED_DT = 1.0f / 60.0f;
    float accumulator_  = 0.0f;
    float game_time_    = 0.0f;
    float render_dt_    = 0.0f;
    int   current_fps_  = 0;

    // State
    GameState state_ = GameState::SPLASH;
    float     splash_timer_ = 0.0f;

    // Core subsystems
    InputSystem     input_;
    CameraSystem    camera_;
    Settings        settings_;

    // Physics
    RaycastVehicle  player_vehicle_;
    CollisionSystem collision_;

    // World
    RoadGenerator   road_;
    TrafficAI       traffic_;
    Environment     env_;

    // Rendering
    VehicleRenderer veh_renderer_;
    HUD             hud_;
    PostFX          fx_;
    AndroidUI       android_ui_;

    // Networking
    ENetClient      net_client_;

    // Social
    AuthSystem      auth_;
    Economy         economy_;

    // Tournament
    TournamentRules rules_;
    ModeratorSystem moderator_;
    BenchmarkEngine benchmark_;

    // Object pools
    FixedPool<RaycastVehicle, MAX_NPC_CARS> npc_pool_;

    // Command-line flags
    bool run_benchmark_ = false;
    bool headless_       = false;
};

} // namespace drt
