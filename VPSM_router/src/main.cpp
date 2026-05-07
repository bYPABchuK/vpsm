#include "infrastructure/ControlPlaneBoost.hpp"
#include "infrastructure/DataPlaneBoost.hpp"
#include "adapter/MembershipStore.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

namespace {
    std::atomic_bool gStop{false};

    void onSignal(int) {
        gStop.store(true);
    }

    std::uint16_t readPort(const char* envName, std::uint16_t defaultPort) {
        const char* raw = std::getenv(envName);
        if (raw == nullptr) {
            return defaultPort;
        }

        const auto v = std::strtol(raw, nullptr, 10);
        if (v <= 0 || v > 65535) {
            return defaultPort;
        }
        return static_cast<std::uint16_t>(v);
    }

    std::uint16_t readWorkers(const char* envName, std::uint16_t defaultWorkers) {
        const char* raw = std::getenv(envName);
        if (raw == nullptr) {
            return defaultWorkers;
        }

        const auto v = std::strtol(raw, nullptr, 10);
        if (v <= 0 || v > 256) {
            return defaultWorkers;
        }
        return static_cast<std::uint16_t>(v);
    }

    std::filesystem::path readPath(const char* envName, std::filesystem::path defaultPath) {
        const char* raw = std::getenv(envName);
        if (raw == nullptr || *raw == '\0') {
            return defaultPath;
        }
        return std::filesystem::path(raw);
    }
}

int main() {
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    const auto udpPort = readPort("VPSM_UDP_PORT", 4000);
    const auto httpPort = readPort("VPSM_CONTROL_PORT", 8080);
    const auto workers = readWorkers("VPSM_WORKERS", 2);
    const auto metricsOutput = readPath("VPSM_METRICS_FILE", "vpsm_metrics.prom");
    const auto uiMainBodyPath = readPath("VPSM_UI_MAIN_BODY_FILE", "config/ui/main-body.json");
    const auto uiLicensescreenPath = readPath("VPSM_UI_LICENSE_screen_FILE", "config/ui/license-screen.json");

    auto sharedMembership = std::make_shared<vpsm::server::adapter::MembershipRegistry>();

    vpsm::server::infrastructure::DataPlaneBoost dataPlane(udpPort, workers, metricsOutput, sharedMembership);
    vpsm::server::infrastructure::ControlPlaneBoost controlPlane(
        httpPort,
        workers,
        uiMainBodyPath,
        uiLicensescreenPath,
        sharedMembership
    );

    const auto dataStart = dataPlane.start();
    if (dataStart != 0) {
        std::cerr << "failed to start dataplane, code=" << dataStart << "\n";
        return 1;
    }

    const auto controlStart = controlPlane.start();
    if (controlStart != 0) {
        std::cerr << "failed to start control-plane, code=" << controlStart << "\n";
        dataPlane.stop();
        return 2;
    }

    std::cout << "vpsm_router started: udp=" << udpPort
              << ", control=" << httpPort
              << ", workers=" << workers
              << ", metrics=" << metricsOutput.string()
              << ", ui_main_body=" << uiMainBodyPath.string()
              << ", ui_license_screen=" << uiLicensescreenPath.string()
              << "\n";

    while (!gStop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    controlPlane.stop();
    dataPlane.stop();
    std::cout << "vpsm_router stopped\n";
    return 0;
}
