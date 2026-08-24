#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Voice Chat (WebRTC via libdatachannel)
// ═══════════════════════════════════════════════════════════════════════════
#include <cstdint>
#include <string>
#include <functional>
#include <array>

namespace drt {

// Voice chat is scaffolded but requires libdatachannel + opus at link time.
// When DRT_ENABLE_VOICE is OFF, all methods are no-ops.

enum class VoiceMode : uint8_t {
    PUSH_TO_TALK,
    OPEN_MIC,
    DISABLED
};

class VoiceChat {
public:
    void init();
    void shutdown();
    void update(float dt);

    // Connection
    void connectToSignaling(const std::string& host, uint16_t port);
    void disconnect();

    // Push-to-talk
    void setPTT(bool pressed);
    void setMode(VoiceMode mode) { mode_ = mode; }
    [[nodiscard]] VoiceMode mode() const { return mode_; }

    // Volume control per player (0-200%)
    void setPlayerVolume(int player_id, float volume);
    [[nodiscard]] float playerVolume(int player_id) const;

    // Noise suppression
    void setNoiseSuppression(bool on) { noise_suppression_ = on; }

    // Query speaking state
    [[nodiscard]] bool isSpeaking(int player_id) const;
    [[nodiscard]] bool isLocalSpeaking() const;

    // WebRTC signaling callbacks (called by ENetClient)
    void onRemoteOffer(int peer_id, const std::string& sdp);
    void onRemoteAnswer(int peer_id, const std::string& sdp);
    void onRemoteICE(int peer_id, const std::string& candidate);

private:
    VoiceMode mode_ = VoiceMode::PUSH_TO_TALK;
    bool      ptt_active_ = false;
    bool      noise_suppression_ = true;
    
    std::array<float, 16> player_volumes_;
    std::array<bool, 16>  player_speaking_;
    
    bool initialized_ = false;
};

} // namespace drt
