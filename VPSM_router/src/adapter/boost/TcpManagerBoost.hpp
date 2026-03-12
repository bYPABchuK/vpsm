#pragma once

#include "../../port/ITcpManager.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>

namespace vpsm::server::adapter::boostImpl {
    class TcpManagerBoost final : public port::ITcpManager {
        public:
        TcpManagerBoost(boost::asio::io_context& io, boost::asio::thread_pool& workers) : io_(io), workers_(workers) {};

        int send(domain::PacketOut pkt) override;

        private:
        boost::asio::thread_pool& workers_;
        boost::asio::io_context& io_;
    };
}