#pragma once

#include "domain/TunnelTypes.hpp"

#include <QString>

namespace vpsm::client {
    struct InterfaceConfiguration {
        QString interfaceName;
        VirtualNetwork network;
    };

    class INetworkConfigurator {
    public:
        virtual ~INetworkConfigurator() = default;
        virtual bool apply(const InterfaceConfiguration& configuration, QString& error) = 0;
        virtual void remove(const InterfaceConfiguration& configuration) = 0;
    };
}