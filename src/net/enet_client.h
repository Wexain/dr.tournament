#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — ENet Client
// ═══════════════════════════════════════════════════════════════════════════
#include "net/protocol.h"
#include <enet/enet.h>
#include <string>
#include <functional>
#include <vector>
#include <array>
#include <cstdint>

namespace drt {

enum class ConnectionState : uint8_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    IN_ROOM,
    RACING
};

enum class ConnectionMode : uint8_t {
    CASUAL_CHANNEL,    // Mode A
    PRIVATE_SERVER,    // Mode B
    DIRECT_TOURNAMENT  // Mode C
};

class ENetClient {
public:
    using StateCallback = std::function<void(const StatePacket&)>;
    using ChatCallback  = std::function<void(const std::string& from, const std::string& msg)>;
    using EventCallback = std::function<void(PacketType type, const uint8_t* data, size_t len)>;

    void init();
    void shutdown();
    void update(float dt);

    // Connection
    bool connectCasual(const std::string& relay_host, uint16_t port,
                       const std::string& channel_code);
    bool connectPrivateServer(const std::string& host, uint16_t port,
                              const std::string& room_name, const std::string& password);
    bool connectDirect(const std::string& host, uint16_t port);
    void disconnect();

    // State
    [[nodiscard]] ConnectionState state() const { return state_; }
    [[nodiscard]] ConnectionMode  mode()  const { return mode_; }
    [[nodiscard]] bool isConnected() const { return state_ >= ConnectionState::CONNECTED; }
    [[nodiscard]] int  pingMs() const { return ping_ms_; }
    [[nodiscard]] uint8_t localPlayerId() const { return local_player_id_; }

    // Send game state (unreliable, 30Hz)
    void sendStateUpdate(const StatePacket& packet);

    // Send chat message (reliable)
    void sendChat(const std::string& message);

    // Send input log for anti-cheat (reliable)
    void sendInputLog(const std::vector<CompactInputFrame>& frames);

    // Room operations
    void createRoom(const RoomConfig& config);
    void joinRoom(const JoinRequest& req);

    // Callbacks
    void setStateCallback(StateCallback cb)  { on_state_ = std::move(cb); }
    void setChatCallback(ChatCallback cb)    { on_chat_ = std::move(cb); }
    void setEventCallback(EventCallback cb)  { on_event_ = std::move(cb); }

    // Seed sync for deterministic traffic
    [[nodiscard]] uint32_t trafficSeed() const { return traffic_seed_; }

private:
    void handlePacket(const uint8_t* data, size_t length, uint8_t channel);
    void sendReliable(const void* data, size_t length);
    void sendUnreliable(const void* data, size_t length);

    ENetHost*   host_   = nullptr;
    ENetPeer*   peer_   = nullptr;
    
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    ConnectionMode  mode_  = ConnectionMode::CASUAL_CHANNEL;
    
    uint8_t  local_player_id_ = 0;
    int      ping_ms_ = 0;
    uint32_t traffic_seed_ = 0;
    
    // State sync rate limiting
    float state_send_timer_  = 0.0f;
    static constexpr float STATE_SEND_INTERVAL = 1.0f / 30.0f;  // 30Hz
    
    // Callbacks
    StateCallback on_state_;
    ChatCallback  on_chat_;
    EventCallback on_event_;

    bool initialized_ = false;
};

} // namespace drt
