#include "../../src/application/AtomicMetricCounter.hpp"

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
        void usersOnlineCount(vpsm::server::domain::RouteAction) override {}
    };

    vpsm::server::domain::RouteAction makeForwardAction() {
        PacketOut packet{
            .buf = std::make_shared<std::vector<std::uint8_t>>(),
            .size = 0,
            .type = vpsm::server::domain::UDP,
            .dest = 0,
        };
        return Forward{packet};
    }

    TEST(AtomicMetricCounterTest, routeActionCount_ForwardAction_PacketsForwardedEqualsOneTrue) {

        AtomicMetricCounterTestDouble counter;

        counter.routeActionCount(makeForwardAction());
        const auto snapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(snapshot.packetsForwarded, 1u);
        EXPECT_EQ(snapshot.packetsDropped, 0u);
    }

    TEST(AtomicMetricCounterTest, routeActionCount_DropAction_PacketsDroppedEqualsOneTrue) {

        AtomicMetricCounterTestDouble counter;

        counter.routeActionCount(Drop{});
        const auto snapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(snapshot.packetsDropped, 1u);
        EXPECT_EQ(snapshot.packetsForwarded, 0u);
    }

    TEST(AtomicMetricCounterTest, snapshotAndReset_AfterCounts_ValuesResetToZeroTrue) {

        AtomicMetricCounterTestDouble counter;
        counter.routeActionCount(makeForwardAction());
        counter.routeActionCount(Drop{});

        const auto firstSnapshot = counter.snapshotAndReset();
        const auto secondSnapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(firstSnapshot.packetsForwarded, 1u);
        EXPECT_EQ(firstSnapshot.packetsDropped, 1u);
        EXPECT_EQ(secondSnapshot.packetsForwarded, 0u);
        EXPECT_EQ(secondSnapshot.packetsDropped, 0u);
    }

    TEST(AtomicMetricCounterTest, snapshotWithoutReset_AfterCounts_ValuesPreservedTrue) {

        AtomicMetricCounterTestDouble counter;
        counter.routeActionCount(makeForwardAction());

        const auto firstSnapshot = counter.snapshotWithoutReset();
        const auto secondSnapshot = counter.snapshotWithoutReset();

        EXPECT_EQ(firstSnapshot.packetsForwarded, 1u);
        EXPECT_EQ(secondSnapshot.packetsForwarded, 1u);
    }

}
