#pragma once

#include "../IControlEndpoint.hpp"
#include "../SessionStore.hpp"
#include "../UiScreenService.hpp"

namespace vpsm::server::application::endpoints {
    class UiMainBodyEndpoint final : public IControlEndpoint {
    public:
        UiMainBodyEndpoint(UiScreenService& uiScreenService, SessionStore& sessionStore)
            : uiScreenService_(uiScreenService),
              sessionStore_(sessionStore) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        UiScreenService& uiScreenService_;
        SessionStore& sessionStore_;
    };
}
