#pragma once

#include "../port/IRoutingService.hpp"
#include "../port/IRoutingPipeline.hpp"
#include "../port/IMembershipStore.hpp"
#include "../port/IAuthService.hpp"

#include <memory>

namespace vpsm::server::application {
    class RoutingService : public port::IRoutingService{
    public:
        RoutingService(
            port::IMembershipStore& memStore,
            port::IAuthService* authService = nullptr
        );

        domain::RouteAction route(domain::PacketIn pck) override;

    private:
        port::IMembershipStore& memStore_;
        port::IAuthService* authService_;
        std::unique_ptr<port::IRoutingPipeline> pipeline_;
    };
}