#pragma once

#include "../domain/type/metricSnapshot.hpp"
namespace vpsm::server::port {
    enum MetricFlushErrors {
        METRIC_FLUSH_OK = 0,
        METRIC_FLUSH_OPEN_TMP_FAILED,
        METRIC_FLUSH_WRITE_FAILED,
        METRIC_FLUSH_FSYNC_FILE_FAILED,
        METRIC_FLUSH_RENAME_FAILED,
        METRIC_FLUSH_OPEN_DIR_FAILED,
        METRIC_FLUSH_FSYNC_DIR_FAILED,
    };

    class IMetricsSink {
    public:
        virtual ~IMetricsSink() = default;
        virtual MetricFlushErrors flush(const domain::MetricsSnapshot& snapshot) = 0;
    };
}