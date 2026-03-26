#pragma once

#include "../../port/IRoutingService.hpp"
#include "../../port/IMetricCounter.hpp"
namespace vpsm::server::application {
    class LogRoutingService : public port::IRoutingService {
    public:
        LogRoutingService(port::IRoutingService& routeService,
            port::IMetricCounter& Metrics)
            : routeService_(routeService),
            Metrics_(Metrics) {};

        domain::RouteAction route(domain::PacketIn pck);
    private:
        port::IRoutingService& routeService_;
        port::IMetricCounter& Metrics_;
    };
}