cmake_minimum_required(VERSION 3.16)

project(VPSM_qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Test)

set(PROJECT_SOURCES
        main.cpp
        src/presentation/ui/mainwindow.cpp
        src/presentation/ui/mainwindow.h
        src/presentation/ui/mainwindow.ui
        src/presentation/viewmodel/MainViewModel.cpp
        src/presentation/viewmodel/MainViewModel.hpp
        src/application/services/DataPlaneService.cpp
        src/application/services/DataPlaneService.hpp
        src/application/logging/PacketLogWriter.cpp
        src/application/logging/PacketLogWriter.hpp
        src/application/services/ControlPlaneService.cpp
        src/application/services/ControlPlaneService.hpp
        src/domain/model/PacketRecord.hpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VPSM_qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET VPSM_qt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(VPSM_qt SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(VPSM_qt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(VPSM_qt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

target_include_directories(VPSM_qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/model
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.VPSM_qt)
endif()
set_target_properties(VPSM_qt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS VPSM_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VPSM_qt)
endif()

enable_testing()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VPSM_qt_test_mainviewmodel
        tests/test_mainviewmodel.cpp
        src/presentation/viewmodel/MainViewModel.cpp
        src/presentation/viewmodel/MainViewModel.hpp
        src/application/services/DataPlaneService.cpp
        src/application/services/DataPlaneService.hpp
        src/application/logging/PacketLogWriter.cpp
        src/application/logging/PacketLogWriter.hpp
        src/application/services/ControlPlaneService.cpp
        src/application/services/ControlPlaneService.hpp
        src/domain/model/PacketRecord.hpp
    )

    qt_add_executable(VPSM_qt_test_packetlogwriter
        tests/test_packetlogwriter.cpp
        src/application/logging/PacketLogWriter.cpp
        src/application/logging/PacketLogWriter.hpp
        src/domain/model/PacketRecord.hpp
    )

    qt_add_executable(VPSM_qt_test_controlplaneservice
        tests/test_controlplaneservice.cpp
        src/application/services/ControlPlaneService.cpp
        src/application/services/ControlPlaneService.hpp
    )
else()
    add_executable(VPSM_qt_test_mainviewmodel
        tests/test_mainviewmodel.cpp
        src/presentation/viewmodel/MainViewModel.cpp
        src/presentation/viewmodel/MainViewModel.hpp
        src/application/services/DataPlaneService.cpp
        src/application/services/DataPlaneService.hpp
        src/application/logging/PacketLogWriter.cpp
        src/application/logging/PacketLogWriter.hpp
        src/application/services/ControlPlaneService.cpp
        src/application/services/ControlPlaneService.hpp
        src/domain/model/PacketRecord.hpp
    )

    add_executable(VPSM_qt_test_packetlogwriter
        tests/test_packetlogwriter.cpp
        src/application/logging/PacketLogWriter.cpp
        src/application/logging/PacketLogWriter.hpp
        src/domain/model/PacketRecord.hpp
    )

    add_executable(VPSM_qt_test_controlplaneservice
        tests/test_controlplaneservice.cpp
        src/application/services/ControlPlaneService.cpp
        src/application/services/ControlPlaneService.hpp
    )
endif()

target_link_libraries(VPSM_qt_test_mainviewmodel PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(VPSM_qt_test_mainviewmodel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/model
)

target_link_libraries(VPSM_qt_test_packetlogwriter PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(VPSM_qt_test_packetlogwriter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/model
)

target_link_libraries(VPSM_qt_test_controlplaneservice PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(VPSM_qt_test_controlplaneservice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/model
)

add_test(NAME VPSM_qt_test_mainviewmodel COMMAND VPSM_qt_test_mainviewmodel)
add_test(NAME VPSM_qt_test_packetlogwriter COMMAND VPSM_qt_test_packetlogwriter)
add_test(NAME VPSM_qt_test_controlplaneservice COMMAND VPSM_qt_test_controlplaneservice)

# For test targets created with qt_add_executable (without MANUAL_FINALIZATION),
# finalization is performed automatically by Qt/CMake.
