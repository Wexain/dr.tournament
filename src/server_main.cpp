// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Headless Dedicated Server Entry Point
// ═══════════════════════════════════════════════════════════════════════════
#include "net/enet_server.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <chrono>
#include <thread>

static volatile bool g_running = true;

void signalHandler(int sig) {
    (void)sig;
    g_running = false;
}

int main(int argc, char** argv) {
    printf("═══════════════════════════════════════════════════\n");
    printf("  Dr. Tournaments — Dedicated Server v0.1.0\n");
    printf("═══════════════════════════════════════════════════\n\n");

    // Parse arguments
    uint16_t port = 7777;
    int max_rooms = 16;
    bool test_mode = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = static_cast<uint16_t>(atoi(argv[++i]));
        }
        if (strcmp(argv[i], "--rooms") == 0 && i + 1 < argc) {
            max_rooms = atoi(argv[++i]);
        }
        if (strcmp(argv[i], "--test") == 0) {
            test_mode = true;
        }
    }

    // Test mode: validate systems and exit
    if (test_mode) {
        printf("[TEST] Initializing server...\n");
        drt::ENetServer server;
        server.init(port, max_rooms);
        
        if (server.isRunning()) {
            printf("[TEST] Server initialized successfully on port %d\n", port);
            printf("[TEST] Creating test room...\n");
            
            drt::RoomConfig cfg;
            cfg.room_name = "Test Room";
            cfg.max_players = 8;
            cfg.lane_count = 3;
            int room_id = server.createRoom(cfg);
            
            if (room_id >= 0) {
                printf("[TEST] Room created (id=%d)\n", room_id);
            } else {
                printf("[TEST] FAIL: Room creation failed\n");
                return 1;
            }
            
            printf("[TEST] All tests passed!\n");
            server.shutdown();
            return 0;
        } else {
            printf("[TEST] FAIL: Server failed to start\n");
            return 1;
        }
    }

    // Production mode
    signal(SIGINT, signalHandler);
    #ifndef _WIN32
    signal(SIGTERM, signalHandler);
    #endif

    drt::ENetServer server;
    server.init(port, max_rooms);

    if (!server.isRunning()) {
        printf("[ERROR] Server failed to start\n");
        return 1;
    }

    printf("[SERVER] Ready. Press Ctrl+C to stop.\n\n");

    // Main server loop — fixed 60 Hz tick rate
    auto prev_time = std::chrono::steady_clock::now();
    constexpr auto tick_duration = std::chrono::microseconds(16667); // 1/60 s

    while (g_running && server.isRunning()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - prev_time);
        prev_time = now;

        server.update(elapsed.count());

        // Sleep to maintain 60 Hz tick rate
        auto frame_end = std::chrono::steady_clock::now();
        auto frame_duration = frame_end - now;
        if (frame_duration < tick_duration) {
            std::this_thread::sleep_for(tick_duration - frame_duration);
        }
    }

    printf("\n[SERVER] Shutting down...\n");
    server.shutdown();
    printf("[SERVER] Goodbye!\n");

    return 0;
}
