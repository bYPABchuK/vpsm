#pragma once

#include "../port/IMetricsSink.hpp"
#include <filesystem>
namespace vpsm::server::adapter {
    class FileMetricsSink : public port::IMetricsSink {
        public:
        FileMetricsSink(std::filesystem::path output) : output_(std::move(output)) {}

        port::MetricFlushErrors flush(const domain::MetricsSnapshot& snapshot) override;
        private:
        std::filesystem::path output_;
    };
}