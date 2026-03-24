#pragma once

#include "../domain/type/routeActions.hpp"
#include "../domain/type/metricSnapshot.hpp"
namespace vpsm::server::port {
    class IMetricCounter {
        public:
        virtual ~IMetricCounter() = default;
        
        virtual void routeActionCount(domain::RouteAction action) = 0;
        virtual void usersOnlineCount(domain::RouteAction action) = 0;

        virtual domain::MetricsSnapshot snapshotAndReset() = 0;
        virtual domain::MetricsSnapshot snapshotWithoutReset() = 0;
    };
}