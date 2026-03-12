#include "RoutingService.hpp"
#include "PacketParser.hpp"
#include "../domain/model/packetOut.hpp"
namespace vpsm::server::application {
    vpsm::server::domain::RouteAction RoutingService::route(domain::PacketIn pck) {
        auto header = PacketParser::parse(pck);
        if (!header.has_value()) {
            return domain::Drop{};
        }
        
        if (!memStore_.resolvePeer(header.value().vNetworkId, header.value().srcVip) ||
            !memStore_.resolvePeer(header.value().vNetworkId, header.value().dstVip)) {
            return domain::Drop{};
        }
        domain::PacketOut pckOut = {
            .buf = pck.buf,
            .size = pck.size,
            .type = pck.type,
            .dest = header.value().dstVip,
        };

        return domain::Forward{pckOut};
    }
}