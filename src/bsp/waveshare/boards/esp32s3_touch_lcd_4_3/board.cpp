// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/// @brief Waveshare ESP32-S3-Touch-LCD-4.3 board module.
///
/// Implementation is currently identical to the 7" board because both
/// models share Waveshare's standard CH422G playbook. Kept as a separate
/// translation unit (rather than a header-only alias) so that once the
/// 4.3" schematic is confirmed, any model-specific wiring (different
/// boot sequence, different blink timing, different default backlight
/// level, different CH422G pin assignment…) can be implemented here
/// without touching the 7" code.
///
/// TODO(waveshare-4.3): confirm this mirrors the real board.

#include "board.h"

#include <bsp/waveshare/common/ch422g_expander.h>
#include <time/time_control.h>

namespace bsp::ws::lcd43 {

    namespace {

        using bsp::waveshare::common::LEVEL_HIGH;
        using bsp::waveshare::common::LEVEL_LOW;

        constexpr uint32_t BLINK_HALF_PERIOD_MS = 100;

    }  // namespace

    bool init(const Config& cfg) {
        const uint8_t mask = detail::outputPinsMaskFor(cfg);
        if (!bsp::waveshare::common::ensureInit(pins::EXPANDER_SDA, pins::EXPANDER_SCL, mask)) {
            return false;
        }

        if (cfg.enableLcd) {
            setBacklight(cfg.initialBacklight != 0 ? LEVEL_HIGH : LEVEL_LOW);
            backlightBlink();
            lcdReset(false);
        }
        if (cfg.enableTouch) {
            touchReset(false);
        }
        if (cfg.enableSdCs) {
            sdCs(false);
        }
        return true;
    }

    void setBacklight(uint8_t level) {
        bsp::waveshare::common::writePin(expander_pins::LCD_BL,
                                         level != 0 ? LEVEL_HIGH : LEVEL_LOW);
    }

    void backlightBlink() {
        setBacklight(0);
        ungula::TimeControl::delay(BLINK_HALF_PERIOD_MS);
        setBacklight(1);
        ungula::TimeControl::delay(BLINK_HALF_PERIOD_MS);
    }

    void sdCs(bool asserted) {
        bsp::waveshare::common::writePin(expander_pins::SD_CS, asserted ? LEVEL_LOW : LEVEL_HIGH);
    }

    void lcdReset(bool asserted) {
        bsp::waveshare::common::writePin(expander_pins::LCD_RST, asserted ? LEVEL_LOW : LEVEL_HIGH);
    }

    void touchReset(bool asserted) {
        bsp::waveshare::common::writePin(expander_pins::TP_RST, asserted ? LEVEL_LOW : LEVEL_HIGH);
    }

}  // namespace bsp::ws::lcd43
