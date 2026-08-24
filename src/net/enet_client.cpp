// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — ENet Client Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "net/enet_client.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <cstdio>

using json = nlohmann::json;

namespace drt {

void ENetClient::init() {
    if (enet_initialize() != 0) {
        TraceLog(LOG_ERROR, "[NET] Failed to initialize ENet");
        return;
    }

    host_ = enet_host_create(nullptr, 1, static_cast<int>(NetChannel::COUNT), 0, 0);
    if (!host_) {
        TraceLog(LOG_ERROR, "[NET] Failed to create ENet client host");
        return;
    }

    initialized_ = true;
    state_ = ConnectionState::DISCONNECTED;
    TraceLog(LOG_INFO, "[NET] ENet client initialized");
}

void ENetClient::shutdown() {
    disconnect();
    if (host_) {
        enet_host_destroy(host_);
        host_ = nullptr;
    }
    if (initialized_) {
        enet_deinitialize();
        initialized_ = false;
    }
}

void ENetClient::update(float dt) {
    if (!host_ || !initialized_) return;

    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                TraceLog(LOG_INFO, "[NET] Connected to server");
                state_ = ConnectionState::CONNECTED;
                ping_ms_ = event.peer->roundTripTime;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event.packet->data, event.packet->dataLength,
                             event.channelID);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                TraceLog(LOG_INFO, "[NET] Disconnected from server");
                state_ = ConnectionState::DISCONNECTED;
                peer_ = nullptr;
                break;

            default:
                break;
        }
    }

    // Update ping
    if (peer_) {
        ping_ms_ = peer_->roundTripTime;
    }

    // State sync rate limiting
    state_send_timer_ += dt;
}

bool ENetClient::connectCasual(const std::string& relay_host, uint16_t port,
                                const std::string& channel_code) {
    if (!host_) return false;

    ENetAddress address;
    enet_address_set_host(&address, relay_host.c_str());
    address.port = port;

    peer_ = enet_host_connect(host_, &address, static_cast<int>(NetChannel::COUNT), 0);
    if (!peer_) {
        TraceLog(LOG_ERROR, "[NET] Failed to connect to relay");
        return false;
    }

    mode_ = ConnectionMode::CASUAL_CHANNEL;
    state_ = ConnectionState::CONNECTING;
    return true;
}

bool ENetClient::connectPrivateServer(const std::string& host, uint16_t port,
                                       const std::string& room_name,
                                       const std::string& password) {
    if (!host_) return false;

    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    peer_ = enet_host_connect(host_, &address, static_cast<int>(NetChannel::COUNT), 0);
    if (!peer_) return false;

    mode_ = ConnectionMode::PRIVATE_SERVER;
    state_ = ConnectionState::CONNECTING;
    return true;
}

bool ENetClient::connectDirect(const std::string& host, uint16_t port) {
    if (!host_) return false;

    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    peer_ = enet_host_connect(host_, &address, static_cast<int>(NetChannel::COUNT), 0);
    if (!peer_) return false;

    mode_ = ConnectionMode::DIRECT_TOURNAMENT;
    state_ = ConnectionState::CONNECTING;
    return true;
}

void ENetClient::disconnect() {
    if (peer_) {
        enet_peer_disconnect(peer_, 0);
        // Wait briefly for graceful disconnect
        ENetEvent event;
        while (enet_host_service(host_, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
        }
        enet_peer_reset(peer_);
        peer_ = nullptr;
    }
    state_ = ConnectionState::DISCONNECTED;
}

void ENetClient::sendStateUpdate(const StatePacket& packet) {
    if (!peer_ || state_ < ConnectionState::CONNECTED) return;

    // Rate limit to 30Hz
    if (state_send_timer_ < STATE_SEND_INTERVAL) return;
    state_send_timer_ = 0.0f;

    sendUnreliable(&packet, sizeof(StatePacket));
}

void ENetClient::sendChat(const std::string& message) {
    if (!peer_) return;

    json j;
    j["type"] = static_cast<int>(PacketType::CHAT_MESSAGE);
    j["message"] = message;
    std::string data = j.dump();
    sendReliable(data.c_str(), data.size());
}

void ENetClient::sendInputLog(const std::vector<CompactInputFrame>& frames) {
    if (!peer_ || frames.empty()) return;

    // Header: type byte + frame count
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(PacketType::INPUT_LOG));
    uint32_t count = static_cast<uint32_t>(frames.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&count),
                reinterpret_cast<uint8_t*>(&count) + sizeof(count));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(frames.data()),
                reinterpret_cast<const uint8_t*>(frames.data()) +
                frames.size() * sizeof(CompactInputFrame));

    sendReliable(data.data(), data.size());
}

