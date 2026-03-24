#include "MetricService.hpp"
#include <thread>
namespace vpsm::server::application {
    void MetricService::start() {
        if (started_) {
            return;
        }
        started_ = 1;
        while (started_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto snapshot = metricCounter_.snapshotAndReset();
            metricsSink_.flush(snapshot);
        }
    }

    void MetricService::stop() {
        started_ = 0;
    }
}