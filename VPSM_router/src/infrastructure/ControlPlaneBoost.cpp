#include "ControlPlaneBoost.hpp"

#include "../adapter/MembershipStore.hpp"
#include "../adapter/boost/ControlHttpBoost.hpp"
#include "../application/ControlPlane/ControlRouter.hpp"
#include "../application/ControlPlane/DispatchEchoEndpoint.hpp"
#include "../application/ControlPlane/JsonCreateNetworkDecoder.hpp"
#include "../application/ControlPlane/JsonCreateNetworkResponseEncoder.hpp"
#include "../application/ControlPlane/JsonCreatePeerDecoder.hpp"
#include "../application/ControlPlane/JsonCreatePeerResponseEncoder.hpp"
#include "../application/ControlPlane/HealthEndpoint.hpp"
#include "../application/ControlPlane/JsonJoinNetworkDecoder.hpp"
#include "../application/ControlPlane/JsonJoinNetworkResponseEncoder.hpp"
#include "../application/ControlPlane/JsonLeaveNetworkDecoder.hpp"
#include "../application/ControlPlane/JsonLeaveNetworkResponseEncoder.hpp"
#include "../application/ControlPlane/JsonLoginDecoder.hpp"
#include "../application/ControlPlane/JsonLoginResponseEncoder.hpp"
#include "../application/ControlPlane/JsonRequestDecoder.hpp"
#include "../application/ControlPlane/JsonResponseEncoder.hpp"
#include "../application/ControlPlane/SessionStore.hpp"
#include "../application/ControlPlane/Endpoints/CreateNetworkAuthEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/CreateNetworkEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/CreatePeerEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/JoinNetworkEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/LeaveNetworkEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/LoginEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/NetworkPeersListEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/NetworkUserAddEndpoint.hpp"
#include "../application/ControlPlane/Endpoints/UserNetworkListEndpoint.hpp"
#include "../application/DataPlane/UserService.hpp"
#include "../repository/InMemoryPeerRepository.hpp"
#include "../repository/InMemoryVNetworkRepository.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
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
              requestDecoder_{std::make_shared<application::JsonRequestDecoder>()},
              responseEncoder_{std::make_shared<application::JsonResponseEncoder>()},
              createPeerDecoder_{std::make_shared<application::JsonCreatePeerDecoder>()},
              createPeerResponseEncoder_{std::make_shared<application::JsonCreatePeerResponseEncoder>()},
              createNetworkDecoder_{std::make_shared<application::JsonCreateNetworkDecoder>()},
              createNetworkResponseEncoder_{std::make_shared<application::JsonCreateNetworkResponseEncoder>()},
              joinNetworkDecoder_{std::make_shared<application::JsonJoinNetworkDecoder>()},
              joinNetworkResponseEncoder_{std::make_shared<application::JsonJoinNetworkResponseEncoder>()},
              leaveNetworkDecoder_{std::make_shared<application::JsonLeaveNetworkDecoder>()},
              leaveNetworkResponseEncoder_{std::make_shared<application::JsonLeaveNetworkResponseEncoder>()},
              loginDecoder_{std::make_shared<application::JsonLoginDecoder>()},
              loginResponseEncoder_{std::make_shared<application::JsonLoginResponseEncoder>()},
              dispatchEcho_{std::make_shared<application::DispatchEchoEndpoint>(*requestDecoder_, *responseEncoder_)},
              createPeer_{std::make_shared<application::endpoints::CreatePeerEndpoint>(userService_, *createPeerDecoder_, *createPeerResponseEncoder_)},
              createNetwork_{std::make_shared<application::endpoints::CreateNetworkEndpoint>(userService_, *createNetworkDecoder_, *createNetworkResponseEncoder_)},
              joinNetwork_{std::make_shared<application::endpoints::JoinNetworkEndpoint>(userService_, *joinNetworkDecoder_, *joinNetworkResponseEncoder_, "PUT")},
              leaveNetwork_{std::make_shared<application::endpoints::LeaveNetworkEndpoint>(userService_, *leaveNetworkDecoder_, *leaveNetworkResponseEncoder_, "DELETE")},
              login_{std::make_shared<application::endpoints::LoginEndpoint>(userService_, sessionStore_, *loginDecoder_, *loginResponseEncoder_)},
              createNetworkAuth_{std::make_shared<application::endpoints::CreateNetworkAuthEndpoint>(userService_)},
              networkUserAdd_{std::make_shared<application::endpoints::NetworkUserAddEndpoint>(userService_)},
              userNetworkList_{std::make_shared<application::endpoints::UserNetworkListEndpoint>(userService_)},
              networkPeersList_{std::make_shared<application::endpoints::NetworkPeersListEndpoint>(userService_)},
              joinNetworkLegacy_{std::make_shared<application::endpoints::JoinNetworkEndpoint>(userService_, *joinNetworkDecoder_, *joinNetworkResponseEncoder_, "POST")},
              leaveNetworkLegacy_{std::make_shared<application::endpoints::LeaveNetworkEndpoint>(userService_, *leaveNetworkDecoder_, *leaveNetworkResponseEncoder_, "POST")},
              listener_{std::make_shared<adapter::boostImpl::HttpListenerBoost>(io_, router_, httpPort)},
              workerNum_{workerNum} {
            router_->setAuthenticator([this](const application::ControlRequest& request) -> std::optional<std::uint64_t> {
                const auto headerByName = [&](const std::initializer_list<const char*>& names) -> std::optional<std::string> {
                    for (const auto* name : names) {
                        const auto it = request.headers.find(name);
                        if (it != request.headers.end() && !it->second.empty()) {
                            return it->second;
                        }
                    }
                    return std::nullopt;
                };

                const auto sessionIdText = headerByName({"sessionId", "SessionId", "X-Session-Id", "x-session-id"});
                const auto sessionKeyText = headerByName({"sessionKey", "SessionKey", "X-Session-Key", "x-session-key"});
                if (!sessionIdText.has_value() || !sessionKeyText.has_value()) {
                    return std::nullopt;
                }

                std::uint64_t sessionId = 0;
                std::uint64_t sessionKey = 0;
                try {
                    sessionId = static_cast<std::uint64_t>(std::stoull(*sessionIdText));
                    sessionKey = static_cast<std::uint64_t>(std::stoull(*sessionKeyText));
                } catch (...) {
                    return std::nullopt;
                }

                return sessionStore_.authenticate(sessionId, sessionKey);
            });

            router_->addRoute("GET", "/health", health_);
            router_->addRoute("POST", "/dispatch/echo", dispatchEcho_);
            router_->addRoute("POST", "/user/login", login_);

            router_->addRoute("POST", "/network/create", createNetworkAuth_);
            router_->addRoute("PUT", "/network/{id}/user-add", networkUserAdd_);
            router_->addRoute("GET", "/user/{id}/network-list", userNetworkList_);
            router_->addRoute("GET", "/network/{id}/peers-list", networkPeersList_);

            router_->addRoute("POST", "/user/peers", createPeer_);
            router_->addRoute("POST", "/user/networks", createNetwork_);
            router_->addRoute("PUT", "/user/networks/{networkId}/members/{peerId}", joinNetwork_);
            router_->addRoute("DELETE", "/user/networks/{networkId}/members/{peerId}", leaveNetwork_);

            router_->addRoute("POST", "/user/create-peer", createPeer_);
            router_->addRoute("POST", "/user/create-network", createNetwork_);
            router_->addRoute("POST", "/user/join-network", joinNetworkLegacy_);
            router_->addRoute("POST", "/user/leave-network", leaveNetworkLegacy_);
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
        std::shared_ptr<application::JsonRequestDecoder> requestDecoder_;
        std::shared_ptr<application::JsonResponseEncoder> responseEncoder_;
        std::shared_ptr<application::JsonCreatePeerDecoder> createPeerDecoder_;
        std::shared_ptr<application::JsonCreatePeerResponseEncoder> createPeerResponseEncoder_;
        std::shared_ptr<application::JsonCreateNetworkDecoder> createNetworkDecoder_;
        std::shared_ptr<application::JsonCreateNetworkResponseEncoder> createNetworkResponseEncoder_;
        std::shared_ptr<application::JsonJoinNetworkDecoder> joinNetworkDecoder_;
        std::shared_ptr<application::JsonJoinNetworkResponseEncoder> joinNetworkResponseEncoder_;
        std::shared_ptr<application::JsonLeaveNetworkDecoder> leaveNetworkDecoder_;
        std::shared_ptr<application::JsonLeaveNetworkResponseEncoder> leaveNetworkResponseEncoder_;
        application::SessionStore sessionStore_;
        std::shared_ptr<application::JsonLoginDecoder> loginDecoder_;
        std::shared_ptr<application::JsonLoginResponseEncoder> loginResponseEncoder_;
        std::shared_ptr<application::DispatchEchoEndpoint> dispatchEcho_;
        std::shared_ptr<application::endpoints::CreatePeerEndpoint> createPeer_;
        std::shared_ptr<application::endpoints::CreateNetworkEndpoint> createNetwork_;
        std::shared_ptr<application::endpoints::JoinNetworkEndpoint> joinNetwork_;
        std::shared_ptr<application::endpoints::LeaveNetworkEndpoint> leaveNetwork_;
        std::shared_ptr<application::endpoints::LoginEndpoint> login_;
        std::shared_ptr<application::endpoints::CreateNetworkAuthEndpoint> createNetworkAuth_;
        std::shared_ptr<application::endpoints::NetworkUserAddEndpoint> networkUserAdd_;
        std::shared_ptr<application::endpoints::UserNetworkListEndpoint> userNetworkList_;
        std::shared_ptr<application::endpoints::NetworkPeersListEndpoint> networkPeersList_;
        std::shared_ptr<application::endpoints::JoinNetworkEndpoint> joinNetworkLegacy_;
        std::shared_ptr<application::endpoints::LeaveNetworkEndpoint> leaveNetworkLegacy_;
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
