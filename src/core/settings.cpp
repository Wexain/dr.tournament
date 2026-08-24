// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Settings Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "core/settings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <raylib.h>

using json = nlohmann::json;

namespace drt {

void Settings::init() {
    // Generate UUID if empty
    if (uuid_.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        const char hex[] = "0123456789abcdef";
        uuid_.resize(32);
        for (auto& c : uuid_) c = hex[dis(gen)];
        uuid_.insert(8, "-"); uuid_.insert(13, "-");
        uuid_.insert(18, "-"); uuid_.insert(23, "-");
    }
}

bool Settings::load(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        json j = json::parse(file);

        // Graphics
        if (j.contains("graphics")) {
            auto& g = j["graphics"];
            if (g.contains("resolution_w")) graphics_.resolution_w = g["resolution_w"];
            if (g.contains("resolution_h")) graphics_.resolution_h = g["resolution_h"];
            if (g.contains("render_scale")) graphics_.render_scale = g["render_scale"];
            if (g.contains("fullscreen"))   graphics_.fullscreen = g["fullscreen"];
            if (g.contains("vsync"))        graphics_.vsync = g["vsync"];
            if (g.contains("shadows"))      graphics_.shadows = static_cast<ShadowQuality>(g["shadows"].get<int>());
            if (g.contains("lod_level"))    graphics_.lod_level = g["lod_level"];
            if (g.contains("mirrors"))      graphics_.mirrors = g["mirrors"];
            if (g.contains("fxaa"))         graphics_.fxaa = g["fxaa"];
            if (g.contains("msaa"))         graphics_.msaa = g["msaa"];
            if (g.contains("view_distance")) graphics_.view_distance = g["view_distance"];
        }

        // Audio
        if (j.contains("audio")) {
            auto& a = j["audio"];
            if (a.contains("master_volume")) audio_.master_volume = a["master_volume"];
            if (a.contains("sfx_volume"))    audio_.sfx_volume = a["sfx_volume"];
            if (a.contains("music_volume"))  audio_.music_volume = a["music_volume"];
            if (a.contains("voice_volume"))  audio_.voice_volume = a["voice_volume"];
            if (a.contains("push_to_talk"))  audio_.push_to_talk = a["push_to_talk"];
            if (a.contains("voice_gain"))    audio_.voice_gain = a["voice_gain"];
        }

        // Controls
        if (j.contains("controls")) {
            auto& c = j["controls"];
            if (c.contains("steering_sensitivity")) controls_.steering_sensitivity = c["steering_sensitivity"];
            if (c.contains("gamepad_deadzone"))      controls_.gamepad_deadzone = c["gamepad_deadzone"];
            if (c.contains("gamepad_vibration"))     controls_.gamepad_vibration = c["gamepad_vibration"];
        }

        // Player
        if (j.contains("nickname")) nickname_ = j["nickname"];
        if (j.contains("uuid"))     uuid_ = j["uuid"];

        return true;
    } catch (...) {
        return false;
    }
}

bool Settings::save(const std::string& path) const {
    try {
        json j;

        j["graphics"] = {
            {"resolution_w", graphics_.resolution_w},
            {"resolution_h", graphics_.resolution_h},
            {"render_scale", graphics_.render_scale},
            {"fullscreen", graphics_.fullscreen},
            {"vsync", graphics_.vsync},
            {"shadows", static_cast<int>(graphics_.shadows)},
            {"lod_level", graphics_.lod_level},
            {"mirrors", graphics_.mirrors},
            {"fxaa", graphics_.fxaa},
            {"msaa", graphics_.msaa},
            {"view_distance", graphics_.view_distance}
        };

        j["audio"] = {
            {"master_volume", audio_.master_volume},
            {"sfx_volume", audio_.sfx_volume},
            {"music_volume", audio_.music_volume},
            {"voice_volume", audio_.voice_volume},
            {"push_to_talk", audio_.push_to_talk},
            {"voice_gain", audio_.voice_gain}
        };

        j["controls"] = {
            {"steering_sensitivity", controls_.steering_sensitivity},
            {"gamepad_deadzone", controls_.gamepad_deadzone},
            {"gamepad_vibration", controls_.gamepad_vibration}
        };

        j["nickname"] = nickname_;
        j["uuid"] = uuid_;

        std::ofstream file(path);
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

void Settings::applyGraphics() {
    SetMasterVolume(audio_.master_volume);
}

void Settings::autoDetectQuality() {
    // Probe GPU via Raylib
    // Simple heuristic: check if we can render at target FPS
    const char* renderer = "Unknown";
    // Note: in actual Raylib, we can check rlGetVersion() after init

    // Default to medium, then adjust
    setPreset(QualityPreset::MEDIUM);
}

void Settings::setPreset(QualityPreset p) {
    preset_ = p;
    switch (p) {
        case QualityPreset::LOW:    applyPresetLow(); break;
        case QualityPreset::MEDIUM: applyPresetMedium(); break;
        case QualityPreset::HIGH:   applyPresetHigh(); break;
        case QualityPreset::CUSTOM: break;
    }
}

void Settings::applyPresetLow() {
    graphics_.resolution_w = 1280;
    graphics_.resolution_h = 720;
    graphics_.render_scale = 0.75f;
    graphics_.shadows = ShadowQuality::BLOB;
    graphics_.lod_level = 0;
    graphics_.mirrors = false;
    graphics_.fxaa = false;
    graphics_.msaa = 0;
    graphics_.view_distance = 200.0f;
}

void Settings::applyPresetMedium() {
    graphics_.resolution_w = 1920;
    graphics_.resolution_h = 1080;
    graphics_.render_scale = 0.75f;
    graphics_.shadows = ShadowQuality::SIMPLE_DYNAMIC;
    graphics_.lod_level = 1;
    graphics_.mirrors = false;
    graphics_.fxaa = true;
    graphics_.msaa = 0;
    graphics_.view_distance = 300.0f;
}

void Settings::applyPresetHigh() {
    graphics_.resolution_w = 1920;
    graphics_.resolution_h = 1080;
    graphics_.render_scale = 1.0f;
    graphics_.shadows = ShadowQuality::SOFT;
    graphics_.lod_level = 2;
    graphics_.mirrors = true;
    graphics_.fxaa = true;
    graphics_.msaa = 4;
    graphics_.view_distance = 500.0f;
}

} // namespace drt
