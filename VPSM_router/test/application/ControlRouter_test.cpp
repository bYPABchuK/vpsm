#include "../../src/application/ControlRouter.hpp"
#include "../../src/application/HealthEndpoint.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::ControlRouter;
    using vpsm::server::application::HealthEndpoint;

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
}
