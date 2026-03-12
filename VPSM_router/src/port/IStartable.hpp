#pragma once

namespace vpsm::server::port {
    class IStartable {
        public:
        virtual int start() = 0;
        virtual int stop() = 0;
    };
}