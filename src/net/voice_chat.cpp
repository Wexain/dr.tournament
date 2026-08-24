// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Voice Chat Stub Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "net/voice_chat.h"

namespace drt {

void VoiceChat::init() {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Initialize libdatachannel, create PeerConnectionFactory
    // TODO: Initialize Opus encoder/decoder
    // TODO: Initialize audio capture via Raylib or platform audio API
    #endif
    
    player_volumes_.fill(1.0f);
    player_speaking_.fill(false);
    initialized_ = true;
}

void VoiceChat::shutdown() {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Close all peer connections
    // TODO: Release Opus codec instances
    // TODO: Stop audio capture
    #endif
    initialized_ = false;
}

void VoiceChat::update(float dt) {
    if (!initialized_) return;
    
    #ifdef DRT_ENABLE_VOICE
    // TODO: Poll libdatachannel for incoming audio frames
    // TODO: Decode Opus frames and queue for playback
    // TODO: If PTT active or open mic, capture and encode audio
    // TODO: Send encoded frames via WebRTC data channel
    #endif
}

void VoiceChat::connectToSignaling(const std::string& host, uint16_t port) {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Connect to signaling server via WebSocket
    // TODO: Exchange SDP offers/answers
    #endif
}

void VoiceChat::disconnect() {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Close all peer connections
    // TODO: Disconnect from signaling server
    #endif
    player_speaking_.fill(false);
}

void VoiceChat::setPTT(bool pressed) {
    ptt_active_ = pressed;
}

void VoiceChat::setPlayerVolume(int player_id, float volume) {
    if (player_id >= 0 && player_id < 16) {
        player_volumes_[player_id] = std::clamp(volume, 0.0f, 2.0f);
    }
}

float VoiceChat::playerVolume(int player_id) const {
    if (player_id >= 0 && player_id < 16) return player_volumes_[player_id];
    return 1.0f;
}

bool VoiceChat::isSpeaking(int player_id) const {
    if (player_id >= 0 && player_id < 16) return player_speaking_[player_id];
    return false;
}

bool VoiceChat::isLocalSpeaking() const {
    if (mode_ == VoiceMode::DISABLED) return false;
    if (mode_ == VoiceMode::PUSH_TO_TALK) return ptt_active_;
    return true; // Open mic
}

void VoiceChat::onRemoteOffer(int peer_id, const std::string& sdp) {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Create PeerConnection, set remote description, create answer
    #endif
}

void VoiceChat::onRemoteAnswer(int peer_id, const std::string& sdp) {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Set remote description on existing PeerConnection
    #endif
}

void VoiceChat::onRemoteICE(int peer_id, const std::string& candidate) {
    #ifdef DRT_ENABLE_VOICE
    // TODO: Add ICE candidate to PeerConnection
    #endif
}

} // namespace drt
