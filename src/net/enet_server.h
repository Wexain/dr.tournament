#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — ENet Dedicated Server
// ═══════════════════════════════════════════════════════════════════════════
#include "net/protocol.h"
#include <enet/enet.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>

namespace drt {

// ── Player Session ─────────────────────────────────────────────────────────
struct PlayerSession {
    ENetPeer*   peer         = nullptr;
    std::string nickname;
    std::string uuid;
    uint8_t     player_id    = 0;
    uint8_t     car_model    = 0;
    bool        is_mod       = false;
    bool        is_spectator = false;
    bool        finished     = false;
    float       finish_time  = 0.0f;
    
    // Anti-cheat
    std::vector<CompactInputFrame> input_log;
    bool validated = false;
};

// ── Room ───────────────────────────────────────────────────────────────────
struct ServerRoom {
    std::string  name;
    std::string  password_hash;
    RoomConfig   config;
    uint32_t     traffic_seed  = 0;
    
    std::array<PlayerSession, 16> players;
    int          player_count  = 0;
    
    // Auto-teardown
    float        empty_timer   = 0.0f;
    bool         race_active   = false;
    float        race_time     = 0.0f;
    
    static constexpr float GRACE_TIMEOUT = 30.0f;
};

// ── Dedicated Server ───────────────────────────────────────────────────────
class ENetServer {
public:
    void init(uint16_t port = 7777, int max_rooms = 16);
    void update(float dt);
    void shutdown();

    [[nodiscard]] bool isRunning() const { return running_; }
    [[nodiscard]] int  totalPlayers() const;
    [[nodiscard]] int  roomCount() const { return active_rooms_; }

    // Room management
    int  createRoom(const RoomConfig& config);
    void closeRoom(int room_id);

    // Moderation
    void kickPlayer(uint8_t room_id, uint8_t player_id, const std::string& reason);
    void broadcastChat(int room_id, const std::string& from, const std::string& msg);

    // Anti-cheat validation
    bool validateInputLog(const PlayerSession& session, float claimed_time);

private:
    void handleConnect(ENetEvent& event);
    void handleDisconnect(ENetEvent& event);
    void handlePacket(ENetEvent& event);
    
    void processJoinRequest(ENetPeer* peer, const uint8_t* data, size_t len);
    void processRoomCreate(ENetPeer* peer, const uint8_t* data, size_t len);
    void processStateUpdate(ENetPeer* peer, const uint8_t* data, size_t len);
    void processInputLog(ENetPeer* peer, const uint8_t* data, size_t len);
    
    void broadcastState(int room_id, const StatePacket& packet, ENetPeer* exclude);
    void sendToPeer(ENetPeer* peer, const void* data, size_t len, bool reliable);
    
    void updateRoomLifecycles(float dt);
    PlayerSession* findPlayer(ENetPeer* peer);
    int findPlayerRoom(ENetPeer* peer);

    ENetHost*   host_ = nullptr;
    bool        running_ = false;
    uint16_t    port_ = 7777;
    
    std::array<ServerRoom, 16> rooms_;
    int active_rooms_ = 0;
    
    // Peer → room mapping
    std::unordered_map<ENetPeer*, int> peer_rooms_;
};

} // namespace drt
