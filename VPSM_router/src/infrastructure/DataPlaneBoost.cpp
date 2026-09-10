#include "DataPlaneBoost.hpp"

#include "../adapter/FileMetricsSink.hpp"
#include "../adapter/GatewayManager.hpp"
#include "../adapter/MembershipStore.hpp"
#include "../adapter/PeerEndpointRegistry.hpp"
#include "../adapter/boost/TcpManagerBoost.hpp"
#include "../adapter/boost/UdpGatewayBoost.hpp"
#include "../application/DataPlane/AtomicMetricCounter.hpp"
#include "../application/DataPlane/AuthServiceV2.hpp"
#include "../application/ControlPlane/SessionStore.hpp"
#include "../application/DataPlane/LogRoutingService.hpp"
#include "../application/DataPlane/MetricService.hpp"
#include "../application/DataPlane/RoutingService.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>

#include <filesystem>
#include <thread>

namespace vpsm::server::infrastructure {
    namespace {
        std::shared_ptr<port::IMembershipStore> ensureMembershipStore(
            std::shared_ptr<port::IMembershipStore> membershipStore
        ) {
            if (!membershipStore) {
                membershipStore = std::make_shared<adapter::MembershipRegistry>();
            }
            return membershipStore;
        }

        std::shared_ptr<application::SessionStore> ensureSessionStore(
            std::shared_ptr<application::SessionStore> sessionStore
        ) {
            return sessionStore ? std::move(sessionStore) : std::make_shared<application::SessionStore>();
        }
    }

    class DataPlaneBoost::Impl {
    public:
        Impl(
            std::uint16_t udpPort,
            std::uint16_t workerNum,
            std::filesystem::path metricsOutput,
            std::shared_ptr<port::IMembershipStore> membershipStore,
            std::shared_ptr<application::SessionStore> sessionStore
        )
            : io_{},
              workGuard_{boost::asio::make_work_guard(io_)},
              workers_{workerNum},
              membershipStore_{ensureMembershipStore(std::move(membershipStore))},
              sessionStore_{ensureSessionStore(std::move(sessionStore))},
              endpointRegistry_{},
              authService_{*sessionStore_},
              routingService_{*membershipStore_, authService_, &endpointRegistry_},
              logRoutingService_{routingService_, metricCounter_},
              udpGateway_{io_, workers_, logRoutingService_, udpPort},
              tcpManager_{io_, workers_},
              sender_{udpGateway_, tcpManager_},
              metricsSink_{std::move(metricsOutput)},
              metricService_{metricCounter_, metricsSink_} {}

        int start() {
            if (started_) return 0;

            const auto udpRes = udpGateway_.start();
            if (udpRes != 0) return udpRes;

            started_ = true;
            metricThread_ = std::thread([this]() { metricService_.start(); });
            ioThread_ = std::thread([this]() { io_.run(); });
            return 0;
        }

        int stop() {
            if (!started_) return 0;

            started_ = false;
            udpGateway_.stop();
            metricService_.stop();

            workGuard_.reset();
            io_.stop();

            if (metricThread_.joinable()) {
                metricThread_.join();
            }

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

        std::shared_ptr<port::IMembershipStore> membershipStore_;
        std::shared_ptr<application::SessionStore> sessionStore_;
        adapter::PeerEndpointRegistry endpointRegistry_;
        application::AuthServiceV2 authService_;
        application::RoutingService routingService_;
        application::AtomicMetricCounter metricCounter_;
        application::LogRoutingService logRoutingService_;
        adapter::boostImpl::UdpGatewayBoost udpGateway_;
        adapter::boostImpl::TcpManagerBoost tcpManager_;
        adapter::GatewayManager sender_;
        adapter::FileMetricsSink metricsSink_;
        application::MetricService metricService_;

        std::thread metricThread_;
        std::thread ioThread_;
        bool started_ = false;
    };

    DataPlaneBoost::DataPlaneBoost(
        std::uint16_t udpPort,
        std::uint16_t workerNum,
        std::filesystem::path metricsOutput,
        std::shared_ptr<port::IMembershipStore> membershipStore,
        std::shared_ptr<application::SessionStore> sessionStore
    )
        : impl_(new Impl(
            udpPort, workerNum, std::move(metricsOutput),
            std::move(membershipStore), std::move(sessionStore)
        )) {}

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
