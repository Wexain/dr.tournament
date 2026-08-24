// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Core Engine Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "core/engine.h"
#include <raylib.h>
#include <raymath.h>
#include <cstring>
#include <cstdio>

namespace drt {

bool Engine::init(int argc, char** argv) {
    // Parse command-line flags
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--benchmark") == 0) run_benchmark_ = true;
        if (strcmp(argv[i], "--headless") == 0)  headless_ = true;
    }

    // Load settings
    settings_.init();
    settings_.load();

    // Initialize window
    int w = settings_.graphics().resolution_w;
    int h = settings_.graphics().resolution_h;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    if (settings_.graphics().vsync) SetConfigFlags(FLAG_VSYNC_HINT);
    if (settings_.graphics().msaa == 4) SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(w, h, "Dr. Tournaments");
    SetTargetFPS(0);  // Uncapped — we handle timing ourselves
    InitAudioDevice();

    if (settings_.graphics().fullscreen) ToggleFullscreen();

    // Initialize subsystems
    input_.init(w, h);
    camera_.init();
    road_.init(settings_.graphics().lod_level >= 1 ? 3 : 2);
    road_.generate(64);
    traffic_.init(&road_);
    env_.init();
    veh_renderer_.init();
    hud_.init(w, h);
    fx_.init(w, h);
    net_client_.init();
    auth_.init();
    auth_.loadLocal();
    economy_.init();
    economy_.load();
    rules_.init();
    benchmark_.init();
    collision_.init();

    // Initialize player vehicle
    player_vehicle_.init(CarModel::SEDAN);
    player_vehicle_.reset({0, 1.0f, 5.0f}, 0.0f);

    // Apply graphics settings
    settings_.applyGraphics();

    state_ = GameState::SPLASH;
    splash_timer_ = 0.0f;

    if (run_benchmark_) {
        state_ = GameState::BENCHMARK_RUNNING;
        benchmark_.startBenchmark();
    }

    return true;
}

void Engine::run() {
    while (!shouldClose()) {
        // Variable timestep for rendering
        render_dt_ = GetFrameTime();
        current_fps_ = GetFPS();

        // Handle window resize
        if (IsWindowResized()) {
            int w = GetScreenWidth();
            int h = GetScreenHeight();
            input_.onResize(w, h);
            hud_.onResize(w, h);
            fx_.onResize(w, h);
        }

        // Input
        processInput();

        // Fixed-timestep physics accumulator
        accumulator_ += render_dt_;
        // Cap accumulator to prevent spiral of death
        if (accumulator_ > 0.2f) accumulator_ = 0.2f;

        while (accumulator_ >= FIXED_DT) {
            fixedUpdate(FIXED_DT);
            accumulator_ -= FIXED_DT;
            game_time_ += FIXED_DT;
        }

        // Network update (variable rate)
        net_client_.update(render_dt_);

        // Render
        render();
    }
}

void Engine::shutdown() {
    settings_.save();
    auth_.saveLocal();
    economy_.save();

    veh_renderer_.cleanup();
    road_.cleanup();
    traffic_.cleanup();
    env_.cleanup();
    fx_.cleanup();
    net_client_.shutdown();

    CloseAudioDevice();
    CloseWindow();
}

bool Engine::shouldClose() const {
    return WindowShouldClose();
}

void Engine::processInput() {
    input_.update(render_dt_);
    const auto& inp = input_.current();

    // Global hotkeys
    if (inp.toggle_settings && state_ == GameState::RACING) {
        state_ = GameState::SETTINGS_MENU;
    }
    if (inp.camera_cycle && state_ == GameState::RACING) {
        camera_.cycleMode();
    }
    if (state_ == GameState::RACING) {
        camera_.setRearView(inp.rear_view);
    }
}

