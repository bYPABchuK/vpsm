#pragma once

#include "../model/packetIn.hpp"
#include "../model/headerV2.hpp"
#include "routeActions.hpp"

#include <optional>
namespace vpsm::server::domain {
    struct RoutingContext {
        PacketIn packet;
        std::optional<OutPacketHeaderV2> outerHeaderV2;
        std::optional<InnerPacketHeaderV2> innerHeaderV2;

        std::optional<std::uint64_t> srcPeerId;
        std::optional<std::uint64_t> dstPeerId;

        RouteAction action = Drop{};
        bool stop = false;
    };
}
