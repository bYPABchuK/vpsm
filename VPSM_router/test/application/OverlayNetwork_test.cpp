#include "../../src/adapter/MembershipStore.hpp"
#include "../../src/domain/type/overlayNetwork.hpp"
#include "../../src/repository/InMemoryVNetworkRepository.hpp"

#include <gtest/gtest.h>

namespace {
    using vpsm::server::adapter::MembershipRegistry;
    using vpsm::server::repository::InMemoryVNetworkRepository;

    TEST(OverlayNetworkTest, repositoryAssignsSharedServerOverlayAndReusesDeletedSlot) {
        InMemoryVNetworkRepository repository;
        const auto firstId = repository.createNetwork(1, "first", "pw");
        const auto secondId = repository.createNetwork(1, "second", "pw");
        ASSERT_EQ(firstId, 1u);
        ASSERT_EQ(secondId, 2u);

        const auto first = repository.getNetwork(*firstId);
        const auto second = repository.getNetwork(*secondId);
        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(second.has_value());
        EXPECT_EQ(first->networkAddress, 0x0AF00000u);  // 10.240.0.0
        EXPECT_EQ(second->networkAddress, 0x0AF00000u);
        EXPECT_EQ(first->prefixLength, 16u);
        EXPECT_EQ(first->mtu, 1400u);

        ASSERT_TRUE(repository.deleteNetwork(*firstId));
        const auto reusedId = repository.createNetwork(1, "reused", "pw");
        ASSERT_EQ(reusedId, 1u);
        ASSERT_TRUE(repository.getNetwork(*reusedId).has_value());
        EXPECT_EQ(repository.getNetwork(*reusedId)->networkAddress, 0x0AF00000u);
    }

    TEST(OverlayNetworkTest, membershipAllocatesUsableFullIpv4Addresses) {
        MembershipRegistry membership;
        const auto first = membership.allocateVip(7, 100);
        const auto second = membership.allocateVip(7, 200);
        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(second.has_value());
        EXPECT_EQ(*first, 0x0AF00001u);  // 10.240.0.1
        EXPECT_EQ(*second, 0x0AF00002u); // 10.240.0.2
        EXPECT_FALSE(membership.bindPeer(7, 300, 0x0AF00000u)); // network
        EXPECT_FALSE(membership.bindPeer(7, 300, 0x0AF0FFFFu)); // broadcast
        EXPECT_FALSE(membership.bindPeer(7, 300, 0x0AF10001u)); // outside overlay
    }

    TEST(OverlayNetworkTest, removedNetworkDoesNotLeakMembershipIntoReusedSlot) {
        MembershipRegistry membership;
        ASSERT_EQ(membership.allocateVip(1, 100), 0x0AF00001u);
        ASSERT_TRUE(membership.hasPeer(1, 100));
        ASSERT_TRUE(membership.removeNetwork(1));
        EXPECT_FALSE(membership.hasPeer(1, 100));
        EXPECT_EQ(membership.allocateVip(1, 200), 0x0AF00001u);
        EXPECT_FALSE(membership.hasPeer(1, 100));
        EXPECT_TRUE(membership.hasPeer(1, 200));
    }

    TEST(OverlayNetworkTest, ipv4FormattingIsStable) {
        EXPECT_EQ(vpsm::server::domain::ipv4ToString(0x0AF00A02u), "10.240.10.2");
        EXPECT_FALSE(vpsm::server::domain::overlayConfigForNetworkId(0).has_value());
        EXPECT_FALSE(vpsm::server::domain::overlayConfigForNetworkId(256).has_value());
    }

    TEST(OverlayNetworkTest, repositoryRejectsDuplicateNetworkNameAtomically) {
        InMemoryVNetworkRepository repository;
        ASSERT_TRUE(repository.createNetwork(1, "unique", "pw").has_value());
        EXPECT_FALSE(repository.createNetwork(2, "unique", "other").has_value());
        EXPECT_EQ(repository.listNetworks().size(), 1u);
    }
}