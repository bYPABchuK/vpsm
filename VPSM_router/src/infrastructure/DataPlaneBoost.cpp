#include "DataPlaneBoost.hpp"

#include "../adapter/GatewayManager.hpp"
#include "../adapter/MembershipStore.hpp"
#include "../adapter/boost/TcpManagerBoost.hpp"
#include "../adapter/boost/UdpGatewayBoost.hpp"
#include "../application/DataPlane/RoutingService.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>

#include <thread>

namespace vpsm::server::infrastructure {
    class DataPlaneBoost::Impl {
    public:
        Impl(std::uint16_t udpPort, std::uint16_t workerNum)
            : io_{},
              workGuard_{boost::asio::make_work_guard(io_)},
              workers_{workerNum},
              membershipStore_{},
              routingService_{membershipStore_},
              udpGateway_{io_, workers_, routingService_, udpPort},
              tcpManager_{io_, workers_},
              sender_{udpGateway_, tcpManager_} {}

        int start() {
            if (started_) return 0;

            const auto udpRes = udpGateway_.start();
            if (udpRes != 0) return udpRes;

            started_ = true;
            ioThread_ = std::thread([this]() { io_.run(); });
            return 0;
        }

        int stop() {
            if (!started_) return 0;

            started_ = false;
            udpGateway_.stop();

            workGuard_.reset();
            io_.stop();

            if (ioThread_.joinable()) {
                ioThread_.join();
            }

            workers_.join();
            return 0;
        }

    private:
        boost::asio::io_context io_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard_;
        boost::asio::thread_pool workers_;

        adapter::MembershipRegistry membershipStore_;
        application::RoutingService routingService_;
        adapter::boostImpl::UdpGatewayBoost udpGateway_;
        adapter::boostImpl::TcpManagerBoost tcpManager_;
        adapter::GatewayManager sender_;

        std::thread ioThread_;
        bool started_ = false;
    };

    DataPlaneBoost::DataPlaneBoost(std::uint16_t udpPort, std::uint16_t workerNum)
        : impl_(new Impl(udpPort, workerNum)) {}

    DataPlaneBoost::~DataPlaneBoost() {
        if (impl_ != nullptr) {
            impl_->stop();
            delete impl_;
            impl_ = nullptr;
        }
    }

    int DataPlaneBoost::start() {
        return impl_->start();
    }

    int DataPlaneBoost::stop() {
        return impl_->stop();
    }
}
