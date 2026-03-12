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

    TEST(MembershipRegistryTest, bindPeer_vipUzheZanyatDrugimPeer_False) {

        MembershipRegistry store;
        ASSERT_TRUE(store.bindPeer(7, 1, 77));

        const bool ok = store.bindPeer(7, 2, 77);

        EXPECT_FALSE(ok);
    }

    TEST(MembershipRegistryTest, unbindPeer_neverniyVip_False) {

        MembershipRegistry store;
        ASSERT_TRUE(store.bindPeer(7, 1, 77));

        const bool ok = store.unbindPeer(7, 1, 78);

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
