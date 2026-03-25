#include "ControlPlaneBoost.hpp"

#include "../adapter/MembershipStore.hpp"
#include "../adapter/boost/ControlHttpBoost.hpp"
#include "../application/ControlRouter.hpp"
#include "../application/DispatchEchoEndpoint.hpp"
#include "../application/HealthEndpoint.hpp"
#include "../application/UserService.hpp"
#include "../application/UserServiceEndpoint.hpp"
#include "../repository/InMemoryPeerRepository.hpp"
#include "../repository/InMemoryVNetworkRepository.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <memory>
#include <thread>
#include <vector>

namespace vpsm::server::infrastructure {
    class ControlPlaneBoost::Impl {
    public:
        Impl(std::uint16_t httpPort, std::uint16_t workerNum)
            : io_{},
              workGuard_{boost::asio::make_work_guard(io_)},
              peerRepository_{},
              networkRepository_{},
              membershipStore_{},
              userService_{peerRepository_, networkRepository_, membershipStore_},
              router_{std::make_shared<application::ControlRouter>()},
              health_{std::make_shared<application::HealthEndpoint>()},
              dispatchEcho_{std::make_shared<application::DispatchEchoEndpoint>()},
              createPeer_{std::make_shared<application::UserServiceEndpoint>(userService_, application::UserServiceEndpoint::Operation::CREATE_PEER)},
              createNetwork_{std::make_shared<application::UserServiceEndpoint>(userService_, application::UserServiceEndpoint::Operation::CREATE_NETWORK)},
              joinNetwork_{std::make_shared<application::UserServiceEndpoint>(userService_, application::UserServiceEndpoint::Operation::JOIN_NETWORK)},
              leaveNetwork_{std::make_shared<application::UserServiceEndpoint>(userService_, application::UserServiceEndpoint::Operation::LEAVE_NETWORK)},
              listener_{std::make_shared<adapter::boostImpl::HttpListenerBoost>(io_, router_, httpPort)},
              workerNum_{workerNum} {
            router_->addRoute("GET", "/health", health_);
            router_->addRoute("POST", "/dispatch/echo", dispatchEcho_);
            router_->addRoute("POST", "/user/create-peer", createPeer_);
            router_->addRoute("POST", "/user/create-network", createNetwork_);
            router_->addRoute("POST", "/user/join-network", joinNetwork_);
            router_->addRoute("POST", "/user/leave-network", leaveNetwork_);
        }

        int start() {
            if (started_) return 0;

            const auto listenRes = listener_->start();
            if (listenRes != 0) return listenRes;

            started_ = true;
            threads_.reserve(workerNum_);
            for (std::uint16_t i = 0; i < workerNum_; ++i) {
                threads_.emplace_back([this]() { io_.run(); });
            }
            return 0;
        }

        int stop() {
            if (!started_) return 0;

            started_ = false;
            listener_->stop();

            workGuard_.reset();
            io_.stop();

            for (auto& th : threads_) {
                if (th.joinable()) th.join();
            }
            threads_.clear();
            return 0;
        }

    private:
        boost::asio::io_context io_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard_;

        repository::InMemoryPeerRepository peerRepository_;
        repository::InMemoryVNetworkRepository networkRepository_;
        adapter::MembershipRegistry membershipStore_;
        application::UserService userService_;

        std::shared_ptr<application::ControlRouter> router_;
        std::shared_ptr<application::HealthEndpoint> health_;
        std::shared_ptr<application::DispatchEchoEndpoint> dispatchEcho_;
        std::shared_ptr<application::UserServiceEndpoint> createPeer_;
        std::shared_ptr<application::UserServiceEndpoint> createNetwork_;
        std::shared_ptr<application::UserServiceEndpoint> joinNetwork_;
        std::shared_ptr<application::UserServiceEndpoint> leaveNetwork_;
        std::shared_ptr<adapter::boostImpl::HttpListenerBoost> listener_;

        std::uint16_t workerNum_;
        std::vector<std::thread> threads_;
        bool started_ = false;
    };

    ControlPlaneBoost::ControlPlaneBoost(std::uint16_t httpPort, std::uint16_t workerNum)
        : impl_(new Impl(httpPort, workerNum)) {}

    ControlPlaneBoost::~ControlPlaneBoost() {
        if (impl_ != nullptr) {
            impl_->stop();
            delete impl_;
            impl_ = nullptr;
        }
    }

    int ControlPlaneBoost::start() {
        return impl_->start();
    }

    int ControlPlaneBoost::stop() {
        return impl_->stop();
    }
}
