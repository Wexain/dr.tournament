#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Friends & Social System
// ═══════════════════════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include <cstdint>

namespace drt {

enum class FriendStatus : uint8_t { OFFLINE, ONLINE, IN_RACE, IN_MENU };

struct Friend {
    std::string uuid;
    std::string nickname;
    FriendStatus status = FriendStatus::OFFLINE;
};

struct DirectMessage {
    std::string from_uuid;
    std::string from_nickname;
    std::string text;
    uint64_t    timestamp = 0;
    bool        read = false;
};

class FriendsSystem {
public:
    void init();
    void update(float dt);

    void addFriend(const std::string& uuid);
    void removeFriend(const std::string& uuid);
    void sendInvite(const std::string& uuid, const std::string& room_name);
    void sendMessage(const std::string& uuid, const std::string& text);

    [[nodiscard]] const std::vector<Friend>& friendsList() const { return friends_; }
    [[nodiscard]] const std::vector<DirectMessage>& messages() const { return messages_; }
    [[nodiscard]] int unreadCount() const;

    bool saveFriendsList() const;
    bool loadFriendsList();

private:
    std::vector<Friend>        friends_;
    std::vector<DirectMessage> messages_;
};

} // namespace drt
