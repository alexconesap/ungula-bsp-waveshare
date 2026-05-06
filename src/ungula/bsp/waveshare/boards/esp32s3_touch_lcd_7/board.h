// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#pragma once

#include <cstdint>

/// @brief Board support for the Waveshare ESP32-S3-Touch-LCD-7 (7" RGB
/// panel with GT911 touch, CH422G I/O expander, microSD over SPI).
///
/// The expander lives behind the BSP's shared CH422G owner — callers use
/// the purpose-named helpers here (setBacklight, sdCs, lcdReset,
/// touchReset) and never touch the chip directly. Pin numbers are PCB
/// traces so they're const data at the bottom of this file.
///
/// Typical use:
///
///   #ifdef BOARD_WAVESHARE_S3_LCD7
///       bsp::ws::lcd7::init({
///           .enableSdCs = true,
///           .enableLcd  = true,   // pair with EMBEDDED_UI on the host
///           .initialBacklight = 0,
///       });
///   #endif
///
/// init() is idempotent — safe to call from multiple subsystems during
/// boot (e.g. SD sink and UI both calling it). The first call brings the
/// expander up; later calls just OR their required pins into its config.

namespace ungula::bsp::waveshare::lcd7 {

    struct Config {
            /// Route SD chip-select through the expander pin. Set when the
            /// firmware uses the onboard microSD slot.
            bool enableSdCs = false;

            /// Configure the LCD reset and backlight expander pins.
            /// Independent of enableTouch because some projects want the
            /// panel lit (e.g. logo screen) without a touch stack.
            bool enableLcd = false;

            /// Configure the touch controller reset expander pin.
            bool enableTouch = false;

            /// Initial backlight level (0 = off, 1 = on). Only applied
            /// when enableLcd is true. A visual boot blink is still
            /// performed regardless, so the operator sees life on boot
            /// even if the UI takes seconds to come up.
            uint8_t initialBacklight = 0;
    };

    /// Bring up the expander with exactly the pins requested in `cfg`.
    /// Idempotent — safe to call more than once. Returns false if the
    /// expander failed to respond on I2C (missing board? wrong SDA/SCL?).
    bool init(const Config& cfg);

    // ---- Purpose-named pin control ----
    //
    // Every helper is safe to call before init(): it's a no-op in that
    // case. That lets hosts sequence calls however they like without
    // guarding each one.

    /// Turn the LCD backlight on or off. 0 = off, 1 = on.
    void setBacklight(uint8_t level);

    /// Quick blink pulse (off → on). Called internally at init() as a
    /// boot-sanity signal — exposed here for apps that want to re-blink
    /// after flashing firmware or as an error indicator.
    void backlightBlink();

    /// Assert / release the SD chip-select line (through the expander).
    /// `asserted=true` drives the line LOW (CS active on SPI slaves).
    /// Call `sdCs(true)` once before mounting the SPI SD filesystem —
    /// the shared SPI bus will then toggle the line for each transfer.
    void sdCs(bool asserted);

    /// LCD reset pulse control. `asserted=true` drives RST low (reset
    /// held). Typical sequence: assert → wait 10 ms → release.
    void lcdReset(bool asserted);

    /// Touch controller reset pulse control. Same semantics as
    /// lcdReset() but for the GT911 pin on the expander.
    void touchReset(bool asserted);

    // ---- Pin map ----
    //
    // Every value here is a hardware fact of the Waveshare
    // ESP32-S3-Touch-LCD-7 PCB. The `pins::` namespace lets host code
    // wire up peripherals (SPI bus, I2C bus, fan PWM, whatever) against
    // the board without guessing GPIO numbers.

    namespace pins {

        // I2C bus shared with the GT911 touch controller (separate I2C
        // port for the controller — this is the expander bus).
        constexpr int8_t EXPANDER_SDA = 8;
        constexpr int8_t EXPANDER_SCL = 9;

        // SPI bus exposed on the board header + used by the microSD
        // slot. The MOSI/SCK/MISO are shared; SD chip-select lives on
        // the expander (see sdCs() above).
        constexpr int8_t SD_SPI_MOSI = 11;
        constexpr int8_t SD_SPI_SCK = 12;
        constexpr int8_t SD_SPI_MISO = 13;

    }  // namespace pins

    // ---- CH422G expander pin assignments (internal use by board.cpp) ----
    //
    // Exposed because an advanced consumer might want to bit-bang a pin
    // the board module doesn't wrap. Keep this list minimal — new
    // use-cases should get a proper purpose-named helper above.
    namespace expander_pins {
        constexpr uint8_t TP_RST = 1;   // GT911 touch reset
        constexpr uint8_t LCD_BL = 2;   // LCD backlight (LOW = off)
        constexpr uint8_t LCD_RST = 3;  // LCD reset
        constexpr uint8_t SD_CS = 4;    // SD card chip-select
        constexpr uint8_t USB_SEL = 5;  // USB host/device select
    }  // namespace expander_pins

    namespace detail {

        /// Map a Config to the bitmask of expander pins that need to be
        /// registered as outputs. Pure function — exposed for host tests
        /// that want to verify "only the requested subsystems claim pins"
        /// without an expander on the wire.
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

}  // namespace ungula::bsp::waveshare::lcd7
