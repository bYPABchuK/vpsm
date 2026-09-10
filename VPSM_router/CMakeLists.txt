cmake_minimum_required(VERSION 3.16)

project(VPSM_router LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

include(CTest)

find_package(Boost REQUIRED COMPONENTS json)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

set(VPSM_ROUTER_CORE_SOURCES
    src/application/DataPlane/PacketParserV1.cpp
    src/application/DataPlane/PacketParserV2.cpp
    src/application/DataPlane/AuthServiceV2.cpp
    src/application/DataPlane/PacketCryptoV2.cpp
    src/application/DataPlane/RoutingService.cpp
    src/application/DataPlane/LogRoutingService.cpp
    src/application/DataPlane/UserService.cpp
    src/application/DataPlane/AtomicMetricCounter.cpp
    src/application/DataPlane/MetricService.cpp
    src/application/ControlPlane/ControlRouter.cpp
    src/application/ControlPlane/HealthEndpoint.cpp
    src/application/ControlPlane/JsonRequestDecoder.cpp
    src/application/ControlPlane/JsonResponseEncoder.cpp
    src/application/ControlPlane/JsonSupport.cpp
    src/application/ControlPlane/JsonCreatePeerDecoder.cpp
    src/application/ControlPlane/JsonCreatePeerResponseEncoder.cpp
    src/application/ControlPlane/JsonCreateNetworkDecoder.cpp
    src/application/ControlPlane/JsonCreateNetworkAuthResponseEncoder.cpp
    src/application/ControlPlane/JsonCreateNetworkResponseEncoder.cpp
    src/application/ControlPlane/JsonNetworkUserAddDecoder.cpp
    src/application/ControlPlane/JsonNetworkUserAddResponseEncoder.cpp
    src/application/ControlPlane/JsonUserNetworkListResponseEncoder.cpp
    src/application/ControlPlane/JsonUserNetworkPeersListResponseEncoder.cpp
    src/application/ControlPlane/JsonNetworkPeersListResponseEncoder.cpp
    src/application/ControlPlane/JsonJoinNetworkDecoder.cpp
    src/application/ControlPlane/JsonJoinNetworkResponseEncoder.cpp
    src/application/ControlPlane/JsonLeaveNetworkDecoder.cpp
    src/application/ControlPlane/JsonLeaveNetworkResponseEncoder.cpp
    src/application/ControlPlane/JsonLoginDecoder.cpp
    src/application/ControlPlane/JsonLoginResponseEncoder.cpp
    src/application/ControlPlane/SessionStore.cpp
    src/application/ControlPlane/UiScreenService.cpp
    src/application/ControlPlane/Endpoints/CreatePeerEndpoint.cpp
    src/application/ControlPlane/Endpoints/CreateNetworkEndpoint.cpp
    src/application/ControlPlane/Endpoints/DeleteNetworkEndpoint.cpp
    src/application/ControlPlane/Endpoints/JoinNetworkEndpoint.cpp
    src/application/ControlPlane/Endpoints/JoinNetworkByNameEndpoint.cpp
    src/application/ControlPlane/Endpoints/LeaveNetworkEndpoint.cpp
    src/application/ControlPlane/Endpoints/LoginEndpoint.cpp
    src/application/ControlPlane/Endpoints/LogoutEndpoint.cpp
    src/application/ControlPlane/Endpoints/NetworkUserAddEndpoint.cpp
    src/application/ControlPlane/Endpoints/UiMainBodyEndpoint.cpp
    src/application/ControlPlane/Endpoints/UiLicenseCardEndpoint.cpp
    src/application/ControlPlane/Endpoints/UserNetworkListEndpoint.cpp
    src/application/ControlPlane/Endpoints/UserNetworkPeersListEndpoint.cpp
    src/application/ControlPlane/Endpoints/NetworkPeersListEndpoint.cpp
    src/application/DTO/JsonDtoDecoder.cpp
    src/adapter/MembershipStore.cpp
    src/adapter/PeerEndpointRegistry.cpp
    src/adapter/FileMetricsSink.cpp
    src/adapter/boost/ControlHttpBoost.cpp
    src/adapter/boost/TcpManagerBoost.cpp
    src/adapter/boost/UdpGatewayBoost.cpp
    src/infrastructure/ControlPlaneBoost.cpp
    src/infrastructure/DataPlaneBoost.cpp
    src/repository/InMemoryPeerRepository.cpp
    src/repository/InMemoryVNetworkRepository.cpp
)

add_library(vpsm_router_core STATIC ${VPSM_ROUTER_CORE_SOURCES})
add_library(vpsm::router_core ALIAS vpsm_router_core)

target_compile_features(vpsm_router_core PUBLIC cxx_std_17)
set_target_properties(vpsm_router_core PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(vpsm_router_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vpsm_router_core
    PUBLIC
        Boost::json
        Threads::Threads
        OpenSSL::Crypto
)

add_executable(vpsm_router src/main.cpp)
target_link_libraries(vpsm_router PRIVATE vpsm::router_core)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(vpsm_router_tests
        test/main.cpp
        test/application/AtomicMetricCounter_test.cpp
        test/application/MetricService_test.cpp
        test/application/LogRoutingService_test.cpp
        test/application/PacketParser_test.cpp
        test/application/PacketParserV2_test.cpp
        test/application/AuthServiceV2_test.cpp
        test/application/ControlRouter_test.cpp
        test/application/UiScreensEndpoint_test.cpp
        test/application/UserServiceEndpoint_test.cpp
        test/application/RoutingService_test.cpp
        test/application/UserService_test.cpp
        test/application/VipAllocator_test.cpp
        test/application/OverlayNetwork_test.cpp
        test/application/SharedMembershipIntegration_test.cpp
        test/application/SecureRoutingIntegration_test.cpp
        test/adapter/FileMetricsSink_test.cpp
        test/adapter/MembershipStore_test.cpp
    )

    target_link_libraries(vpsm_router_tests
        PRIVATE
            vpsm::router_core
            GTest::gtest
    )

    gtest_discover_tests(vpsm_router_tests)
endif()
