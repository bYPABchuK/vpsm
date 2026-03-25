#pragma once

#include "../domain/model/packetIn.hpp"
#include "../domain/type/routeActions.hpp"
namespace vpsm::server::port {
    class IRoutingService {
        public:
        virtual domain::RouteAction route(domain::PacketIn pck) = 0;
    };
}