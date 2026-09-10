#include "../../src/adapter/MembershipStore.hpp"

#include <gtest/gtest.h>

namespace {
    using vpsm::server::adapter::MembershipRegistry;

    TEST(MembershipRegistryTest, allocateVip_noviyPeer_VipIDobavlyaetMembership) {

        MembershipRegistry store;

        const auto vip = store.allocateVip(10, 1001);

        ASSERT_TRUE(vip.has_value());
        EXPECT_TRUE(store.hasPeer(10, 1001));
        EXPECT_EQ(store.resolveVip(10, 1001), vip);
        EXPECT_EQ(store.resolvePeer(10, *vip), std::optional<std::uint64_t>(1001));
    }

    TEST(MembershipRegistryTest, allocateVip_sushestvuyushiyPeer_TotZheVip) {

        MembershipRegistry store;
        const auto first = store.allocateVip(10, 1001);
        ASSERT_TRUE(first.has_value());

        const auto second = store.allocateVip(10, 1001);

        ASSERT_TRUE(second.has_value());
        EXPECT_EQ(*first, *second);
    }

    TEST(MembershipRegistryTest, peerKeepsOneServerWideVipAcrossNetworks) {
        MembershipRegistry store;
        const auto inFirstNetwork = store.allocateVip(10, 1001);
        const auto inSecondNetwork = store.allocateVip(20, 1001);
        const auto anotherPeer = store.allocateVip(20, 1002);

        ASSERT_TRUE(inFirstNetwork.has_value());
        ASSERT_TRUE(inSecondNetwork.has_value());
        ASSERT_TRUE(anotherPeer.has_value());
        EXPECT_EQ(inFirstNetwork, inSecondNetwork);
        EXPECT_NE(inFirstNetwork, anotherPeer);
        EXPECT_EQ(store.resolvePeer(10, *inFirstNetwork), 1001u);
        EXPECT_EQ(store.resolvePeer(20, *inFirstNetwork), 1001u);
    }

    TEST(MembershipRegistryTest, vipIsReleasedOnlyAfterLastMembershipIsRemoved) {
        MembershipRegistry store;
        const auto vip = store.allocateVip(10, 1001);
        ASSERT_EQ(store.allocateVip(20, 1001), vip);
        ASSERT_TRUE(store.releaseVip(10, 1001));
        EXPECT_EQ(store.resolveVip(20, 1001), vip);
        EXPECT_NE(store.allocateVip(10, 1002), vip);
        ASSERT_TRUE(store.releaseVip(20, 1001));
        EXPECT_EQ(store.allocateVip(30, 1003), vip);
    }

    TEST(MembershipRegistryTest, bindPeer_vipUzheZanyatDrugimPeer_False) {

        MembershipRegistry store;
        constexpr std::uint32_t vip = 0x0AF0074Du; // 10.240.7.77
        ASSERT_TRUE(store.bindPeer(7, 1, vip));

        const bool ok = store.bindPeer(7, 2, vip);

        EXPECT_FALSE(ok);
    }

    TEST(MembershipRegistryTest, unbindPeer_neverniyVip_False) {

        MembershipRegistry store;
        constexpr std::uint32_t vip = 0x0AF0074Du; // 10.240.7.77
        ASSERT_TRUE(store.bindPeer(7, 1, vip));

        const bool ok = store.unbindPeer(7, 1, vip + 1);

        EXPECT_FALSE(ok);
    }

    TEST(MembershipRegistryTest, releaseVip_sushestvuyushiyPeer_TrueIOsvobozhdaetVip) {

        MembershipRegistry store;
        const auto vip = store.allocateVip(3, 42);
        ASSERT_TRUE(vip.has_value());

        const bool released = store.releaseVip(3, 42);

        EXPECT_TRUE(released);
        EXPECT_FALSE(store.hasPeer(3, 42));
        EXPECT_FALSE(store.resolveVip(3, 42).has_value());
        EXPECT_FALSE(store.resolvePeer(3, *vip).has_value());
    }

    TEST(MembershipRegistryTest, resolveVip_peerNeNayden_Nullopt) {

        MembershipRegistry store;

        const auto vip = store.resolveVip(1, 999);

        EXPECT_FALSE(vip.has_value());
    }

    TEST(MembershipRegistryTest, resolvePeer_vipNeNayden_Nullopt) {

        MembershipRegistry store;

        const auto peer = store.resolvePeer(1, 999);

        EXPECT_FALSE(peer.has_value());
    }

}
