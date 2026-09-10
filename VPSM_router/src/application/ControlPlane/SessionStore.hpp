#pragma once

#include "../DataPlane/PacketCryptoV2.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace vpsm::server::application {
    class SessionStore {
    public:
        using Clock = std::chrono::steady_clock;
        using DataPlaneKey = PacketCryptoV2::Key;

        struct SessionData {
            std::uint64_t sessionId = 0;
            std::uint64_t sessionKey = 0;
            DataPlaneKey dataPlaneKey{};
            Clock::time_point expiresAt{};
        };

        struct DataPlaneSession {
            std::uint64_t sessionId = 0;
            std::uint64_t peerId = 0;
            DataPlaneKey key{};
        };

        explicit SessionStore(
            std::chrono::seconds sessionLifetime = std::chrono::hours(24)
        ) : sessionLifetime_(sessionLifetime) {}

        SessionData createSession(std::uint64_t peerId);
        std::optional<std::uint64_t> authenticate(
            std::uint64_t sessionId,
            std::uint64_t sessionKey,
            Clock::time_point now = Clock::now()
        ) const;
        std::optional<DataPlaneSession> beginDataPlanePacket(
            std::uint64_t sessionId,
            Clock::time_point now = Clock::now()
        ) const;
        bool commitSequence(
            std::uint64_t sessionId,
            std::uint64_t sequence,
            Clock::time_point now = Clock::now()
        );
        std::optional<std::uint64_t> nextOutboundSequence(
            std::uint64_t sessionId,
            Clock::time_point now = Clock::now()
        );
        bool revoke(std::uint64_t sessionId);
        std::size_t pruneExpired(Clock::time_point now = Clock::now());

        static std::string keyToHex(const DataPlaneKey& key);
        static std::optional<DataPlaneKey> keyFromHex(const std::string& hex);

    private:
        static constexpr std::uint64_t REPLAY_WINDOW_BITS = 64;

        struct Entry {
            std::uint64_t peerId = 0;
            std::uint64_t controlKey = 0;
            DataPlaneKey dataPlaneKey{};
            Clock::time_point expiresAt{};
            std::uint64_t highestInboundSequence = 0;
            std::uint64_t inboundSequenceBitmap = 0;
            std::uint64_t nextOutboundSequence = 1;
            bool revoked = false;
        };

        static bool isExpired(const Entry& entry, Clock::time_point now);
        static bool acceptSequence(Entry& entry, std::uint64_t sequence);

        mutable std::mutex mutex_;
        std::unordered_map<std::uint64_t, Entry> bySessionId_;
        std::chrono::seconds sessionLifetime_;
    };
}