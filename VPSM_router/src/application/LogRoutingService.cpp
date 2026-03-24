#include "LogRoutingService.hpp"

namespace vpsm::server::application {
    domain::RouteAction LogRoutingService::route(domain::PacketIn pck) {
        auto result = routeService_.route(pck);
        Metrics_.routeActionCount(result);

        return result;
    }
}