void Engine::fixedUpdate(float dt) {
    switch (state_) {
        case GameState::SPLASH:
            updateSplash();
            break;
        case GameState::MAIN_MENU:
            updateMainMenu();
            break;
        case GameState::SETTINGS_MENU:
            updateSettingsMenu();
            break;
        case GameState::BENCHMARK_RUNNING:
            updateBenchmark();
            break;
        case GameState::GARAGE:
            updateGarage();
            break;
        case GameState::MULTIPLAYER_LOBBY:
            updateMultiplayerLobby();
            break;
        case GameState::CONNECTING:
            updateConnecting();
            break;
        case GameState::RACING:
            updateRacing();
            break;
        case GameState::RACE_RESULTS:
            updateRaceResults();
            break;
        default: break;
    }
}

void Engine::render() {
    BeginDrawing();
    ClearBackground({20, 20, 25, 255});

    switch (state_) {
        case GameState::SPLASH:         renderSplash(); break;
        case GameState::MAIN_MENU:      renderMainMenu(); break;
        case GameState::SETTINGS_MENU:  renderSettingsMenu(); break;
        case GameState::BENCHMARK_RUNNING:
        case GameState::BENCHMARK_RESULTS:
            renderBenchmarkUI(); break;
        case GameState::GARAGE:         renderGarage(); break;
        case GameState::MULTIPLAYER_LOBBY: renderMultiplayerLobby(); break;
        case GameState::RACING:
        case GameState::SPECTATING:
            renderRacing(); break;
        case GameState::RACE_RESULTS:   renderRaceResults(); break;
        default: break;
    }

    // Debug info overlay
    #ifndef NDEBUG
    DrawText(TextFormat("FPS: %d  State: %d", current_fps_, (int)state_),
             10, 10, 16, LIME);
    #endif

    EndDrawing();
}

// ── State Handlers ─────────────────────────────────────────────────────────

void Engine::updateSplash() {
    splash_timer_ += FIXED_DT;
    if (splash_timer_ > 2.5f) {
        state_ = GameState::MAIN_MENU;
    }
}

void Engine::updateMainMenu() {
    const auto& inp = input_.current();
    // Menu navigation handled in render
}

void Engine::updateSettingsMenu() {
    if (input_.current().menu_back) {
        state_ = GameState::MAIN_MENU;
    }
}

void Engine::updateBenchmark() {
    benchmark_.update(FIXED_DT);
    if (!benchmark_.isRunning()) {
        state_ = GameState::BENCHMARK_RESULTS;
    }
}

void Engine::updateGarage() {
    // Car selection, customization
}

void Engine::updateMultiplayerLobby() {
    // Room browsing, creation
}

void Engine::updateConnecting() {
    if (net_client_.isConnected()) {
        state_ = GameState::RACING;
    }
}

