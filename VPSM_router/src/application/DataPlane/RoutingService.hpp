#pragma once

#include "../../port/IRoutingService.hpp"
#include "../../port/IRoutingPipeline.hpp"
#include "../../port/IMembershipStore.hpp"
#include "../../port/IAuthServiceV2.hpp"
#include "../../port/IPeerEndpointRegistry.hpp"

#include <memory>

namespace vpsm::server::application {
    class RoutingService : public port::IRoutingService{
    public:
        RoutingService(
            port::IMembershipStore& memStore,
            port::IAuthServiceV2* authService = nullptr,
            port::IPeerEndpointRegistry* endpointRegistry = nullptr
        );

        domain::RouteAction route(domain::PacketIn pck) override;

    private:
        port::IMembershipStore& memStore_;
        port::IAuthServiceV2* authService_;
        port::IPeerEndpointRegistry* endpointRegistry_;
        std::unique_ptr<port::IRoutingPipeline> pipeline_;
    };
}