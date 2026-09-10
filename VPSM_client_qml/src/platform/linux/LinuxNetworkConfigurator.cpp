#include "LinuxNetworkConfigurator.hpp"
#include "LinuxPrivilegedHelperClient.hpp"

namespace vpsm::client {
    LinuxNetworkConfigurator::LinuxNetworkConfigurator(std::shared_ptr<LinuxPrivilegedHelperClient> helper)
        : helper_(std::move(helper)) {}

    bool LinuxNetworkConfigurator::apply(const InterfaceConfiguration& configuration, QString& error) {
        if (!helper_) {
            error = QStringLiteral("Privilege helper is not available");
            return false;
        }
        return helper_->configure(configuration, error);
    }

    void LinuxNetworkConfigurator::remove(const InterfaceConfiguration&) {
        if (helper_) helper_->cleanup();
    }
}