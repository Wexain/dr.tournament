#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Network Protocol Definitions
// ═══════════════════════════════════════════════════════════════════════════
#include <raylib.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <array>

namespace drt {

// ── Packet Types ───────────────────────────────────────────────────────────
enum class PacketType : uint8_t {
    // Connection
    JOIN_REQUEST    = 0x01,
    JOIN_ACCEPT     = 0x02,
    JOIN_REJECT     = 0x03,
    LEAVE           = 0x04,
    HEARTBEAT       = 0x05,
    
    // Room management
    ROOM_CREATE     = 0x10,
    ROOM_JOIN       = 0x11,
    ROOM_CONFIG     = 0x12,
    ROOM_LIST       = 0x13,
    ROOM_CLOSE      = 0x14,
    
    // Game state (unreliable, 30Hz)
    STATE_UPDATE    = 0x20,
    SEED_SYNC       = 0x21,
    RACE_START      = 0x22,
    RACE_FINISH     = 0x23,
    
    // Anti-cheat
    INPUT_LOG       = 0x30,
    VALIDATE_RESULT = 0x31,
    
    // Social
    CHAT_MESSAGE    = 0x40,
    FRIEND_REQUEST  = 0x41,
    INVITE          = 0x42,
    
    // Voice signaling
    VOICE_OFFER     = 0x50,
    VOICE_ANSWER    = 0x51,
    VOICE_ICE       = 0x52
};

// ── ENet Channels ──────────────────────────────────────────────────────────
enum class NetChannel : uint8_t {
    RELIABLE   = 0,  // Room management, chat, auth
    STATE      = 1,  // Unreliable game state
    VOICE_SIG  = 2,  // Voice signaling (reliable)
    COUNT      = 3
};

// ── Compact Input Frame (3 bytes — anti-cheat logging) ─────────────────────
#pragma pack(push, 1)
struct CompactInputFrame {
    int8_t  steering;    // -128..127 → -1.0..1.0
    uint8_t gas_brake;   // hi nibble = gas(0-15), lo nibble = brake(0-15)
    uint8_t gear_flags;  // bits 0-1 = gear (P/R/N/D), bit 2 = handbrake,
                         // bit 3 = left_ind, bit 4 = right_ind, bit 5 = hazards

    [[nodiscard]] float getGas() const { return (gas_brake >> 4) / 15.0f; }
    [[nodiscard]] float getBrake() const { return (gas_brake & 0x0F) / 15.0f; }
    [[nodiscard]] float getSteering() const { return steering / 127.0f; }
    [[nodiscard]] uint8_t getGear() const { return gear_flags & 0x03; }
    [[nodiscard]] bool getHandbrake() const { return (gear_flags >> 2) & 1; }
};
#pragma pack(pop)

// ── State Update Packet (sent at 30Hz per player) ──────────────────────────
#pragma pack(push, 1)
struct StatePacket {
    PacketType type = PacketType::STATE_UPDATE;
    uint8_t    player_id;
    float      pos_x, pos_y, pos_z;
    float      rot_x, rot_y, rot_z, rot_w;  // Quaternion
    float      vel_x, vel_y, vel_z;
    float      steer_angle;
    uint8_t    gear;
    uint8_t    flags;  // bit 0 = left_ind, bit 1 = right_ind, bit 2 = hazards,
                       // bit 3 = reverse_light, bit 4 = horn
    uint16_t   speed_cmps;  // Speed in cm/s (uint16 enough for driving game)
    
    static constexpr size_t SIZE = sizeof(StatePacket);
};
#pragma pack(pop)

// ── Room Configuration ─────────────────────────────────────────────────────
struct RoomConfig {
    std::string room_name;
    std::string password;         // Empty = no password
    int         max_players  = 8;
    int         lane_count   = 3;
    bool        spec_lock    = false;
    bool        transmission_lock = false;
    bool        zero_tolerance_dnf = false;
    float       traffic_density = 0.5f;
    float       surface_friction = 1.0f;  // 1.0=dry, 0.65=wet, 0.35=oil
    uint8_t     forced_car_model = 0xFF;  // 0xFF = player choice
    
    // Serialize to JSON string
    [[nodiscard]] std::string toJSON() const;
    static RoomConfig fromJSON(const std::string& json);
};

// ── Join Request ───────────────────────────────────────────────────────────
struct JoinRequest {
    std::string nickname;
    std::string uuid;
    std::string room_password;
    std::string channel_code;     // Mode A: 3-6 digit code
    uint8_t     car_model = 0;
    
    [[nodiscard]] std::string toJSON() const;
    static JoinRequest fromJSON(const std::string& json);
};

} // namespace drt
