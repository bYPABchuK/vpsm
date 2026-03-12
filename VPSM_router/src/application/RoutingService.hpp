#pragma once

#include "../port/IRoutingService.hpp"
#include "../port/IMembershipStore.hpp"
namespace vpsm::server::application {
    class RoutingService : public port::IRoutingService{
    public:
        RoutingService(port::IMembershipStore& memStore) : memStore_(memStore) {};

        domain::RouteAction route(domain::PacketIn pck);
    private:
        port::IMembershipStore& memStore_;
    };
}