#pragma once

#include "IPacketSender.hpp"
#include "IStartable.hpp"
namespace vpsm::server::port {
    class IUdpGateway : public IPacketSender, public IStartable {};
}