void ENetClient::createRoom(const RoomConfig& config) {
    if (!peer_) return;
    json j;
    j["type"] = static_cast<int>(PacketType::ROOM_CREATE);
    j["config"] = config.toJSON();
    std::string data = j.dump();
    sendReliable(data.c_str(), data.size());
}

void ENetClient::joinRoom(const JoinRequest& req) {
    if (!peer_) return;
    std::string data = req.toJSON();
    sendReliable(data.c_str(), data.size());
}

void ENetClient::handlePacket(const uint8_t* data, size_t length, uint8_t channel) {
    if (length == 0) return;

    PacketType type = static_cast<PacketType>(data[0]);

    switch (type) {
        case PacketType::STATE_UPDATE: {
            if (length >= sizeof(StatePacket) && on_state_) {
                StatePacket pkt;
                memcpy(&pkt, data, sizeof(StatePacket));
                on_state_(pkt);
            }
            break;
        }
        case PacketType::SEED_SYNC: {
            if (length >= 5) {
                memcpy(&traffic_seed_, data + 1, sizeof(uint32_t));
                TraceLog(LOG_INFO, "[NET] Traffic seed: %u", traffic_seed_);
            }
            break;
        }
        case PacketType::JOIN_ACCEPT: {
            if (length >= 2) {
                local_player_id_ = data[1];
                state_ = ConnectionState::IN_ROOM;
                TraceLog(LOG_INFO, "[NET] Joined as player %d", local_player_id_);
            }
            break;
        }
        case PacketType::CHAT_MESSAGE: {
            if (on_chat_ && channel == static_cast<uint8_t>(NetChannel::RELIABLE)) {
                try {
                    std::string json_str(reinterpret_cast<const char*>(data), length);
                    auto j = json::parse(json_str);
                    on_chat_(j.value("from", ""), j.value("message", ""));
                } catch (...) {}
            }
            break;
        }
        default: {
            if (on_event_) {
                on_event_(type, data, length);
            }
            break;
        }
    }
}

void ENetClient::sendReliable(const void* data, size_t length) {
    if (!peer_) return;
    ENetPacket* packet = enet_packet_create(data, length, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, static_cast<uint8_t>(NetChannel::RELIABLE), packet);
}

void ENetClient::sendUnreliable(const void* data, size_t length) {
    if (!peer_) return;
    ENetPacket* packet = enet_packet_create(data, length, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer_, static_cast<uint8_t>(NetChannel::STATE), packet);
}

// ── Protocol JSON helpers ──────────────────────────────────────────────────
std::string RoomConfig::toJSON() const {
    json j;
    j["room_name"] = room_name;
    j["password"] = password;
    j["max_players"] = max_players;
    j["lane_count"] = lane_count;
    j["spec_lock"] = spec_lock;
    j["transmission_lock"] = transmission_lock;
    j["zero_tolerance_dnf"] = zero_tolerance_dnf;
    j["traffic_density"] = traffic_density;
    j["surface_friction"] = surface_friction;
    j["forced_car_model"] = forced_car_model;
    return j.dump();
}

RoomConfig RoomConfig::fromJSON(const std::string& json_str) {
    RoomConfig cfg;
    try {
        auto j = json::parse(json_str);
        cfg.room_name = j.value("room_name", "");
        cfg.password = j.value("password", "");
        cfg.max_players = j.value("max_players", 8);
        cfg.lane_count = j.value("lane_count", 3);
        cfg.spec_lock = j.value("spec_lock", false);
        cfg.transmission_lock = j.value("transmission_lock", false);
        cfg.zero_tolerance_dnf = j.value("zero_tolerance_dnf", false);
        cfg.traffic_density = j.value("traffic_density", 0.5f);
        cfg.surface_friction = j.value("surface_friction", 1.0f);
        cfg.forced_car_model = j.value("forced_car_model", 0xFF);
    } catch (...) {}
    return cfg;
}

std::string JoinRequest::toJSON() const {
    json j;
    j["type"] = static_cast<int>(PacketType::JOIN_REQUEST);
    j["nickname"] = nickname;
    j["uuid"] = uuid;
    j["room_password"] = room_password;
    j["channel_code"] = channel_code;
    j["car_model"] = car_model;
    return j.dump();
}

JoinRequest JoinRequest::fromJSON(const std::string& json_str) {
    JoinRequest req;
    try {
        auto j = json::parse(json_str);
        req.nickname = j.value("nickname", "");
        req.uuid = j.value("uuid", "");
        req.room_password = j.value("room_password", "");
        req.channel_code = j.value("channel_code", "");
        req.car_model = j.value("car_model", 0);
    } catch (...) {}
    return req;
}

} // namespace drt
