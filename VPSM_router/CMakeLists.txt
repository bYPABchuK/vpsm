cmake_minimum_required(VERSION 3.16)

project(VPSM_router LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

enable_testing()
include(GoogleTest)

add_executable(unit_tests
    src/application/PacketParserV1.cpp
    src/application/RoutingService.cpp
    src/application/UserService.cpp
    src/application/AtomicMetricCounter.cpp
    src/application/MetricService.cpp
    src/adapter/MembershipStore.cpp
    src/adapter/FileMetricsSink.cpp

    test/main.cpp
    test/application/AtomicMetricCounter_test.cpp
    test/application/MetricService_test.cpp
    test/application/PacketParser_test.cpp
    test/application/RoutingService_test.cpp
    test/application/UserService_test.cpp
    test/application/VipAllocator_test.cpp
    test/adapter/FileMetricsSink_test.cpp
    test/adapter/MembershipStore_test.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

gtest_discover_tests(unit_tests)