void Engine::updateRacing() {
    const auto& inp = input_.current();

    // Gear shifting
    if (inp.shift_park)    player_vehicle_.transmission().shiftTo(GearPosition::PARK);
    if (inp.shift_reverse) player_vehicle_.transmission().shiftTo(GearPosition::REVERSE);
    if (inp.shift_neutral) player_vehicle_.transmission().shiftTo(GearPosition::NEUTRAL);
    if (inp.shift_drive)   player_vehicle_.transmission().shiftTo(GearPosition::DRIVE);

    // Vehicle indicators
    if (inp.indicator_left)  player_vehicle_.left_indicator = !player_vehicle_.left_indicator;
    if (inp.indicator_right) player_vehicle_.right_indicator = !player_vehicle_.right_indicator;
    if (inp.hazards)         player_vehicle_.hazards = !player_vehicle_.hazards;
    player_vehicle_.horn_active = inp.horn;

    // Update vehicle physics
    player_vehicle_.update(FIXED_DT, inp.gas, inp.brake, inp.steering,
                           inp.handbrake, rules_.surfaceFriction());

    // Update traffic AI
    traffic_.update(FIXED_DT, player_vehicle_.position(), player_vehicle_.speedKmh());

    // Update collisions
    collision_.update(player_vehicle_,
                      reinterpret_cast<RaycastVehicle*>(traffic_.npcs()),
                      traffic_.activeCount(),
                      road_.barriers().data(),
                      static_cast<int>(road_.barriers().size()));

    // Update camera
    camera_.update(player_vehicle_.position(), player_vehicle_.forward(),
                   player_vehicle_.up(), FIXED_DT);

    // Update environment
    env_.update(FIXED_DT);

    // Send network state
    if (net_client_.isConnected()) {
        StatePacket pkt;
        pkt.type = PacketType::STATE_UPDATE;
        pkt.player_id = net_client_.localPlayerId();
        pkt.pos_x = player_vehicle_.position().x;
        pkt.pos_y = player_vehicle_.position().y;
        pkt.pos_z = player_vehicle_.position().z;
        auto rot = player_vehicle_.rotation();
        pkt.rot_x = rot.x; pkt.rot_y = rot.y;
        pkt.rot_z = rot.z; pkt.rot_w = rot.w;
        pkt.vel_x = player_vehicle_.velocity().x;
        pkt.vel_y = player_vehicle_.velocity().y;
        pkt.vel_z = player_vehicle_.velocity().z;
        pkt.steer_angle = player_vehicle_.steerAngle();
        pkt.gear = static_cast<uint8_t>(player_vehicle_.transmission().position());
        pkt.flags = 0;
        if (player_vehicle_.left_indicator)  pkt.flags |= 0x01;
        if (player_vehicle_.right_indicator) pkt.flags |= 0x02;
        if (player_vehicle_.hazards)         pkt.flags |= 0x04;
        if (player_vehicle_.transmission().isReverse()) pkt.flags |= 0x08;
        if (player_vehicle_.horn_active)     pkt.flags |= 0x10;
        pkt.speed_cmps = static_cast<uint16_t>(player_vehicle_.speed() * 100.0f);
        net_client_.sendStateUpdate(pkt);
    }

    // Leaderboard toggle
    hud_.setShowLeaderboard(inp.show_leaderboard);
}

void Engine::updateRaceResults() {
    // Wait for input to return to menu
    if (input_.current().menu_confirm) {
        state_ = GameState::MAIN_MENU;
    }
}

// ── Render State Handlers ──────────────────────────────────────────────────

void Engine::renderSplash() {
    float alpha = splash_timer_ < 0.5f ? splash_timer_ * 2.0f :
                  splash_timer_ > 2.0f ? (2.5f - splash_timer_) * 2.0f : 1.0f;
    int a = static_cast<int>(alpha * 255);

    const char* title = "DR. TOURNAMENTS";
    int tw = MeasureText(title, 60);
    DrawText(title, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 40,
             60, {255, 255, 255, (unsigned char)a});

    const char* sub = "Precision Driving";
    int sw = MeasureText(sub, 24);
    DrawText(sub, GetScreenWidth() / 2 - sw / 2, GetScreenHeight() / 2 + 30,
             24, {180, 180, 200, (unsigned char)a});
}

void Engine::renderMainMenu() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    // Title
    const char* title = "DR. TOURNAMENTS";
    int tw = MeasureText(title, 48);
    DrawText(title, w / 2 - tw / 2, h / 6, 48, WHITE);

    // Gradient line under title
    DrawRectangleGradientH(w / 4, h / 6 + 55, w / 2, 3,
                           {0, 150, 255, 255}, {255, 100, 0, 255});

    // Menu items
    struct MenuItem { const char* label; GameState target; };
    MenuItem items[] = {
        {"PLAY",         GameState::RACING},
        {"MULTIPLAYER",  GameState::MULTIPLAYER_LOBBY},
        {"GARAGE",       GameState::GARAGE},
        {"BENCHMARK",    GameState::BENCHMARK_RUNNING},
        {"SETTINGS",     GameState::SETTINGS_MENU},
    };

    int menu_y = h / 3;
    for (int i = 0; i < 5; ++i) {
        Rectangle btn = {(float)(w / 2 - 140), (float)(menu_y + i * 60), 280, 48};
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        Color bg = hover ? Color{50, 100, 200, 200} : Color{40, 40, 55, 180};
        DrawRectangleRounded(btn, 0.3f, 8, bg);
        DrawRectangleRoundedLines(btn, 0.3f, 8, hover ? WHITE : Color{80, 80, 100, 200});

        int tl = MeasureText(items[i].label, 22);
        DrawText(items[i].label, w / 2 - tl / 2, menu_y + i * 60 + 13, 22,
                 hover ? WHITE : Color{180, 180, 200, 255});

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (items[i].target == GameState::BENCHMARK_RUNNING) {
                benchmark_.startBenchmark();
            }
            if (items[i].target == GameState::RACING) {
                // Reset vehicle for new race
                player_vehicle_.reset({0, 1.0f, 5.0f}, 0.0f);
                player_vehicle_.transmission().shiftTo(GearPosition::DRIVE);
                traffic_.spawnTraffic(player_vehicle_.position(), 20);
            }
            state_ = items[i].target;
        }
    }

    // Version & info
    DrawText("v0.1.0  |  C++20 + Raylib + ENet", 10, h - 25, 14, {100, 100, 120, 200});

    // Nickname display
    DrawText(TextFormat("Player: %s", auth_.profile().nickname.c_str()),
             w - 200, 10, 16, {150, 200, 255, 200});
}

