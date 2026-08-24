// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — ENet Dedicated Server Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "net/enet_server.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <cstdio>
#include <random>
#include <algorithm>

using json = nlohmann::json;

namespace drt {

void ENetServer::init(uint16_t port, int max_rooms) {
    if (enet_initialize() != 0) {
        printf("[SERVER] Failed to initialize ENet\n");
        return;
    }

    port_ = port;
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    host_ = enet_host_create(&address, 64, static_cast<int>(NetChannel::COUNT), 0, 0);
    if (!host_) {
        printf("[SERVER] Failed to create server on port %d\n", port);
        return;
    }

    running_ = true;
    active_rooms_ = 0;
    printf("[SERVER] Dr. Tournaments server running on port %d\n", port);
}

void ENetServer::update(float dt) {
    if (!host_ || !running_) return;

    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                handleConnect(event);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                handleDisconnect(event);
                break;
            default:
                break;
        }
    }

    // Update room lifecycles
    updateRoomLifecycles(dt);
}

void ENetServer::shutdown() {
    if (host_) {
        // Disconnect all peers
        for (size_t i = 0; i < host_->peerCount; ++i) {
            if (host_->peers[i].state == ENET_PEER_STATE_CONNECTED) {
                enet_peer_disconnect(&host_->peers[i], 0);
            }
        }
        enet_host_flush(host_);
        enet_host_destroy(host_);
        host_ = nullptr;
    }
    enet_deinitialize();
    running_ = false;
    printf("[SERVER] Shutdown complete\n");
}

void ENetServer::handleConnect(ENetEvent& event) {
    printf("[SERVER] Peer connected from %x:%u\n",
           event.peer->address.host, event.peer->address.port);
}

void ENetServer::handleDisconnect(ENetEvent& event) {
    printf("[SERVER] Peer disconnected\n");

    int room_id = findPlayerRoom(event.peer);
    if (room_id >= 0) {
        auto& room = rooms_[room_id];
        for (auto& player : room.players) {
            if (player.peer == event.peer) {
                printf("[SERVER] Player '%s' left room '%s'\n",
                       player.nickname.c_str(), room.name.c_str());
                player.peer = nullptr;
                player.nickname.clear();
                room.player_count--;
                break;
            }
        }
        peer_rooms_.erase(event.peer);
    }
}

void ENetServer::handlePacket(ENetEvent& event) {
    if (event.packet->dataLength == 0) return;

    PacketType type = static_cast<PacketType>(event.packet->data[0]);

    switch (type) {
        case PacketType::JOIN_REQUEST:
            processJoinRequest(event.peer, event.packet->data, event.packet->dataLength);
            break;
        case PacketType::ROOM_CREATE:
            processRoomCreate(event.peer, event.packet->data, event.packet->dataLength);
            break;
        case PacketType::STATE_UPDATE:
            processStateUpdate(event.peer, event.packet->data, event.packet->dataLength);
            break;
        case PacketType::INPUT_LOG:
            processInputLog(event.peer, event.packet->data, event.packet->dataLength);
            break;
        default:
            break;
    }
}

