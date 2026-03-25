#include "RoutingService.hpp"
#include "PacketParser.hpp"
#include "../domain/model/packetOut.hpp"
#include "../port/IRoutingStage.hpp"

#include <memory>
#include <vector>
namespace vpsm::server::application {
    namespace stage { /*later divide this maybe*/
        class ParseRoutingStage final : public port::IRoutingStage {
        public:
            void execute(domain::RoutingContext& ctx) override {
                ctx.header = PacketParser::parse(ctx.packet);
                if (!ctx.header.has_value()) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                }
            }
        };

        class AuthRoutingStage final : public port::IRoutingStage {
        public:
            explicit AuthRoutingStage(port::IAuthService* authService)
                : authService_(authService) {}

            void execute(domain::RoutingContext& ctx) override {
                if (authService_ == nullptr || !ctx.header.has_value()) {
                    return;
                }

                if (!authService_->verify(*ctx.header, ctx.packet)) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                }
            }

        private:
            port::IAuthService* authService_;
        };

        class MembershipRoutingStage final : public port::IRoutingStage {
        public:
            explicit MembershipRoutingStage(port::IMembershipStore& memStore)
                : memStore_(memStore) {}

            void execute(domain::RoutingContext& ctx) override {
                if (!ctx.header.has_value()) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                    return;
                }

                ctx.srcPeerId = memStore_.resolvePeer(ctx.header->vNetworkId, ctx.header->srcVip);
                ctx.dstPeerId = memStore_.resolvePeer(ctx.header->vNetworkId, ctx.header->dstVip);

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
                if (!ctx.header.has_value()) {
                    ctx.action = domain::Drop{};
                    ctx.stop = true;
                    return;
                }

                domain::PacketOut packetOut{
                    .buf = ctx.packet.buf,
                    .size = ctx.packet.size,
                    .type = ctx.packet.type,
                    .dest = ctx.header->dstVip,
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
        port::IAuthService* authService
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