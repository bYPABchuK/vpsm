cmake_minimum_required(VERSION 3.16)

project(VPSM_client_qml VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Core Network)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

option(VPSM_BUILD_GUI "Build the Qt Quick GUI client" ON)
if(VPSM_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick)
endif()

add_library(vpsm_tunnel_core STATIC
    src/protocol/PacketCodecV2.cpp
    src/protocol/PacketCodecV2.hpp
    src/protocol/ReplayWindow.cpp
    src/protocol/ReplayWindow.hpp
    src/protocol/Ipv4PacketParser.cpp
    src/protocol/Ipv4PacketParser.hpp
    src/application/TunnelController.cpp
    src/application/TunnelController.hpp
    src/application/ports/ITunDevice.hpp
    src/application/ports/ITunnelTransport.hpp
    src/application/ports/INetworkConfigurator.hpp
    src/application/ports/IControlPlaneClient.hpp
    src/application/ports/IPlatformServices.hpp
    src/domain/TunnelTypes.hpp
    src/presentation/TunnelViewModel.cpp
    src/presentation/TunnelViewModel.hpp
    src/infrastructure/UdpTunnelTransport.cpp
    src/infrastructure/UdpTunnelTransport.hpp
    src/infrastructure/HttpControlPlaneClient.cpp
    src/infrastructure/HttpControlPlaneClient.hpp
    src/presentation/AppViewModel.cpp
    src/presentation/AppViewModel.hpp
)
add_library(vpsm::tunnel_core ALIAS vpsm_tunnel_core)

target_include_directories(vpsm_tunnel_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(vpsm_tunnel_core PUBLIC Qt6::Core Qt6::Network OpenSSL::Crypto)

set(VPSM_PLATFORM_BACKEND "AUTO" CACHE STRING "Platform backend: AUTO, LINUX, STUB")
set_property(CACHE VPSM_PLATFORM_BACKEND PROPERTY STRINGS AUTO LINUX STUB)

if(VPSM_PLATFORM_BACKEND STREQUAL "AUTO")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(VPSM_PLATFORM_BACKEND "LINUX")
    else()
        set(VPSM_PLATFORM_BACKEND "STUB")
    endif()
endif()

if(VPSM_PLATFORM_BACKEND STREQUAL "LINUX")
    add_library(vpsm_platform STATIC
        src/platform/linux/LinuxTunDevice.cpp
        src/platform/linux/LinuxTunDevice.hpp
        src/platform/linux/LinuxNetworkConfigurator.cpp
        src/platform/linux/LinuxNetworkConfigurator.hpp
        src/platform/linux/LinuxPlatformServices.cpp
        src/platform/linux/LinuxPlatformServices.hpp
        src/platform/linux/LinuxPrivilegedHelperClient.cpp
        src/platform/linux/LinuxPrivilegedHelperClient.hpp
        src/platform/linux/PrivilegedHelperProtocol.cpp
        src/platform/linux/PrivilegedHelperProtocol.hpp
        src/platform/PlatformServicesFactoryLinux.cpp
        src/platform/PlatformServicesFactory.hpp
    )
    target_compile_definitions(vpsm_platform PUBLIC VPSM_PLATFORM_LINUX=1)
    add_executable(vpsm_net_helper
        src/net_helper_main.cpp
        src/platform/linux/PrivilegedHelperProtocol.cpp
        src/platform/linux/PrivilegedHelperProtocol.hpp
    )
    target_include_directories(vpsm_net_helper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(vpsm_net_helper PRIVATE cxx_std_17)
elseif(VPSM_PLATFORM_BACKEND STREQUAL "STUB")
    add_library(vpsm_platform STATIC
        src/platform/stub/StubPlatformServices.cpp
        src/platform/stub/StubPlatformServices.hpp
        src/platform/PlatformServicesFactoryStub.cpp
        src/platform/PlatformServicesFactory.hpp
    )
else()
    message(FATAL_ERROR "Unsupported VPSM_PLATFORM_BACKEND=${VPSM_PLATFORM_BACKEND}")
endif()

add_library(vpsm::platform ALIAS vpsm_platform)
target_include_directories(vpsm_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(vpsm_platform PUBLIC vpsm::tunnel_core Qt6::Core Qt6::Network)

add_executable(vpsm_tunnel_headless src/headless_main.cpp)
target_link_libraries(vpsm_tunnel_headless PRIVATE vpsm::tunnel_core vpsm::platform)

if(VPSM_BUILD_GUI)
    add_executable(vpsm_client src/gui_main.cpp)
    qt_add_qml_module(vpsm_client
        URI VPSM.Client
        VERSION 1.0
        RESOURCE_PREFIX /qt/qml
        QML_FILES
            Main.qml
    )
    target_link_libraries(vpsm_client PRIVATE
        vpsm::tunnel_core
        vpsm::platform
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    )
endif()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    function(vpsm_add_qtest target source)
        add_executable(${target} ${source} tests/TestFakes.hpp)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        target_link_libraries(${target} PRIVATE vpsm::tunnel_core Qt6::Test)
        add_test(NAME ${target} COMMAND ${target})
    endfunction()
    vpsm_add_qtest(vpsm_test_packetcodec tests/test_packetcodec.cpp)
    vpsm_add_qtest(vpsm_test_tunnelcontroller tests/test_tunnelcontroller.cpp)
    vpsm_add_qtest(vpsm_test_tunnelviewmodel tests/test_tunnelviewmodel.cpp)
    vpsm_add_qtest(vpsm_test_httpcontrolplane tests/test_httpcontrolplane.cpp)
    vpsm_add_qtest(vpsm_test_appviewmodel tests/test_appviewmodel.cpp)
    if(VPSM_PLATFORM_BACKEND STREQUAL "LINUX")
        add_executable(vpsm_test_linuxplatform tests/test_linuxplatform.cpp)
        target_link_libraries(vpsm_test_linuxplatform PRIVATE vpsm::tunnel_core vpsm::platform Qt6::Test)
        add_test(NAME vpsm_test_linuxplatform COMMAND vpsm_test_linuxplatform)
    endif()
    if(VPSM_BUILD_GUI)
        add_test(NAME vpsm_test_qml_smoke COMMAND vpsm_client --smoke-test -platform offscreen)
        set_tests_properties(vpsm_test_qml_smoke PROPERTIES
            ENVIRONMENT "QSG_RHI_BACKEND=software"
            TIMEOUT 10
        )
    endif()
endif()

install(TARGETS vpsm_tunnel_headless RUNTIME DESTINATION bin)
if(VPSM_PLATFORM_BACKEND STREQUAL "LINUX")
    install(TARGETS vpsm_net_helper RUNTIME DESTINATION libexec/vpsm)
endif()
if(VPSM_BUILD_GUI)
    install(TARGETS vpsm_client RUNTIME DESTINATION bin)
endif()