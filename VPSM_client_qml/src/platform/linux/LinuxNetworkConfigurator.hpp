#pragma once

#include "application/ports/INetworkConfigurator.hpp"

#include <memory>

namespace vpsm::client {
    class LinuxPrivilegedHelperClient;

    class LinuxNetworkConfigurator final : public INetworkConfigurator {
    public:
        explicit LinuxNetworkConfigurator(std::shared_ptr<LinuxPrivilegedHelperClient> helper);
        bool apply(const InterfaceConfiguration& configuration, QString& error) override;
        void remove(const InterfaceConfiguration& configuration) override;

    private:
        std::shared_ptr<LinuxPrivilegedHelperClient> helper_;
    };
}