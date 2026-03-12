#include "../port/IPacketSender.hpp"
#include "../port/IUdpGateway.hpp"
#include "../port/ITcpManager.hpp"

namespace vpsm::server::adapter {
    class GatewayManager final : public port::IPacketSender {
        public:
        GatewayManager(port::IUdpGateway& udpGateway, port::ITcpManager& tcpManager) : 
            udpGateway_(udpGateway), tcpManager_(tcpManager) {};

        int send(server::domain::PacketOut pkt) override {
            switch (pkt.type) {
                case domain::UDP : return udpGateway_.send(pkt);
                case domain::TCP : return tcpManager_.send(pkt);
                default: return -1;
            }

        }

        private:
        port::IUdpGateway& udpGateway_;
        port::ITcpManager& tcpManager_;
    };
}