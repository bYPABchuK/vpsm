#pragma once

#include "../port/IStartable.hpp"

#include <cstdint>
#include <memory>

namespace vpsm::server::port {
    class IMembershipStore;
}

namespace vpsm::server::infrastructure {
    class ControlPlaneBoost final : public port::IStartable {
    public:
        ControlPlaneBoost(
            std::uint16_t httpPort,
            std::uint16_t workerNum,
            std::shared_ptr<port::IMembershipStore> membershipStore = nullptr
        );
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
