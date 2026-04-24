// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#pragma once

#include <cstdint>

/// @brief Board support for the Waveshare ESP32-S3-Touch-LCD-4.3 (4.3"
/// RGB panel variant of the 7" board; same brand playbook — CH422G I/O
/// expander behind it, SD CS routed through the expander).
///
/// The API mirrors bsp::ws::lcd7 so host code that abstracts "which
/// board" through a simple `namespace board = bsp::ws::lcd7;` alias can
/// swap models by changing the alias.
///
/// ⚠ Pin map and CH422G assignments below are PLACEHOLDERS until the
/// 4.3" board schematic is confirmed. Do not flash against this module
/// with real hardware until the marked values are verified.

namespace bsp::ws::lcd43 {

    struct Config {
            bool enableSdCs = false;
            bool enableLcd = false;
            bool enableTouch = false;
            uint8_t initialBacklight = 0;
    };

    bool init(const Config& cfg);

    void setBacklight(uint8_t level);
    void backlightBlink();
    void sdCs(bool asserted);
    void lcdReset(bool asserted);
    void touchReset(bool asserted);

    namespace pins {
        // TODO(waveshare-4.3): confirm these values against the board
        // schematic. The 4.3" board shares Waveshare's ESP32-S3 family
        // layout but the I2C pins have historically differed between
        // models in this product line.
        constexpr int8_t EXPANDER_SDA = 8;
        constexpr int8_t EXPANDER_SCL = 9;
        constexpr int8_t SD_SPI_MOSI = 11;
        constexpr int8_t SD_SPI_SCK = 12;
        constexpr int8_t SD_SPI_MISO = 13;
    }  // namespace pins

    // TODO(waveshare-4.3): the CH422G pin map on the 4.3" board may use
    // the same assignments as the 7" (both share the same expander
    // design) but that's an assumption. Verify against the schematic
    // before trusting these.
    namespace expander_pins {
        constexpr uint8_t TP_RST = 1;
        constexpr uint8_t LCD_BL = 2;
        constexpr uint8_t LCD_RST = 3;
        constexpr uint8_t SD_CS = 4;
        constexpr uint8_t USB_SEL = 5;
    }  // namespace expander_pins

    namespace detail {

        constexpr uint8_t outputPinsMaskFor(const Config& cfg) {
            uint8_t mask = 0;
            if (cfg.enableLcd) {
                mask |= static_cast<uint8_t>(1U << expander_pins::LCD_BL);
                mask |= static_cast<uint8_t>(1U << expander_pins::LCD_RST);
            }
            if (cfg.enableTouch) {
                mask |= static_cast<uint8_t>(1U << expander_pins::TP_RST);
            }
            if (cfg.enableSdCs) {
                mask |= static_cast<uint8_t>(1U << expander_pins::SD_CS);
            }
            return mask;
        }

    }  // namespace detail

}  // namespace bsp::ws::lcd43
