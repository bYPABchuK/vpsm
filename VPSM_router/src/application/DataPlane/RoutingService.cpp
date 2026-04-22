#include "RoutingService.hpp"
#include "PacketParserV2.hpp"
#include "../../domain/model/packetOut.hpp"
#include "../../port/IRoutingStage.hpp"

#include <chrono>
#include <memory>
#include <vector>
namespace vpsm::server::application {
    namespace stage { /*later divide this maybe*/
        static constexpr auto DEMO_ENDPOINT_TTL = std::chrono::minutes(30);

        class ParseRoutingStage final : public port::IRoutingStage {
        public:
            void execute(domain::RoutingContext& ctx) override {
                ctx.outerHeaderV2 = PacketParserV2::parseOuter(ctx.packet);
                if (!ctx.outerHeaderV2.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::PARSE};
                    ctx.stop = true;
                }
            }
        };

        class AuthRoutingStage final : public port::IRoutingStage {
        public:
            explicit AuthRoutingStage(port::IAuthServiceV2* authService)
                : authService_(authService) {}

            void execute(domain::RoutingContext& ctx) override {
                if (!ctx.outerHeaderV2.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::PARSE};
                    ctx.stop = true;
                    return;
                }

                if (authService_ == nullptr) {
                    const auto inner = PacketParserV2::parseInner(
                        ctx.packet.buf->data(),
                        ctx.packet.size,
                        domain::OUTER_HEADER_V2_SIZE
                    );
                    if (!inner.has_value()) {
                        ctx.action = domain::Drop{.reason = domain::DropReason::PARSE};
                        ctx.stop = true;
                        return;
                    }

                    ctx.innerHeaderV2 = inner;
                    return;
                }

                const auto authResult = authService_->verifyAndDecrypt(ctx.packet);
                if (!authResult.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::AUTH};
                    ctx.stop = true;
                    return;
                }