void ENetServer::processJoinRequest(ENetPeer* peer, const uint8_t* data, size_t len) {
    try {
        std::string json_str(reinterpret_cast<const char*>(data), len);
        auto req = JoinRequest::fromJSON(json_str);

        // Find room by channel code or room name
        int room_id = -1;
        for (int i = 0; i < (int)rooms_.size(); ++i) {
            if (rooms_[i].player_count > 0 && rooms_[i].name == req.channel_code) {
                room_id = i;
                break;
            }
        }

        if (room_id < 0) {
            // Create new room with defaults
            room_id = createRoom(RoomConfig{});
            if (room_id < 0) {
                uint8_t reject[] = {static_cast<uint8_t>(PacketType::JOIN_REJECT)};
                sendToPeer(peer, reject, 1, true);
                return;
            }
        }

        auto& room = rooms_[room_id];
        if (room.player_count >= room.config.max_players) {
            uint8_t reject[] = {static_cast<uint8_t>(PacketType::JOIN_REJECT)};
            sendToPeer(peer, reject, 1, true);
            return;
        }

        // Find free player slot
        for (int i = 0; i < 16; ++i) {
            if (room.players[i].peer == nullptr) {
                room.players[i].peer = peer;
                room.players[i].nickname = req.nickname;
                room.players[i].uuid = req.uuid;
                room.players[i].player_id = static_cast<uint8_t>(i);
                room.players[i].car_model = req.car_model;
                room.player_count++;
                peer_rooms_[peer] = room_id;

                // Send accept
                uint8_t accept[] = {
                    static_cast<uint8_t>(PacketType::JOIN_ACCEPT),
                    static_cast<uint8_t>(i)
                };
                sendToPeer(peer, accept, 2, true);

                // Send traffic seed
                uint8_t seed_pkt[5];
                seed_pkt[0] = static_cast<uint8_t>(PacketType::SEED_SYNC);
                memcpy(seed_pkt + 1, &room.traffic_seed, 4);
                sendToPeer(peer, seed_pkt, 5, true);

                printf("[SERVER] Player '%s' joined room '%s' as #%d\n",
                       req.nickname.c_str(), room.name.c_str(), i);
                break;
            }
        }
    } catch (...) {
        printf("[SERVER] Invalid join request\n");
    }
}

void ENetServer::processRoomCreate(ENetPeer* peer, const uint8_t* data, size_t len) {
    try {
        std::string json_str(reinterpret_cast<const char*>(data), len);
        auto j = json::parse(json_str);
        auto config = RoomConfig::fromJSON(j.value("config", "{}"));
        int room_id = createRoom(config);
        if (room_id >= 0) {
            printf("[SERVER] Room '%s' created (id=%d)\n", config.room_name.c_str(), room_id);
        }
    } catch (...) {}
}

void ENetServer::processStateUpdate(ENetPeer* peer, const uint8_t* data, size_t len) {
    if (len < sizeof(StatePacket)) return;

    StatePacket pkt;
    memcpy(&pkt, data, sizeof(StatePacket));

    int room_id = findPlayerRoom(peer);
    if (room_id >= 0) {
        broadcastState(room_id, pkt, peer);
    }
}

void ENetServer::processInputLog(ENetPeer* peer, const uint8_t* data, size_t len) {
    if (len < 5) return;

    PlayerSession* player = findPlayer(peer);
    if (!player) return;

    uint32_t frame_count;
    memcpy(&frame_count, data + 1, sizeof(uint32_t));

    size_t expected_size = 5 + frame_count * sizeof(CompactInputFrame);
    if (len < expected_size) return;

    player->input_log.resize(frame_count);
    memcpy(player->input_log.data(), data + 5, frame_count * sizeof(CompactInputFrame));

    printf("[SERVER] Received input log from '%s': %u frames (%.1f KB)\n",
           player->nickname.c_str(), frame_count,
           frame_count * sizeof(CompactInputFrame) / 1024.0f);
}

int ENetServer::createRoom(const RoomConfig& config) {
    for (int i = 0; i < (int)rooms_.size(); ++i) {
        if (rooms_[i].player_count == 0 && rooms_[i].name.empty()) {
            rooms_[i].name = config.room_name.empty() ?
                "Room " + std::to_string(i) : config.room_name;
            rooms_[i].config = config;
            rooms_[i].empty_timer = 0.0f;

            // Generate deterministic traffic seed
            std::random_device rd;
            rooms_[i].traffic_seed = rd();

            active_rooms_++;
            return i;
        }
    }
    return -1;  // No free rooms
}

void ENetServer::closeRoom(int room_id) {
    if (room_id < 0 || room_id >= (int)rooms_.size()) return;
    auto& room = rooms_[room_id];

    // Kick all players
    for (auto& player : room.players) {
        if (player.peer) {
            enet_peer_disconnect(player.peer, 0);
            peer_rooms_.erase(player.peer);
            player.peer = nullptr;
        }
        player.nickname.clear();
    }

    room.name.clear();
    room.player_count = 0;
    active_rooms_--;
    printf("[SERVER] Room %d closed\n", room_id);
}

