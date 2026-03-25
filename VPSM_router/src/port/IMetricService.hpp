#pragma once

namespace vpsm::server::port {
    class IMetricService {
    public:
        virtual ~IMetricService() = default;

        virtual void start() {}
        virtual void stop() {}
    };
}