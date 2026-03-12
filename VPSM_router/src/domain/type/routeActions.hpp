#pragma once

#include "../model/packetOut.hpp"
#include <variant>
namespace vpsm::server::domain {
    struct Drop {};

    struct Forward {
        PacketOut packet;
    };

    using RouteAction = std::variant<Drop, Forward>;

}
