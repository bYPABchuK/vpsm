#include "SessionStore.hpp"

#include <openssl/rand.h>

#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace vpsm::server::application {
    namespace {
        template <typename T>
        T secureRandomValue() {
            T value{};
            if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) {
                throw std::runtime_error("secure random generation failed");
            }
            return value;
        }

        SessionStore::DataPlaneKey secureRandomKey() {
            SessionStore::DataPlaneKey key{};
            if (RAND_bytes(key.data(), static_cast<int>(key.size())) != 1) {
                throw std::runtime_error("secure random generation failed");
            }
            return key;
        }

        std::optional<std::uint8_t> hexNibble(char value) {
            if (value >= '0' && value <= '9') return static_cast<std::uint8_t>(value - '0');
            if (value >= 'a' && value <= 'f') return static_cast<std::uint8_t>(10 + value - 'a');
            if (value >= 'A' && value <= 'F') return static_cast<std::uint8_t>(10 + value - 'A');
            return std::nullopt;
        }
    }

    SessionStore::SessionData SessionStore::createSession(std::uint64_t peerId) {
        std::lock_guard lock(mutex_);
        const auto now = Clock::now();
        for (auto it = bySessionId_.begin(); it != bySessionId_.end();) {
            if (it->second.revoked || isExpired(it->second, now)) it = bySessionId_.erase(it);
            else ++it;
        }
        std::uint64_t sessionId = 0;
        do {
            sessionId = secureRandomValue<std::uint64_t>();
        } while (sessionId == 0 || bySessionId_.find(sessionId) != bySessionId_.end());

        std::uint64_t controlKey = 0;
        do {
            controlKey = secureRandomValue<std::uint64_t>();
        } while (controlKey == 0);

        const auto dataPlaneKey = secureRandomKey();
        const auto expiresAt = now + sessionLifetime_;
        bySessionId_.emplace(sessionId, Entry{
            .peerId = peerId,
            .controlKey = controlKey,
            .dataPlaneKey = dataPlaneKey,
            .expiresAt = expiresAt,
        });
        return SessionData{
            .sessionId = sessionId,
            .sessionKey = controlKey,
            .dataPlaneKey = dataPlaneKey,
            .expiresAt = expiresAt,
        };
    }

    std::optional<std::uint64_t> SessionStore::authenticate(
        std::uint64_t sessionId, std::uint64_t sessionKey, Clock::time_point now
    ) const {
        std::lock_guard lock(mutex_);
        const auto it = bySessionId_.find(sessionId);
        if (it == bySessionId_.end() || it->second.revoked || isExpired(it->second, now) ||
            it->second.controlKey != sessionKey) return std::nullopt;
        return it->second.peerId;
    }

    std::optional<SessionStore::DataPlaneSession> SessionStore::beginDataPlanePacket(
        std::uint64_t sessionId, Clock::time_point now
    ) const {
        std::lock_guard lock(mutex_);
        const auto it = bySessionId_.find(sessionId);
        if (it == bySessionId_.end() || it->second.revoked || isExpired(it->second, now)) return std::nullopt;
        return DataPlaneSession{sessionId, it->second.peerId, it->second.dataPlaneKey};
    }

    bool SessionStore::commitSequence(
        std::uint64_t sessionId, std::uint64_t sequence, Clock::time_point now
    ) {
        std::lock_guard lock(mutex_);
        const auto it = bySessionId_.find(sessionId);
        if (it == bySessionId_.end() || it->second.revoked || isExpired(it->second, now)) return false;
        return acceptSequence(it->second, sequence);
    }

    std::optional<std::uint64_t> SessionStore::nextOutboundSequence(
        std::uint64_t sessionId, Clock::time_point now
    ) {
        std::lock_guard lock(mutex_);
        const auto it = bySessionId_.find(sessionId);
        if (it == bySessionId_.end() || it->second.revoked || isExpired(it->second, now) ||
            it->second.nextOutboundSequence == 0 ||
            it->second.nextOutboundSequence == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
        return it->second.nextOutboundSequence++;
    }

    bool SessionStore::revoke(std::uint64_t sessionId) {
        std::lock_guard lock(mutex_);
        const auto it = bySessionId_.find(sessionId);
        if (it == bySessionId_.end()) return false;
        it->second.revoked = true;
        return true;
    }

    std::size_t SessionStore::pruneExpired(Clock::time_point now) {
        std::lock_guard lock(mutex_);
        std::size_t removed = 0;
        for (auto it = bySessionId_.begin(); it != bySessionId_.end();) {
            if (it->second.revoked || isExpired(it->second, now)) {
                it = bySessionId_.erase(it);
                ++removed;
            } else ++it;
        }
        return removed;
    }

    std::string SessionStore::keyToHex(const DataPlaneKey& key) {
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (const auto byte : key) out << std::setw(2) << static_cast<unsigned>(byte);
        return out.str();
    }

    std::optional<SessionStore::DataPlaneKey> SessionStore::keyFromHex(const std::string& hex) {
        DataPlaneKey key{};
        if (hex.size() != key.size() * 2) return std::nullopt;
        for (std::size_t i = 0; i < key.size(); ++i) {
            const auto high = hexNibble(hex[i * 2]);
            const auto low = hexNibble(hex[i * 2 + 1]);
            if (!high || !low) return std::nullopt;
            key[i] = static_cast<std::uint8_t>((*high << 4) | *low);
        }
        return key;
    }

    bool SessionStore::isExpired(const Entry& entry, Clock::time_point now) {
        return now >= entry.expiresAt;
    }

    bool SessionStore::acceptSequence(Entry& entry, std::uint64_t sequence) {
        if (sequence == 0) return false;
        if (entry.highestInboundSequence == 0) {
            entry.highestInboundSequence = sequence;
            entry.inboundSequenceBitmap = 1;
            return true;
        }
        if (sequence > entry.highestInboundSequence) {
            const auto shift = sequence - entry.highestInboundSequence;
            entry.inboundSequenceBitmap = shift >= REPLAY_WINDOW_BITS
                ? 1 : (entry.inboundSequenceBitmap << shift) | 1;
            entry.highestInboundSequence = sequence;
            return true;
        }
        const auto age = entry.highestInboundSequence - sequence;
        if (age >= REPLAY_WINDOW_BITS) return false;
        const std::uint64_t mask = std::uint64_t{1} << age;
        if ((entry.inboundSequenceBitmap & mask) != 0) return false;
        entry.inboundSequenceBitmap |= mask;
        return true;
    }
}