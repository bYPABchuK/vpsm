#include "AtomicMetricCounter.hpp"
#include <type_traits>
namespace vpsm::server::application {
    void AtomicMetricCounter::routeActionCount(domain::RouteAction action) {
        packetsReceived_.fetch_add(1);
        std::visit([this](const auto& type) {
            using T = std::decay_t<decltype(type)>;
            if constexpr (std::is_same_v<T, domain::Forward>) {
                packetsForwarded_.fetch_add(1);
            }
            else if constexpr (std::is_same_v<T, domain::Drop>) {
                packetsDropped_.fetch_add(1);
                switch (type.reason) {
                    case domain::DropReason::PARSE:
                        packetsDroppedParse_.fetch_add(1);
                        break;
                    case domain::DropReason::AUTH:
                        packetsDroppedAuth_.fetch_add(1);
                        break;
                    case domain::DropReason::MEMBERSHIP:
                        packetsDroppedMembership_.fetch_add(1);
                        break;
                    case domain::DropReason::NO_ENDPOINT:
                        packetsDroppedNoEndpoint_.fetch_add(1);
                        break;
                    case domain::DropReason::UNKNOWN:
                    default:
                        break;
                }
            }
        }, action);
    }

    void AtomicMetricCounter::usersOnlineCount(domain::RouteAction) {
        usersOnline_.fetch_add(1);
    }


    domain::MetricsSnapshot AtomicMetricCounter::snapshotAndReset() {
        return domain::MetricsSnapshot{
            .packetsReceived = packetsReceived_.exchange(0),
            .packetsForwarded = packetsForwarded_.exchange(0),
            .packetsDropped = packetsDropped_.exchange(0),
            .packetsDroppedParse = packetsDroppedParse_.exchange(0),
            .packetsDroppedAuth = packetsDroppedAuth_.exchange(0),
            .packetsDroppedMembership = packetsDroppedMembership_.exchange(0),
            .packetsDroppedNoEndpoint = packetsDroppedNoEndpoint_.exchange(0),
            .packetsResponded = packetsResponded_.exchange(0),

            .usersOnline = usersOnline_.exchange(0)
        };
    }

    domain::MetricsSnapshot AtomicMetricCounter::snapshotWithoutReset() {
        return (domain::MetricsSnapshot){
            .packetsReceived = packetsReceived_,
            .packetsForwarded = packetsForwarded_,
            .packetsDropped = packetsDropped_,
            .packetsDroppedParse = packetsDroppedParse_,
            .packetsDroppedAuth = packetsDroppedAuth_,
            .packetsDroppedMembership = packetsDroppedMembership_,
            .packetsDroppedNoEndpoint = packetsDroppedNoEndpoint_,
            .packetsResponded = packetsResponded_,

            .usersOnline = usersOnline_,
        };
    }
}