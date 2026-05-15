// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#include "board.h"

#include <ungula/bsp/waveshare/common/ch422g_expander.h>
#include <ungula/core/time/time.h>

namespace ungula::bsp::waveshare::lcd7
{

namespace
{

        using bsp::waveshare::common::LEVEL_HIGH;
        using bsp::waveshare::common::LEVEL_LOW;

        constexpr uint32_t BLINK_HALF_PERIOD_MS = 100;

} // namespace

bool init(const Config &cfg)
{
        const uint8_t mask = detail::outputPinsMaskFor(cfg);
        if (!bsp::waveshare::common::ensureInit(pins::EXPANDER_SDA, pins::EXPANDER_SCL, mask)) {
                return false;
        }

        // Default pin states — everything "not active" so nothing spuriously
        // drives attached hardware while the host finishes booting.
        if (cfg.enableLcd) {
                // Backlight starts at the requested initial level, then we
                // run a blink as a boot-sanity signal. Host decides final
                // level via setBacklight() once the screen is ready to draw.
                setBacklight(cfg.initialBacklight != 0 ? LEVEL_HIGH : LEVEL_LOW);
                backlightBlink();
                lcdReset(false); // release = ready
        }
        if (cfg.enableTouch) {
                touchReset(false);
        }
        if (cfg.enableSdCs) {
                sdCs(false); // deasserted — SPI bus will assert per-transfer
        }
        return true;
}

void setBacklight(uint8_t level)
{
        bsp::waveshare::common::writePin(expander_pins::LCD_BL,
                                         level != 0 ? LEVEL_HIGH : LEVEL_LOW);
}

void backlightBlink()
{
        setBacklight(0);
        ungula::core::time::delay(BLINK_HALF_PERIOD_MS);
        setBacklight(1);
        ungula::core::time::delay(BLINK_HALF_PERIOD_MS);
}

void sdCs(bool asserted)
{
        // Active-low — asserted = drive the line LOW so the SD card
        // listens to the SPI bus.
        bsp::waveshare::common::writePin(expander_pins::SD_CS, asserted ? LEVEL_LOW : LEVEL_HIGH);
}

void lcdReset(bool asserted)
{
        bsp::waveshare::common::writePin(expander_pins::LCD_RST, asserted ? LEVEL_LOW : LEVEL_HIGH);
}

void touchReset(bool asserted)
{
        bsp::waveshare::common::writePin(expander_pins::TP_RST, asserted ? LEVEL_LOW : LEVEL_HIGH);
}

} // namespace ungula::bsp::waveshare::lcd7
