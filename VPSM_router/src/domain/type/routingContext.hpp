#pragma once

#include "../model/packetIn.hpp"
#include "../model/headerV2.hpp"
#include "peerEndpoint.hpp"
#include "routeActions.hpp"

#include <optional>
#include <vector>
namespace vpsm::server::domain {
    struct RoutingContext {
        PacketIn packet;
        std::optional<OutPacketHeaderV2> outerHeaderV2;
        std::optional<InnerPacketHeaderV2> innerHeaderV2;

        std::optional<std::uint64_t> srcPeerId;
        std::optional<std::uint64_t> dstPeerId;
        std::optional<PeerEndpoint> dstEndpoint;
        std::shared_ptr<std::vector<std::uint8_t>> plaintextInnerAndPayload;

        RouteAction action = Drop{};
        bool stop = false;
    };
}
