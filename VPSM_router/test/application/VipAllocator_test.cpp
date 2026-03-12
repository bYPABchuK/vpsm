#include "../../src/application/VipAllocator.hpp"

#include <gtest/gtest.h>

namespace {
    using vpsm::server::application::VipAllocator;

    TEST(VipAllocatorTest, allocate_pustoyPul_MinimalniyVip) {

        VipAllocator allocator(10, 20);

        const auto vip = allocator.allocate();

        ASSERT_TRUE(vip.has_value());
        EXPECT_EQ(*vip, 10u);
    }

    TEST(VipAllocatorTest, allocate_posledovatelnieVyzovi_VipPoPoryadku) {

        VipAllocator allocator(1, 10);

        const auto vip1 = allocator.allocate();
        const auto vip2 = allocator.allocate();
        const auto vip3 = allocator.allocate();

        ASSERT_TRUE(vip1.has_value());
        ASSERT_TRUE(vip2.has_value());
        ASSERT_TRUE(vip3.has_value());
        EXPECT_EQ(*vip1, 1u);
        EXPECT_EQ(*vip2, 2u);
        EXPECT_EQ(*vip3, 3u);
    }

    TEST(VipAllocatorTest, release_osvobozhdenniyVip_sleduyushiyAllocateMinimalniyIzKuchi) {

        VipAllocator allocator(1, 10);
        const auto vip1 = allocator.allocate();
        const auto vip2 = allocator.allocate();
        const auto vip3 = allocator.allocate();
        ASSERT_TRUE(vip1.has_value());
        ASSERT_TRUE(vip2.has_value());
        ASSERT_TRUE(vip3.has_value());
        ASSERT_TRUE(allocator.release(*vip2));

        const auto recycled = allocator.allocate();

        ASSERT_TRUE(recycled.has_value());
        EXPECT_EQ(*recycled, *vip2);
    }

    TEST(VipAllocatorTest, reserve_vipNizheMinimuma_False) {

        VipAllocator allocator(10, 20);

        const bool reserved = allocator.reserve(9);

        EXPECT_FALSE(reserved);
    }

    TEST(VipAllocatorTest, allocate_diapazonIscherpan_Nullopt) {

        VipAllocator allocator(5, 5);
        ASSERT_TRUE(allocator.allocate().has_value());

        const auto vip = allocator.allocate();

        EXPECT_FALSE(vip.has_value());
    }

    TEST(VipAllocatorTest, release_vipNeVydelyalsya_False) {

        VipAllocator allocator(1, 10);

        const bool released = allocator.release(4);

        EXPECT_FALSE(released);
    }

}