                ctx.outerHeaderV2 = authResult->outer;
                ctx.innerHeaderV2 = authResult->inner;
                ctx.srcPeerId = authResult->authenticatedPeerId;
            }

        private:
            port::IAuthServiceV2* authService_;
        };

        class MembershipRoutingStage final : public port::IRoutingStage {
        public:
            explicit MembershipRoutingStage(port::IMembershipStore& memStore)
                : memStore_(memStore) {}

            void execute(domain::RoutingContext& ctx) override {
                if (!ctx.innerHeaderV2.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::PARSE};
                    ctx.stop = true;
                    return;
                }

                if (ctx.srcPeerId.has_value()) {
                    const auto expectedSrcVip = memStore_.resolveVip(ctx.innerHeaderV2->vNetworkId, *ctx.srcPeerId);
                    if (!expectedSrcVip.has_value() || *expectedSrcVip != ctx.innerHeaderV2->srcVip) {
                        ctx.action = domain::Drop{.reason = domain::DropReason::MEMBERSHIP};
                        ctx.stop = true;
                        return;
                    }
                } else {
                    ctx.srcPeerId = memStore_.resolvePeer(ctx.innerHeaderV2->vNetworkId, ctx.innerHeaderV2->srcVip);
                }

                ctx.dstPeerId = memStore_.resolvePeer(ctx.innerHeaderV2->vNetworkId, ctx.innerHeaderV2->dstVip);

                if (!ctx.srcPeerId.has_value() || !ctx.dstPeerId.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::MEMBERSHIP};
                    ctx.stop = true;
                }
            }

        private:
            port::IMembershipStore& memStore_;
        };

        class EndpointRoutingStage final : public port::IRoutingStage {
        public:
            explicit EndpointRoutingStage(port::IPeerEndpointRegistry* endpointRegistry)
                : endpointRegistry_(endpointRegistry) {}

            void execute(domain::RoutingContext& ctx) override {
                if (!ctx.dstPeerId.has_value() || endpointRegistry_ == nullptr) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::NO_ENDPOINT};
                    ctx.stop = true;
                    return;
                }

                endpointRegistry_->pruneExpired(
                    std::chrono::steady_clock::now(),
                    DEMO_ENDPOINT_TTL
                );

                ctx.dstEndpoint = endpointRegistry_->resolve(*ctx.dstPeerId);
                if (!ctx.dstEndpoint.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::NO_ENDPOINT};
                    ctx.stop = true;
                }
            }

        private:
            port::IPeerEndpointRegistry* endpointRegistry_;
        };

        class SourceEndpointUpdateStage final : public port::IRoutingStage {
        public:
            explicit SourceEndpointUpdateStage(port::IPeerEndpointRegistry* endpointRegistry)
                : endpointRegistry_(endpointRegistry) {}

            void execute(domain::RoutingContext& ctx) override {
                if (endpointRegistry_ == nullptr || !ctx.srcPeerId.has_value()) {
                    return;
                }

                if (ctx.packet.sourceIp == 0 || ctx.packet.sourcePort == 0) {
                    return;
                }

                endpointRegistry_->upsert(
                    *ctx.srcPeerId,
                    domain::PeerEndpoint{
                        .ip = ctx.packet.sourceIp,
                        .port = ctx.packet.sourcePort,
                    },
                    std::chrono::steady_clock::now()
                );
            }

        private:
            port::IPeerEndpointRegistry* endpointRegistry_;
        };

        class ForwardRoutingStage final : public port::IRoutingStage {
        public:
            void execute(domain::RoutingContext& ctx) override {
                if (!ctx.innerHeaderV2.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::PARSE};
                    ctx.stop = true;
                    return;
                }

                if (!ctx.dstEndpoint.has_value()) {
                    ctx.action = domain::Drop{.reason = domain::DropReason::NO_ENDPOINT};
                    ctx.stop = true;
                    return;
                }

                domain::PacketOut packetOut{
                    .buf = ctx.packet.buf,
                    .size = ctx.packet.size,
                    .type = ctx.packet.type,
                    .destIp = ctx.dstEndpoint->ip,
                    .destPort = ctx.dstEndpoint->port,
                };

                ctx.action = domain::Forward{packetOut};
                ctx.stop = true;
            }
        };

        class DefaultRoutingPipeline final : public port::IRoutingPipeline {
        public:
            explicit DefaultRoutingPipeline(std::vector<std::unique_ptr<port::IRoutingStage>> stages)
                : stages_(std::move(stages)) {}

            void process(domain::RoutingContext& ctx) override {
                for (auto& stage : stages_) {
                    if (ctx.stop) {
                        break;
                    }
                    stage->execute(ctx);
                }
            }

        private:
            std::vector<std::unique_ptr<port::IRoutingStage>> stages_;
        };
    }

    RoutingService::RoutingService(
        port::IMembershipStore& memStore,
        port::IAuthServiceV2* authService,
        port::IPeerEndpointRegistry* endpointRegistry
    )
        : memStore_(memStore),
          authService_(authService),
          endpointRegistry_(endpointRegistry) {
        std::vector<std::unique_ptr<port::IRoutingStage>> stages;
        stages.emplace_back(std::make_unique<stage::ParseRoutingStage>());
        stages.emplace_back(std::make_unique<stage::AuthRoutingStage>(authService_));
        stages.emplace_back(std::make_unique<stage::MembershipRoutingStage>(memStore_));
        stages.emplace_back(std::make_unique<stage::SourceEndpointUpdateStage>(endpointRegistry_));
        stages.emplace_back(std::make_unique<stage::EndpointRoutingStage>(endpointRegistry_));
        stages.emplace_back(std::make_unique<stage::ForwardRoutingStage>());

        pipeline_ = std::make_unique<stage::DefaultRoutingPipeline>(std::move(stages));
    }

    vpsm::server::domain::RouteAction RoutingService::route(domain::PacketIn pck) {
        domain::RoutingContext ctx{
            .packet = std::move(pck),
        };

        pipeline_->process(ctx);

        if (!ctx.stop) {
            return domain::Drop{.reason = domain::DropReason::UNKNOWN};
        }

        return ctx.action;
    }
}