 #include "../../src/application/DataPlane/LogRoutingService.hpp"

 #include <gtest/gtest.h>

 namespace {
     using vpsm::server::application::LogRoutingService;
     using vpsm::server::domain::Drop;
     using vpsm::server::domain::Forward;
     using vpsm::server::domain::PacketIn;
     using vpsm::server::domain::RouteAction;

     class RoutingServiceFake final : public vpsm::server::port::IRoutingService {
     public:
         RouteAction route(PacketIn) override {
             ++calls;
             return actionToReturn;
         }

         int calls = 0;
         RouteAction actionToReturn = Drop{};
     };

     class MetricCounterFake final : public vpsm::server::port::IMetricCounter {
     public:
         void routeActionCount(RouteAction action) override {
             ++routeActionCalls;
             lastAction = action;
         }

         void usersOnlineCount(RouteAction) override {}

         vpsm::server::domain::MetricsSnapshot snapshotAndReset() override {
             return {};
         }

         vpsm::server::domain::MetricsSnapshot snapshotWithoutReset() override {
             return {};
         }

         int routeActionCalls = 0;
         RouteAction lastAction = Drop{};
     };

     TEST(LogRoutingServiceTest, route_WhenForward_DelegatesAndCounts) {
         RoutingServiceFake base;
         MetricCounterFake metrics;
         base.actionToReturn = Forward{};

         LogRoutingService service(base, metrics);
         PacketIn pkt{};

         const auto result = service.route(pkt);

         EXPECT_TRUE(std::holds_alternative<Forward>(result));
         EXPECT_EQ(base.calls, 1);
         EXPECT_EQ(metrics.routeActionCalls, 1);
         EXPECT_TRUE(std::holds_alternative<Forward>(metrics.lastAction));
     }

     TEST(LogRoutingServiceTest, route_WhenDrop_DelegatesAndCounts) {
         RoutingServiceFake base;
         MetricCounterFake metrics;
         base.actionToReturn = Drop{};

         LogRoutingService service(base, metrics);
         PacketIn pkt{};

         const auto result = service.route(pkt);

         EXPECT_TRUE(std::holds_alternative<Drop>(result));
         EXPECT_EQ(base.calls, 1);
         EXPECT_EQ(metrics.routeActionCalls, 1);
         EXPECT_TRUE(std::holds_alternative<Drop>(metrics.lastAction));
     }
 }