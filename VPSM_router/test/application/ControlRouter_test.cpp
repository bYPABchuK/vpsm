#include "../../src/application/ControlPlane/ControlRouter.hpp"
#include "../../src/application/ControlPlane/HealthEndpoint.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::ControlResponse;
    using vpsm::server::application::ControlRouter;
    using vpsm::server::application::HealthEndpoint;
    using vpsm::server::application::IControlEndpoint;

    class EchoPathParamEndpoint final : public IControlEndpoint {
    public:
        ControlResponse handle(const ControlRequest& request) override {
            const auto it = request.pathParams.find("peerId");
            if (it == request.pathParams.end()) {
                return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'b','a','d'}};
            }
            return ControlResponse{
                .status = 200,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>(it->second.begin(), it->second.end()),
            };
        }
    };

    class MultiParamEndpoint final : public IControlEndpoint {
    public:
        ControlResponse handle(const ControlRequest& request) override {
            const auto nIt = request.pathParams.find("networkId");
            const auto pIt = request.pathParams.find("peerId");
            if (nIt == request.pathParams.end() || pIt == request.pathParams.end()) {
                return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'b','a','d'}};
            }

            const auto merged = nIt->second + ":" + pIt->second;
            return ControlResponse{
                .status = 200,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>(merged.begin(), merged.end()),
            };
        }
    };

    class StaticWinsEndpoint final : public IControlEndpoint {
    public:
        explicit StaticWinsEndpoint(std::string text)
            : text_(std::move(text)) {}

        ControlResponse handle(const ControlRequest&) override {
            return ControlResponse{
                .status = 200,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>(text_.begin(), text_.end()),
            };
        }

    private:
        std::string text_;
    };

    TEST(ControlRouterTest, route_UnknownRoute_Returns404True) {
        ControlRouter router;

        const auto response = router.route(ControlRequest{
            .method = "GET",
            .path = "/unknown",
        });

        EXPECT_EQ(response.status, 404);
    }

    TEST(ControlRouterTest, route_HealthRoute_Returns200AndOkBodyTrue) {
        ControlRouter router;
        auto health = std::make_shared<HealthEndpoint>();
        router.addRoute("GET", "/health", health);

        const auto response = router.route(ControlRequest{
            .method = "GET",
            .path = "/health",
        });

        EXPECT_EQ(response.status, 200);
        ASSERT_EQ(response.body.size(), 2u);
        EXPECT_EQ(response.body[0], static_cast<std::uint8_t>('o'));
        EXPECT_EQ(response.body[1], static_cast<std::uint8_t>('k'));
    }

    TEST(ControlRouterTest, route_PathParams_ExtractedFromDynamicSegmentsTrue) {
        ControlRouter router;
        router.addRoute("GET", "/peers/{peerId}", std::make_shared<EchoPathParamEndpoint>());

        const auto response = router.route(ControlRequest{
            .method = "GET",
            .path = "/peers/12345",
        });

        EXPECT_EQ(response.status, 200);
        ASSERT_EQ(response.body.size(), 5u);
        EXPECT_EQ(response.body[0], static_cast<std::uint8_t>('1'));
        EXPECT_EQ(response.body[4], static_cast<std::uint8_t>('5'));
    }

    TEST(ControlRouterTest, route_PathExistsButMethodMismatch_Returns405True) {
        ControlRouter router;
        router.addRoute("GET", "/health", std::make_shared<HealthEndpoint>());

        const auto response = router.route(ControlRequest{
            .method = "POST",
            .path = "/health",
        });

        EXPECT_EQ(response.status, 405);
    }

    TEST(ControlRouterTest, route_MultiplePathParams_ExtractedForDeepPathTrue) {
        ControlRouter router;
        router.addRoute("PUT", "/networks/{networkId}/members/{peerId}", std::make_shared<MultiParamEndpoint>());

        const auto response = router.route(ControlRequest{
            .method = "PUT",
            .path = "/networks/77/members/1001",
        });

        EXPECT_EQ(response.status, 200);
        const std::string body(response.body.begin(), response.body.end());
        EXPECT_EQ(body, "77:1001");
    }

    TEST(ControlRouterTest, route_StaticSegmentHasPriorityOverDynamicTrue) {
        ControlRouter router;
        router.addRoute("GET", "/peers/me", std::make_shared<StaticWinsEndpoint>("static"));
        router.addRoute("GET", "/peers/{peerId}", std::make_shared<StaticWinsEndpoint>("dynamic"));

        const auto response = router.route(ControlRequest{
            .method = "GET",
            .path = "/peers/me",
        });

        EXPECT_EQ(response.status, 200);
        const std::string body(response.body.begin(), response.body.end());
        EXPECT_EQ(body, "static");
    }

    TEST(ControlRouterTest, route_QueryStringIgnoredDuringMatchingTrue) {
        ControlRouter router;
        router.addRoute("GET", "/health", std::make_shared<HealthEndpoint>());

        const auto response = router.route(ControlRequest{
            .method = "GET",
            .path = "/health?full=1",
        });

        EXPECT_EQ(response.status, 200);
    }
}
