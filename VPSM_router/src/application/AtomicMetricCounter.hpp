#pragma once

#include "../port/IMetricCounter.hpp"
#include "cstdint"
#include "atomic"
namespace vpsm::server::application {
    class AtomicMetricCounter : public port::IMetricCounter {
        public:
        void routeActionCount(domain::RouteAction action) override;

        
        domain::MetricsSnapshot snapshotAndReset() override;
        domain::MetricsSnapshot snapshotWithoutReset() override;

        private:
        inline domain::MetricsSnapshot generateSnapshot();
        inline void reset();


        std::atomic<std::uint64_t> packetsReceived_{0};
        std::atomic<std::uint64_t> packetsForwarded_{0};
        std::atomic<std::uint64_t> packetsDropped_{0};
        std::atomic<std::uint64_t> packetsResponded_{0};

        std::atomic<std::uint64_t> usersOnline_{0};
    };
}