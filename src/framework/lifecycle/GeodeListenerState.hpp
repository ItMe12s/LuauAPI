#pragma once

#include <Geode/Geode.hpp>

namespace luax {
    struct GeodeListenerState {
        geode::ListenerHandle handle;
        bool connected = true;

        ~GeodeListenerState() {
            disconnect();
        }

        void disconnect() {
            if (!connected) return;
            handle = geode::ListenerHandle();
            connected = false;
        }
    };
} // namespace luax
