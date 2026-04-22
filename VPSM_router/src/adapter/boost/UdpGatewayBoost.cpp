#include "UdpGatewayBoost.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include <memory>
#include <vector>

namespace vpsm::server::adapter::boostImpl {
    int UdpGatewayBoost::start() {
        if (started_) {
            return 0;
        }

        boost::system::error_code ec;
        socket_.open(udp::v4(), ec);
        if (ec) {
            return -1;
        }

        socket_.bind(udp::endpoint(udp::v4(), bindPort_), ec);
        if (ec) {
            return -2;
        }

        started_ = 1;
        recieving_();
        return 0;
    }

    int UdpGatewayBoost::stop() {
        if (!started_) {
            return 0;
        }

        started_ = false;

        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);

        return 0;
    }

    int UdpGatewayBoost::send(domain::PacketOut pkt) {
        boost::asio::post(io_, [this, pkt = std::move(pkt)]() mutable {
            if (!started_ || !pkt.buf) {
                return;
            }

            const auto endpoint = udp::endpoint(
                boost::asio::ip::address_v4(pkt.destIp),
                pkt.destPort
            );

            socket_.async_send_to(
                boost::asio::buffer(pkt.buf->data(), pkt.size),
                endpoint,
                [buf = pkt.buf](const boost::system::error_code&, std::size_t) {}
            );
        });

        return 0;
    }

    void UdpGatewayBoost::recieving_() {
        if (!started_) {
            return;
        }

        socket_.async_receive_from(
            boost::asio::buffer(recvBuffer_.data(), recvBuffer_.size()),
            remoteEndpoint_,
            [this](const boost::system::error_code& ec, std::size_t bytesReceived) {
                if (!started_) {
                    return;
                }

                if (!ec) {
                    auto buf = std::make_shared<std::vector<std::uint8_t>>(
                        recvBuffer_.begin(),
                        recvBuffer_.begin() + static_cast<std::ptrdiff_t>(bytesReceived)
                    );

                    domain::PacketIn pkt{
                        .buf = buf,
                        .size = bytesReceived,
                        .type = domain::UDP,
                        .sourceIp = remoteEndpoint_.address().to_v4().to_uint(),
                        .sourcePort = remoteEndpoint_.port(),
                    };

                    boost::asio::post(workers_, [this, pkt = std::move(pkt)]() mutable {
                        auto action = routingService_.route(std::move(pkt));

                        if (auto* fwd = std::get_if<domain::Forward>(&action)) {
                            this->send(std::move(fwd->packet));
                        }
                    });
                }

                recieving_();
            }
        );
    }

}