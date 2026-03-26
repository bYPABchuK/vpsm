#include "RoutingService.hpp"
#include "PacketParserV2.hpp"
#include "../../domain/model/packetOut.hpp"
#include "../../port/IRoutingStage.hpp"

#include <memory>
#include <vector>
namespace vpsm::server::application {
    namespace stage { /*later divide this maybe*/
        class ParseRoutingStage final : public port::IRoutingStage {
        public:
            void execute(domain::RoutingContext& ctx) override {
                ctx.outerHeaderV2 = PacketParserV2::parseOuter(ctx.packet);
                if (!ctx.outerHeaderV2.has_value()) {
                    ctx.action = domain::Drop{};
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
                    ctx.action = domain::Drop{};
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
                        ctx.action = domain::Drop{};
                        ctx.stop = true;
                        return;
                    }

                    ctx.innerHeaderV2 = inner;
                    return;
                }

                const auto authResult = authService_->verifyAndDecrypt(ctx.packet);
                if (!authResult.has_value()) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                    return;
                }

                ctx.outerHeaderV2 = authResult->outer;
                ctx.innerHeaderV2 = authResult->inner;
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
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                    return;
                }

                ctx.srcPeerId = memStore_.resolvePeer(ctx.innerHeaderV2->vNetworkId, ctx.innerHeaderV2->srcVip);
                ctx.dstPeerId = memStore_.resolvePeer(ctx.innerHeaderV2->vNetworkId, ctx.innerHeaderV2->dstVip);

                if (!ctx.srcPeerId.has_value() || !ctx.dstPeerId.has_value()) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                }
            }

        private:
            port::IMembershipStore& memStore_;
        };

        class ForwardRoutingStage final : public port::IRoutingStage {
        public:
            void execute(domain::RoutingContext& ctx) override {
                if (!ctx.innerHeaderV2.has_value()) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                    return;
                }

                domain::PacketOut packetOut{
                    .buf = ctx.packet.buf,
                    .size = ctx.packet.size,
                    .type = ctx.packet.type,
                    .dest = ctx.innerHeaderV2->dstVip,
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
        port::IAuthServiceV2* authService
    )
        : memStore_(memStore),
          authService_(authService) {
        std::vector<std::unique_ptr<port::IRoutingStage>> stages;
        stages.emplace_back(std::make_unique<stage::ParseRoutingStage>());
        stages.emplace_back(std::make_unique<stage::AuthRoutingStage>(authService_));
        stages.emplace_back(std::make_unique<stage::MembershipRoutingStage>(memStore_));
        stages.emplace_back(std::make_unique<stage::ForwardRoutingStage>());

        pipeline_ = std::make_unique<stage::DefaultRoutingPipeline>(std::move(stages));
    }

    vpsm::server::domain::RouteAction RoutingService::route(domain::PacketIn pck) {
        domain::RoutingContext ctx{
            .packet = std::move(pck),
        };

        pipeline_->process(ctx);

        if (!ctx.stop) {
            return domain::Drop{};
        }

        return ctx.action;
    }
}