void Engine::renderSettingsMenu() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawText("SETTINGS", w / 2 - MeasureText("SETTINGS", 36) / 2, 40, 36, WHITE);
    DrawText("Press ESC to go back", w / 2 - MeasureText("Press ESC to go back", 16) / 2,
             h - 40, 16, {120, 120, 140, 200});

    int y = 120;
    auto drawSetting = [&](const char* label, const char* value) {
        DrawText(label, 100, y, 20, {180, 180, 200, 255});
        DrawText(value, w - 300, y, 20, WHITE);
        y += 40;
    };

    drawSetting("Resolution", TextFormat("%dx%d", settings_.graphics().resolution_w,
                                          settings_.graphics().resolution_h));
    drawSetting("Render Scale", TextFormat("%.0f%%", settings_.graphics().render_scale * 100));
    drawSetting("Shadows", settings_.graphics().shadows == ShadowQuality::OFF ? "Off" :
                           settings_.graphics().shadows == ShadowQuality::BLOB ? "Blob" :
                           settings_.graphics().shadows == ShadowQuality::SIMPLE_DYNAMIC ? "Simple" : "Soft");
    drawSetting("FXAA", settings_.graphics().fxaa ? "On" : "Off");
    drawSetting("VSync", settings_.graphics().vsync ? "On" : "Off");
    drawSetting("Master Volume", TextFormat("%.0f%%", settings_.audio().master_volume * 100));
    drawSetting("Steering Sensitivity", TextFormat("%.1f", settings_.controls().steering_sensitivity));
}

void Engine::renderBenchmarkUI() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    if (benchmark_.isRunning()) {
        DrawText("BENCHMARKING...", w / 2 - MeasureText("BENCHMARKING...", 32) / 2,
                 h / 3, 32, WHITE);

        float prog = benchmark_.progress();
        DrawRectangle(w / 4, h / 2, (int)(w / 2 * prog), 20, {0, 180, 255, 255});
        DrawRectangleLines(w / 4, h / 2, w / 2, 20, WHITE);

        DrawText(TextFormat("%.0f%%", prog * 100), w / 2 - 20, h / 2 + 30, 20, WHITE);
    } else {
        const auto& r = benchmark_.results();

        DrawText("BENCHMARK RESULTS", w / 2 - MeasureText("BENCHMARK RESULTS", 36) / 2,
                 60, 36, WHITE);

        Color badge_color = r.badge == BenchmarkBadge::TOURNAMENT_READY ? Color{0, 220, 80, 255} :
                            r.badge == BenchmarkBadge::PLAYABLE ? Color{255, 200, 0, 255} :
                            Color{255, 60, 60, 255};

        int y = 150;
        DrawText(TextFormat("Avg FPS:     %.1f", r.avg_fps), w / 3, y, 24, WHITE); y += 40;
        DrawText(TextFormat("1%% Low FPS:  %.1f", r.low_1pct_fps), w / 3, y, 24, WHITE); y += 40;
        DrawText(TextFormat("Peak RAM:    %.1f MB", r.peak_ram_mb), w / 3, y, 24, WHITE); y += 60;

        DrawText(r.badge_text.c_str(), w / 2 - MeasureText(r.badge_text.c_str(), 28) / 2,
                 y, 28, badge_color);

        y += 60;
        Rectangle btn = {(float)(w / 2 - 100), (float)y, 200, 45};
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        DrawRectangleRounded(btn, 0.3f, 8, hover ? Color{50, 120, 200, 220} : Color{40, 40, 55, 180});
        DrawText("BACK", w / 2 - MeasureText("BACK", 20) / 2, y + 12, 20, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state_ = GameState::MAIN_MENU;
        }
    }
}

