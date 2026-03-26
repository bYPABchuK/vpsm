#include "AtomicMetricCounter.hpp"
#include <type_traits>
namespace vpsm::server::application {
    void AtomicMetricCounter::routeActionCount(domain::RouteAction action) {
        std::visit([this](const auto& type) {
            using T = std::decay_t<decltype(type)>;
            if constexpr (std::is_same_v<T, domain::Forward>) {
                packetsForwarded_.fetch_add(1);
            }
            else if constexpr (std::is_same_v<T, domain::Drop>) {
                packetsDropped_.fetch_add(1);
            }
        }, action);
    }


    domain::MetricsSnapshot AtomicMetricCounter::snapshotAndReset() {
        return domain::MetricsSnapshot{
            .packetsReceived = packetsReceived_.exchange(0),
            .packetsForwarded = packetsForwarded_.exchange(0),
            .packetsDropped = packetsDropped_.exchange(0),
            .packetsResponded = packetsResponded_.exchange(0),

            .usersOnline = usersOnline_.exchange(0)
        };
    }

    domain::MetricsSnapshot AtomicMetricCounter::snapshotWithoutReset() {
        return (domain::MetricsSnapshot){
            .packetsReceived = packetsReceived_,
            .packetsForwarded = packetsForwarded_,
            .packetsDropped = packetsDropped_,
            .packetsResponded = packetsResponded_,

            .usersOnline = usersOnline_,
        };
    }
}