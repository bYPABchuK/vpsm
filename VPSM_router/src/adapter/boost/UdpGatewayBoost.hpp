#pragma once

#include "../../port/IUdpGateway.hpp"
#include "../../port/IRoutingService.hpp"

#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cstdint>

using boost::asio::ip::udp;

namespace vpsm::server::adapter::boostImpl {
    class UdpGatewayBoost final : public port::IUdpGateway {
    public:
        UdpGatewayBoost(boost::asio::io_context& io, 
            boost::asio::thread_pool& workers,
            port::IRoutingService& routingService,
            std::uint16_t bindPort
        ) : io_(io), workers_(workers), routingService_(routingService), bindPort_(bindPort), socket_(io) {};

        int start() override;

        int stop() override;

        int send(domain::PacketOut pkt) override;

        ~UdpGatewayBoost()
        {
            stop();
        }

    private:
        static constexpr std::size_t MAX_DATAGRAM_SIZE = 2048;

        boost::asio::thread_pool& workers_;
        boost::asio::io_context& io_;
        port::IRoutingService& routingService_;
        std::uint16_t bindPort_;

        bool started_ = false;
        boost::asio::ip::udp::socket socket_;
        std::array<std::uint8_t, MAX_DATAGRAM_SIZE> recvBuffer_{};
        boost::asio::ip::udp::endpoint remoteEndpoint_{};

        void recieving_();
    };
}