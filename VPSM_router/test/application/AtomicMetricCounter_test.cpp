#include "../../src/application/DataPlane/AtomicMetricCounter.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {
    using vpsm::server::application::AtomicMetricCounter;
    using vpsm::server::domain::Drop;
    using vpsm::server::domain::Forward;
    using vpsm::server::domain::PacketOut;

    class AtomicMetricCounterTestDouble final : public AtomicMetricCounter {
    public:
        void usersOnlineCount(vpsm::server::domain::RouteAction action) override {
            AtomicMetricCounter::usersOnlineCount(action);
        }
    };

    vpsm::server::domain::RouteAction makeForwardAction() {
        PacketOut packet{
            .buf = std::make_shared<std::vector<std::uint8_t>>(),
            .size = 0,
            .type = vpsm::server::domain::UDP,
            .destIp = 0,
            .destPort = 0,
        };
        return Forward{packet};
    }

    TEST(AtomicMetricCounterTest, routeActionCount_ForwardAction_PacketsForwardedEqualsOneTrue) {

        AtomicMetricCounterTestDouble counter;

        counter.routeActionCount(makeForwardAction());
        const auto snapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(snapshot.packetsForwarded, 1u);
        EXPECT_EQ(snapshot.packetsDropped, 0u);
        EXPECT_EQ(snapshot.packetsReceived, 1u);
    }

    TEST(AtomicMetricCounterTest, routeActionCount_DropAction_PacketsDroppedEqualsOneTrue) {

        AtomicMetricCounterTestDouble counter;

        counter.routeActionCount(Drop{});
        const auto snapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(snapshot.packetsDropped, 1u);
        EXPECT_EQ(snapshot.packetsForwarded, 0u);
        EXPECT_EQ(snapshot.packetsReceived, 1u);
    }

    TEST(AtomicMetricCounterTest, snapshotAndReset_AfterCounts_ValuesResetToZeroTrue) {

        AtomicMetricCounterTestDouble counter;
        counter.routeActionCount(makeForwardAction());
        counter.routeActionCount(Drop{});
        counter.routeActionCount(Drop{.reason = vpsm::server::domain::DropReason::PARSE});
        counter.routeActionCount(Drop{.reason = vpsm::server::domain::DropReason::AUTH});
        counter.routeActionCount(Drop{.reason = vpsm::server::domain::DropReason::MEMBERSHIP});
        counter.routeActionCount(Drop{.reason = vpsm::server::domain::DropReason::NO_ENDPOINT});

        const auto firstSnapshot = counter.snapshotAndReset();
        const auto secondSnapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(firstSnapshot.packetsForwarded, 1u);
        EXPECT_EQ(firstSnapshot.packetsDropped, 5u);
        EXPECT_EQ(firstSnapshot.packetsReceived, 6u);
        EXPECT_EQ(firstSnapshot.packetsDroppedParse, 1u);
        EXPECT_EQ(firstSnapshot.packetsDroppedAuth, 1u);
        EXPECT_EQ(firstSnapshot.packetsDroppedMembership, 1u);
        EXPECT_EQ(firstSnapshot.packetsDroppedNoEndpoint, 1u);
        EXPECT_EQ(secondSnapshot.packetsForwarded, 0u);
        EXPECT_EQ(secondSnapshot.packetsDropped, 0u);
        EXPECT_EQ(secondSnapshot.packetsReceived, 0u);
        EXPECT_EQ(secondSnapshot.packetsDroppedParse, 0u);
        EXPECT_EQ(secondSnapshot.packetsDroppedAuth, 0u);
        EXPECT_EQ(secondSnapshot.packetsDroppedMembership, 0u);
        EXPECT_EQ(secondSnapshot.packetsDroppedNoEndpoint, 0u);
    }

    TEST(AtomicMetricCounterTest, snapshotWithoutReset_AfterCounts_ValuesPreservedTrue) {

        AtomicMetricCounterTestDouble counter;
        counter.routeActionCount(makeForwardAction());

        const auto firstSnapshot = counter.snapshotWithoutReset();
        const auto secondSnapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(firstSnapshot.packetsForwarded, 1u);
        EXPECT_EQ(secondSnapshot.packetsForwarded, 1u);
        EXPECT_EQ(firstSnapshot.packetsReceived, 1u);
        EXPECT_EQ(secondSnapshot.packetsReceived, 1u);
    }

    TEST(AtomicMetricCounterTest, usersOnlineCount_WhenCalled_UsersOnlineIncrements) {

        AtomicMetricCounterTestDouble counter;

        counter.usersOnlineCount(Drop{});
        counter.usersOnlineCount(makeForwardAction());
        const auto snapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(snapshot.usersOnline, 2u);
    }

}
