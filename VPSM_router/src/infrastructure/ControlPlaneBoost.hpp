#pragma once

#include "../port/IStartable.hpp"

#include <cstdint>

namespace vpsm::server::infrastructure {
    class ControlPlaneBoost final : public port::IStartable {
    public:
        ControlPlaneBoost(std::uint16_t httpPort, std::uint16_t workerNum);
        ~ControlPlaneBoost();
        ControlPlaneBoost(const ControlPlaneBoost&) = delete;
        ControlPlaneBoost& operator=(const ControlPlaneBoost&) = delete;

        int start() override;
        int stop() override;

    private:
        class Impl;
        Impl* impl_;
    };
}
