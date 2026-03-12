#pragma once

#include "IPacketSender.hpp"
#include "IStartable.hpp"
namespace vpsm::server::port {
    class ITcpManager : public IPacketSender {};
    class ITcpSession : public IPacketSender, public IStartable {};
}