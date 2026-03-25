#pragma once

#include "../model/headerV2.hpp"

#include <cstdint>

namespace vpsm::server::domain {
    enum class AuthErrorV2 {
        OUTER_PARSE_FAILED,
        SESSION_NOT_FOUND,
        REPLAY_DETECTED,
        INNER_PARSE_FAILED,
    };

    struct AuthResultV2 {
        OutPacketHeaderV2 outer;
        InnerPacketHeaderV2 inner;
    };
}
