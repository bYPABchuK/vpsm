#pragma once

#include "../port/IDataPlane.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace vpsm::server::port {
    class IMembershipStore;
}

namespace vpsm::server::application {
    class SessionStore;
}

namespace vpsm::server::infrastructure {
    class DataPlaneBoost final : public port::IDataPlane {
    public:
        DataPlaneBoost(
            std::uint16_t udpPort,
            std::uint16_t workerNum,
            std::filesystem::path metricsOutput = "vpsm_metrics.prom",
            std::shared_ptr<port::IMembershipStore> membershipStore = nullptr,
            std::shared_ptr<application::SessionStore> sessionStore = nullptr
        );
        ~DataPlaneBoost();
        DataPlaneBoost(const DataPlaneBoost&) = delete;
        DataPlaneBoost& operator=(const DataPlaneBoost&) = delete;
        int start() override;
        int stop() override;

    private:
        class Impl;
        Impl* impl_;
    };
}