void Engine::renderGarage() {
    int w = GetScreenWidth();
    DrawText("GARAGE", w / 2 - MeasureText("GARAGE", 36) / 2, 40, 36, WHITE);
    DrawText("Car selection coming soon...", w / 2 - 130, 120, 18, {120, 120, 140, 200});
}

void Engine::renderMultiplayerLobby() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawText("MULTIPLAYER", w / 2 - MeasureText("MULTIPLAYER", 36) / 2, 40, 36, WHITE);

    // Connection modes
    struct ModeItem { const char* label; const char* desc; };
    ModeItem modes[] = {
        {"CASUAL CHANNEL", "Enter a 3-6 digit code to find opponents"},
        {"PRIVATE SERVER", "Create or join a private room with password"},
        {"DIRECT CONNECT", "Connect to a tournament server via IP:Port"},
    };

    int y = 140;
    for (int i = 0; i < 3; ++i) {
        Rectangle btn = {(float)(w / 2 - 200), (float)(y + i * 90), 400, 70};
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        DrawRectangleRounded(btn, 0.2f, 8,
                             hover ? Color{50, 100, 200, 200} : Color{35, 35, 50, 180});
        DrawText(modes[i].label, w / 2 - MeasureText(modes[i].label, 22) / 2,
                 y + i * 90 + 12, 22, WHITE);
        DrawText(modes[i].desc, w / 2 - MeasureText(modes[i].desc, 14) / 2,
                 y + i * 90 + 42, 14, {140, 140, 160, 200});
    }

    // Back button
    if (IsKeyPressed(KEY_ESCAPE)) state_ = GameState::MAIN_MENU;
}

void Engine::renderRacing() {
    // 3D scene
    BeginMode3D(camera_.camera());

    // Environment
    env_.renderGround();
    env_.render();

    // Road
    road_.render();

    // NPC traffic
    for (int i = 0; i < traffic_.activeCount(); ++i) {
        if (traffic_.npcs()[i].active) {
            veh_renderer_.renderVehicle(traffic_.npcs()[i].vehicle);
        }
    }

    // Player vehicle
    veh_renderer_.renderVehicle(player_vehicle_);

    // Ghost vehicles (multiplayer)
    for (const auto& ghost : ghosts) {
        if (ghost.active) {
            veh_renderer_.renderGhost(
                ghost.position, ghost.rotation, ghost.steer_angle,
                ghost.gear, ghost.left_indicator, ghost.right_indicator,
                ghost.hazards, CarModel::SEDAN, 0.5f);
        }
    }

    EndMode3D();

    // HUD overlay
    hud_.render(*this);

    // Android touch UI
    #if defined(DRT_PLATFORM_ANDROID)
    android_ui_.render();
    #endif
}

void Engine::renderRaceResults() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawText("RACE COMPLETE", w / 2 - MeasureText("RACE COMPLETE", 36) / 2,
             h / 3, 36, {0, 220, 120, 255});
    DrawText("Press ENTER to continue",
             w / 2 - MeasureText("Press ENTER to continue", 18) / 2,
             h / 2, 18, {150, 150, 170, 200});
}

} // namespace drt
