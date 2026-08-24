#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// Dr. Tournaments — Fixed-Size Memory Pool (Zero-Allocation Game Loop)
// ═══════════════════════════════════════════════════════════════════════════
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace drt {

// ── FixedPool<T, Capacity> ─────────────────────────────────────────────────
// Pre-allocated object pool with O(1) acquire/release and zero heap allocs.
// Uses a free-list embedded in the storage slots themselves.
template <typename T, std::size_t Capacity>
class FixedPool {
    static_assert(Capacity > 0, "Pool capacity must be > 0");
    static_assert(sizeof(T) >= sizeof(uint32_t), "T must be >= 4 bytes for free-list node");

public:
    struct Handle {
        uint32_t index = UINT32_MAX;
        [[nodiscard]] bool valid() const { return index != UINT32_MAX; }
        bool operator==(const Handle&) const = default;
    };

    FixedPool() { clear(); }

    void clear() {
        active_.reset();
        count_ = 0;
        free_head_ = 0;
        for (uint32_t i = 0; i < Capacity; ++i) {
            *reinterpret_cast<uint32_t*>(&storage_[i]) =
                (i + 1 < Capacity) ? (i + 1) : UINT32_MAX;
        }
    }

    // Acquire a slot and construct T in-place
    template <typename... Args>
    [[nodiscard]] Handle acquire(Args&&... args) {
        if (free_head_ == UINT32_MAX) return Handle{};
        uint32_t idx = free_head_;
        free_head_ = *reinterpret_cast<uint32_t*>(&storage_[idx]);
        new (&storage_[idx]) T(std::forward<Args>(args)...);
        active_.set(idx);
        ++count_;
        return Handle{idx};
    }

    // Release a slot, destroy T
    void release(Handle h) {
        if (!h.valid() || !active_.test(h.index)) return;
        get(h)->~T();
        active_.reset(h.index);
        *reinterpret_cast<uint32_t*>(&storage_[h.index]) = free_head_;
        free_head_ = h.index;
        --count_;
    }

    [[nodiscard]] T* get(Handle h) {
        if (!h.valid() || !active_.test(h.index)) return nullptr;
        return reinterpret_cast<T*>(&storage_[h.index]);
    }

    [[nodiscard]] const T* get(Handle h) const {
        if (!h.valid() || !active_.test(h.index)) return nullptr;
        return reinterpret_cast<const T*>(&storage_[h.index]);
    }

    [[nodiscard]] bool is_active(Handle h) const {
        return h.valid() && active_.test(h.index);
    }

    [[nodiscard]] std::size_t count() const { return count_; }
    [[nodiscard]] std::size_t capacity() const { return Capacity; }
    [[nodiscard]] bool full() const { return count_ == Capacity; }
    [[nodiscard]] bool empty() const { return count_ == 0; }

    // Iterate all active elements
    template <typename Fn>
    void for_each(Fn&& fn) {
        for (std::size_t i = 0; i < Capacity; ++i) {
            if (active_.test(i)) {
                fn(Handle{static_cast<uint32_t>(i)},
                   *reinterpret_cast<T*>(&storage_[i]));
            }
        }
    }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (std::size_t i = 0; i < Capacity; ++i) {
            if (active_.test(i)) {
                fn(Handle{static_cast<uint32_t>(i)},
                   *reinterpret_cast<const T*>(&storage_[i]));
            }
        }
    }

private:
    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::array<Storage, Capacity> storage_;
    std::bitset<Capacity>         active_;
    uint32_t                      free_head_ = 0;
    std::size_t                   count_     = 0;
};

// ── Convenience Pool Sizes ─────────────────────────────────────────────────
inline constexpr std::size_t MAX_VEHICLES   = 32;
inline constexpr std::size_t MAX_NPC_CARS   = 64;
inline constexpr std::size_t MAX_PARTICLES  = 512;
inline constexpr std::size_t MAX_PLAYERS    = 16;
inline constexpr std::size_t MAX_ROAD_CHUNKS = 128;

} // namespace drt
