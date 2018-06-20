/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include <stdint.h>
#include <functional>

namespace libgvnc {
    struct KeyEvent {
        bool Down;
        uint32_t KeySym;
    };

    using KeyboardCallback = std::function<void(const KeyEvent&) >;

    class Keyboard {
    public:
        void SendEvent(struct KeyEvent &event) {
            callback_(event);
        }

        void SetCallback(const KeyboardCallback &callback) {
            callback_ = callback;
        }

    private:
        KeyboardCallback callback_;
    };
}
