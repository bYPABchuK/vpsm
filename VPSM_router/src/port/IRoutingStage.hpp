#pragma once

#include "../domain/type/routingContext.hpp"

namespace vpsm::server::port {
    class IRoutingStage {
    public:
        virtual ~IRoutingStage() = default;
        virtual void execute(domain::RoutingContext& ctx) = 0;
    };
}
