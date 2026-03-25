#pragma once

#include "../model/packetIn.hpp"
#include "../model/headerV1.hpp"
#include "routeActions.hpp"

#include <optional>
namespace vpsm::server::domain {
    struct RoutingContext {
        PacketIn packet;
        std::optional<PacketHeader> header;

        std::optional<std::uint64_t> srcPeerId;
        std::optional<std::uint64_t> dstPeerId;

        RouteAction action = Drop{};
        bool stop = false;
    };
}