void ENetServer::broadcastState(int room_id, const StatePacket& packet, ENetPeer* exclude) {
    auto& room = rooms_[room_id];
    for (auto& player : room.players) {
        if (player.peer && player.peer != exclude) {
            sendToPeer(player.peer, &packet, sizeof(StatePacket), false);
        }
    }
}

void ENetServer::broadcastChat(int room_id, const std::string& from, const std::string& msg) {
    json j;
    j["type"] = static_cast<int>(PacketType::CHAT_MESSAGE);
    j["from"] = from;
    j["message"] = msg;
    std::string data = j.dump();

    auto& room = rooms_[room_id];
    for (auto& player : room.players) {
        if (player.peer) {
            sendToPeer(player.peer, data.c_str(), data.size(), true);
        }
    }
}

void ENetServer::kickPlayer(uint8_t room_id, uint8_t player_id, const std::string& reason) {
    if (room_id >= rooms_.size()) return;
    auto& room = rooms_[room_id];
    if (player_id >= 16) return;
    auto& player = room.players[player_id];
    if (player.peer) {
        printf("[SERVER] Kicking '%s': %s\n", player.nickname.c_str(), reason.c_str());
        enet_peer_disconnect(player.peer, 0);
        peer_rooms_.erase(player.peer);
        player.peer = nullptr;
        player.nickname.clear();
        room.player_count--;
    }
}

void ENetServer::updateRoomLifecycles(float dt) {
    for (int i = 0; i < (int)rooms_.size(); ++i) {
        auto& room = rooms_[i];
        if (room.name.empty()) continue;

        if (room.player_count == 0) {
            room.empty_timer += dt;
            if (room.empty_timer >= ServerRoom::GRACE_TIMEOUT) {
                printf("[SERVER] Room '%s' auto-closing (30s grace expired)\n",
                       room.name.c_str());
                closeRoom(i);
            }
        } else {
            room.empty_timer = 0.0f;
        }

        // Update race time
        if (room.race_active) {
            room.race_time += dt;
        }
    }
}

bool ENetServer::validateInputLog(const PlayerSession& session, float claimed_time) {
    // Anti-cheat: replay the input log and verify the result is consistent
    // For now: basic frame count validation
    float expected_frames = claimed_time * 60.0f;  // 60 Hz
    float actual_frames = static_cast<float>(session.input_log.size());

    // Allow 5% tolerance
    float ratio = actual_frames / expected_frames;
    if (ratio < 0.95f || ratio > 1.05f) {
        printf("[ANTICHEAT] Frame count mismatch for '%s': expected ~%.0f, got %.0f\n",
               session.nickname.c_str(), expected_frames, actual_frames);
        return false;
    }

    // Check for impossible inputs (all max gas, no steering variation)
    int max_gas_frames = 0;
    for (const auto& frame : session.input_log) {
        if (frame.getGas() >= 0.99f) max_gas_frames++;
    }
    float max_gas_ratio = (float)max_gas_frames / actual_frames;
    if (max_gas_ratio > 0.99f) {
        printf("[ANTICHEAT] Suspicious: '%s' had %.0f%% full gas\n",
               session.nickname.c_str(), max_gas_ratio * 100);
        // Not necessarily cheating, but flag for review
    }

    return true;
}

int ENetServer::totalPlayers() const {
    int total = 0;
    for (const auto& room : rooms_) total += room.player_count;
    return total;
}

PlayerSession* ENetServer::findPlayer(ENetPeer* peer) {
    int room_id = findPlayerRoom(peer);
    if (room_id < 0) return nullptr;
    for (auto& player : rooms_[room_id].players) {
        if (player.peer == peer) return &player;
    }
    return nullptr;
}

int ENetServer::findPlayerRoom(ENetPeer* peer) {
    auto it = peer_rooms_.find(peer);
    return (it != peer_rooms_.end()) ? it->second : -1;
}

void ENetServer::sendToPeer(ENetPeer* peer, const void* data, size_t len, bool reliable) {
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;
    uint8_t channel = reliable ? static_cast<uint8_t>(NetChannel::RELIABLE)
                               : static_cast<uint8_t>(NetChannel::STATE);
    ENetPacket* packet = enet_packet_create(data, len, flags);
    enet_peer_send(peer, channel, packet);
}

} // namespace drt
