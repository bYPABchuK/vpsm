#pragma once

#include "../model/packetOut.hpp"
#include <cstdint>
#include <variant>
namespace vpsm::server::domain {
    enum class DropReason : std::uint8_t {
        UNKNOWN = 0,
        PARSE,
        AUTH,
        MEMBERSHIP,
        NO_ENDPOINT,
    };

    struct Drop {
        DropReason reason = DropReason::UNKNOWN;
    };

    struct Forward {
        PacketOut packet;
    };

    using RouteAction = std::variant<Drop, Forward>;
}
