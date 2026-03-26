cmake_minimum_required(VERSION 3.16)

project(VPSM_router LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)
find_package(Threads REQUIRED)

enable_testing()
include(GoogleTest)

add_executable(unit_tests
    src/application/DataPlane/PacketParserV1.cpp
    src/application/DataPlane/PacketParserV2.cpp
    src/application/DataPlane/AuthServiceV2.cpp
    src/application/DataPlane/RoutingService.cpp
    src/application/DataPlane/UserService.cpp
    src/application/DataPlane/AtomicMetricCounter.cpp
    src/application/DataPlane/MetricService.cpp
    src/application/ControlPlane/ControlRouter.cpp
    src/application/ControlPlane/HealthEndpoint.cpp
    src/application/ControlPlane/JsonRequestDecoder.cpp
    src/application/ControlPlane/JsonResponseEncoder.cpp
    src/application/ControlPlane/DispatchEchoEndpoint.cpp
    src/application/ControlPlane/UserServiceEndpoint.cpp
    src/adapter/MembershipStore.cpp
    src/adapter/FileMetricsSink.cpp
    src/adapter/boost/ControlHttpBoost.cpp
    src/infrastructure/ControlPlaneBoost.cpp
    src/repository/InMemoryPeerRepository.cpp
    src/repository/InMemoryVNetworkRepository.cpp

    test/main.cpp
    test/application/AtomicMetricCounter_test.cpp
    test/application/MetricService_test.cpp
    test/application/PacketParser_test.cpp
    test/application/PacketParserV2_test.cpp
    test/application/AuthServiceV2_test.cpp
    test/application/ControlRouter_test.cpp
    test/application/DispatchEchoEndpoint_test.cpp
    test/application/UserServiceEndpoint_test.cpp
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
    Boost::json
    Threads::Threads
)

gtest_discover_tests(unit_tests)

add_executable(vpsm_router
    src/main.cpp
    src/application/DataPlane/PacketParserV1.cpp
    src/application/DataPlane/PacketParserV2.cpp
    src/application/DataPlane/AuthServiceV2.cpp
    src/application/DataPlane/RoutingService.cpp
    src/application/DataPlane/UserService.cpp
    src/application/DataPlane/AtomicMetricCounter.cpp
    src/application/DataPlane/MetricService.cpp
    src/application/ControlPlane/ControlRouter.cpp
    src/application/ControlPlane/HealthEndpoint.cpp
    src/application/ControlPlane/JsonRequestDecoder.cpp
    src/application/ControlPlane/JsonResponseEncoder.cpp
    src/application/ControlPlane/DispatchEchoEndpoint.cpp
    src/application/ControlPlane/UserServiceEndpoint.cpp
    src/adapter/MembershipStore.cpp
    src/adapter/FileMetricsSink.cpp
    src/adapter/boost/ControlHttpBoost.cpp
    src/adapter/boost/UdpGatewayBoost.cpp
    src/adapter/boost/TcpManagerBoost.cpp
    src/infrastructure/ControlPlaneBoost.cpp
    src/infrastructure/DataPlaneBoost.cpp
    src/repository/InMemoryPeerRepository.cpp
    src/repository/InMemoryVNetworkRepository.cpp
)

target_include_directories(vpsm_router PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vpsm_router PRIVATE
    Boost::json
    Threads::Threads
)
