#pragma once

#include "../model/headerV2.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace vpsm::server::domain {
    enum class AuthErrorV2 {
        OUTER_PARSE_FAILED,
        SESSION_NOT_FOUND,
        REPLAY_DETECTED,
        INNER_PARSE_FAILED,
        AUTH_TAG_INVALID,
    };

    struct AuthResultV2 {
        OutPacketHeaderV2 outer;
        InnerPacketHeaderV2 inner;
        std::optional<std::uint64_t> authenticatedPeerId = std::nullopt;
        std::shared_ptr<std::vector<std::uint8_t>> plaintextInnerAndPayload;
    };
}
