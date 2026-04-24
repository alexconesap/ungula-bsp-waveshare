// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#include "ch422g_expander.h"

#if defined(ESP_PLATFORM)

#include <esp_io_expander.hpp>

namespace bsp::waveshare::common {

    namespace {

        // Single-instance state. The BSP is the only owner of the chip on
        // any given project; multiple board / project consumers funnel
        // through ensureInit() + writePin().
        esp_expander::CH422G* s_expander = nullptr;
        uint8_t s_outputPinsMask = 0;

    }  // namespace

    bool ensureInit(int8_t sdaPin, int8_t sclPin, uint8_t outputPinsMask) {
        if (s_expander == nullptr) {
            s_expander = new esp_expander::CH422G(sclPin, sdaPin,
                                                   ESP_IO_EXPANDER_I2C_CH422G_ADDRESS);
            s_expander->init();
            s_expander->begin();
        }

        // OR-merge the requested output pins so a second consumer can add
        // its own pins without disturbing the first one's config.
        const uint8_t newPins = outputPinsMask & ~s_outputPinsMask;
        if (newPins != 0) {
            s_expander->multiPinMode(newPins, OUTPUT);
            s_outputPinsMask |= newPins;
        }
        return true;
    }

    void writePin(uint8_t pinNumber, uint8_t level) {
        if (s_expander == nullptr) {
            return;
        }
        s_expander->digitalWrite(pinNumber, level);
    }

    bool isReady() {
        return s_expander != nullptr;
    }

}  // namespace bsp::waveshare::common

#else   // ESP_PLATFORM

// Off-target stubs so unit tests / host-side builds compile.
namespace bsp::waveshare::common {
    bool ensureInit(int8_t, int8_t, uint8_t) {
        return false;
    }
    void writePin(uint8_t, uint8_t) {}
    bool isReady() {
        return false;
    }
}  // namespace bsp::waveshare::common

#endif  // ESP_PLATFORM
