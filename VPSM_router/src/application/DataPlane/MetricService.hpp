#pragma once

#include "../../port/IMetricService.hpp"
#include "../../port/IMetricCounter.hpp"
#include "../../port/IMetricsSink.hpp"
namespace vpsm::server::application {
    class MetricService : public port::IMetricService {
        public:
            MetricService(port::IMetricCounter& metricCounter,
                port::IMetricsSink& metricsSink)
                : metricCounter_(metricCounter),
                metricsSink_(metricsSink)
                {};

            void start();
            void stop();
        
        private:
            bool started_ = 0;
            port::IMetricCounter& metricCounter_;
            port::IMetricsSink& metricsSink_;

    };
}