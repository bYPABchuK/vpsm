#include "../../src/application/ControlPlane/Endpoints/UiLicenseCardEndpoint.hpp"
#include "../../src/application/ControlPlane/Endpoints/UiMainBodyEndpoint.hpp"
#include "../../src/application/ControlPlane/SessionStore.hpp"
#include "../../src/application/ControlPlane/UiScreenService.hpp"

#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::UiScreenService;
    using vpsm::server::application::endpoints::UiLicensescreenEndpoint;
    using vpsm::server::application::endpoints::UiMainBodyEndpoint;

    struct TempUiFiles {
        std::filesystem::path dir;
        std::filesystem::path mainBody;
        std::filesystem::path licensescreen;

        TempUiFiles() {
            const auto stamp = std::to_string(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count()
            );
            dir = std::filesystem::temp_directory_path() / ("vpsm_ui_test_" + stamp);
            std::filesystem::create_directories(dir);
            mainBody = dir / "main-body.json";
            licensescreen = dir / "license-screen.json";
        }

        ~TempUiFiles() {
            std::error_code ec;
            std::filesystem::remove_all(dir, ec);
        }

        static void write(const std::filesystem::path& path, const std::string& text) {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            out << text;
            out.flush();
        }
    };

    TEST(UiScreensEndpointTest, mainBody_MissingSessionHeaders_Returns401True) {
        TempUiFiles files;
        TempUiFiles::write(files.mainBody, R"({"schemaVersion":1,"screenId":"main-body","components":[]})");

        UiScreenService service(files.mainBody, files.licensescreen);
        vpsm::server::application::SessionStore sessionStore;
        UiMainBodyEndpoint endpoint(service, sessionStore);

        const auto response = endpoint.handle(ControlRequest{.method = "GET", .path = "/ui/screens/main-body"});
        EXPECT_EQ(response.status, 401);
    }

    TEST(UiScreensEndpointTest, mainBody_InvalidSession_Returns403True) {
        TempUiFiles files;
        TempUiFiles::write(files.mainBody, R"({"schemaVersion":1,"screenId":"main-body","components":[]})");

        UiScreenService service(files.mainBody, files.licensescreen);
        vpsm::server::application::SessionStore sessionStore;
        UiMainBodyEndpoint endpoint(service, sessionStore);

        const auto response = endpoint.handle(ControlRequest{
            .method = "GET",
            .path = "/ui/screens/main-body",
            .headers = {{"sessionId", "1"}, {"sessionKey", "2"}},
        });
        EXPECT_EQ(response.status, 403);
    }

    TEST(UiScreensEndpointTest, mainBody_ValidSession_ReturnsScreen200True) {
        TempUiFiles files;
        TempUiFiles::write(files.mainBody, R"({"schemaVersion":1,"screenId":"main-body","components":[]})");

        UiScreenService service(files.mainBody, files.licensescreen);
        vpsm::server::application::SessionStore sessionStore;
        const auto session = sessionStore.createSession(777);
        UiMainBodyEndpoint endpoint(service, sessionStore);

        const auto response = endpoint.handle(ControlRequest{
            .method = "GET",
            .path = "/ui/screens/main-body",
            .headers = {
                {"sessionId", std::to_string(session.sessionId)},
                {"sessionKey", std::to_string(session.sessionKey)},
            },
        });

        EXPECT_EQ(response.status, 200);
        const auto body = boost::json::parse(std::string(response.body.begin(), response.body.end())).as_object();
        EXPECT_TRUE(body.at("ok").as_bool());
        ASSERT_TRUE(body.at("screen").is_object());
        const auto& screen = body.at("screen").as_object();
        EXPECT_EQ(screen.at("schemaVersion").as_int64(), 1);
    }

    TEST(UiScreensEndpointTest, licensescreen_Public200True) {
        TempUiFiles files;
        TempUiFiles::write(files.licensescreen, R"({"title":"Лицензия сервера","text":"Server is licensed under ...","styleRef":"body"})");

        UiScreenService service(files.mainBody, files.licensescreen);
        UiLicensescreenEndpoint endpoint(service);

        const auto response = endpoint.handle(ControlRequest{.method = "GET", .path = "/ui/screens/license-screen"});
        EXPECT_EQ(response.status, 200);

        const auto body = boost::json::parse(std::string(response.body.begin(), response.body.end())).as_object();
        EXPECT_TRUE(body.at("ok").as_bool());
        ASSERT_TRUE(body.at("screen").is_object());
        const auto& screen = body.at("screen").as_object();
        EXPECT_EQ(std::string(screen.at("styleRef").as_string().c_str()), "body");
    }

    TEST(UiScreensEndpointTest, service_CacheWindowAndReloadByMtimeTrue) {
        TempUiFiles files;
        TempUiFiles::write(files.licensescreen, R"({"title":"A","text":"t","styleRef":"body"})");

        UiScreenService service(files.mainBody, files.licensescreen, std::chrono::milliseconds(250));
        const auto first = service.getLicensescreen();
        ASSERT_EQ(first.status, UiScreenService::Status::Ok);
        EXPECT_EQ(std::string(first.payload.at("title").as_string().c_str()), "A");

        TempUiFiles::write(files.licensescreen, R"({"title":"B","text":"t","styleRef":"body"})");

        const auto cached = service.getLicensescreen();
        ASSERT_EQ(cached.status, UiScreenService::Status::Ok);
        EXPECT_EQ(std::string(cached.payload.at("title").as_string().c_str()), "A");

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        // Гарантируем обновление mtime на файловых системах с грубой дискретностью.
        TempUiFiles::write(files.licensescreen, R"({"title":"B","text":"t","styleRef":"body"})");

        const auto reloaded = service.getLicensescreen();
        ASSERT_EQ(reloaded.status, UiScreenService::Status::Ok);
        EXPECT_EQ(std::string(reloaded.payload.at("title").as_string().c_str()), "B");
    }
}
