#pragma once

#include "../port/IDataPlane.hpp"

#include <cstdint>

namespace vpsm::server::infrastructure {
    class DataPlaneBoost final : public port::IDataPlane {
    public:
        DataPlaneBoost(std::uint16_t udpPort, std::uint16_t workerNum);
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
