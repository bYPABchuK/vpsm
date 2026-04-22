#pragma once

#include "../model/headerV2.hpp"

#include <cstdint>
#include <optional>

namespace vpsm::server::domain {
    struct SessionAuthStateV2 {
        std::uint64_t peerId;
        std::uint64_t highestSeq;
    };

    enum class AuthErrorV2 {
        OUTER_PARSE_FAILED,
        SESSION_NOT_FOUND,
        REPLAY_DETECTED,
        INNER_PARSE_FAILED,
    };

    struct AuthResultV2 {
        OutPacketHeaderV2 outer;
        InnerPacketHeaderV2 inner;
        std::optional<std::uint64_t> authenticatedPeerId = std::nullopt;
    };
}
