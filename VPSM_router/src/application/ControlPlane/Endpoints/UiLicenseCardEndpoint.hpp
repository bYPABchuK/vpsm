#pragma once

#include "../IControlEndpoint.hpp"
#include "../UiScreenService.hpp"

namespace vpsm::server::application::endpoints {
    class UiLicensescreenEndpoint final : public IControlEndpoint {
    public:
        explicit UiLicensescreenEndpoint(UiScreenService& uiScreenService)
            : uiScreenService_(uiScreenService) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        UiScreenService& uiScreenService_;
    };
}
