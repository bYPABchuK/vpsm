#pragma once

#include "../domain/type/routingContext.hpp"
namespace vpsm::server::port {
    class IRoutingPipeline {
    public:
        virtual ~IRoutingPipeline() = default;
        virtual void process(domain::RoutingContext& ctx) = 0;
    };
}
