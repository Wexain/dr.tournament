// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Friends System Implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "social/friends.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace drt {

void FriendsSystem::init() {
    loadFriendsList();
}

void FriendsSystem::update(float dt) {
    // In production: poll server for friend status updates
}

void FriendsSystem::addFriend(const std::string& uuid) {
    // Check duplicates
    for (const auto& f : friends_) {
        if (f.uuid == uuid) return;
    }
    Friend f;
    f.uuid = uuid;
    f.nickname = "Unknown";
    f.status = FriendStatus::OFFLINE;
    friends_.push_back(f);
    saveFriendsList();
}

void FriendsSystem::removeFriend(const std::string& uuid) {
    friends_.erase(
        std::remove_if(friends_.begin(), friends_.end(),
                        [&](const Friend& f) { return f.uuid == uuid; }),
        friends_.end());
    saveFriendsList();
}

void FriendsSystem::sendInvite(const std::string& uuid, const std::string& room_name) {
    // TODO: Send via network
    DirectMessage msg;
    msg.from_uuid = "local";
    msg.from_nickname = "You";
    msg.text = "Invited to room: " + room_name;
    messages_.push_back(msg);
}

void FriendsSystem::sendMessage(const std::string& uuid, const std::string& text) {
    // TODO: Send via network
    DirectMessage msg;
    msg.from_uuid = "local";
    msg.from_nickname = "You";
    msg.text = text;
    messages_.push_back(msg);
}

int FriendsSystem::unreadCount() const {
    int count = 0;
    for (const auto& m : messages_) {
        if (!m.read) count++;
    }
    return count;
}

bool FriendsSystem::saveFriendsList() const {
    try {
        json j = json::array();
        for (const auto& f : friends_) {
            j.push_back({{"uuid", f.uuid}, {"nickname", f.nickname}});
        }
        std::ofstream file("friends.json");
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool FriendsSystem::loadFriendsList() {
    try {
        std::ifstream file("friends.json");
        if (!file.is_open()) return false;
        json j = json::parse(file);
        friends_.clear();
        for (const auto& item : j) {
            Friend f;
            f.uuid = item.value("uuid", "");
            f.nickname = item.value("nickname", "Unknown");
            f.status = FriendStatus::OFFLINE;
            friends_.push_back(f);
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace drt
