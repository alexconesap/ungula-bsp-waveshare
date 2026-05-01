// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#pragma once

#include <cstdint>

/// @brief CH422G I/O expander shared by every Waveshare ESP32-S3 LCD board.
///
/// Single owner across the BSP — every board module that needs an expander
/// pin (LCD reset, LCD backlight, touch reset, SD chip-select, USB select,
/// audio mute…) goes through this object instead of touching the chip
/// directly. Lazy first-use init: the first board::init() call brings the
/// chip up; subsequent calls (from other consumers on the same project)
/// are no-ops.
///
/// The exact CH422G pin → function map is board-specific and lives in each
/// board module. This file only deals with "talk to the chip" mechanics.

namespace bsp::waveshare::common {

    /// Logical levels — match the CH422G driver's expected values without
    /// dragging Arduino macros into the header.
    constexpr uint8_t LEVEL_LOW = 0;
    constexpr uint8_t LEVEL_HIGH = 1;

    /// I/O direction for `pinMode()`.
    enum class PinMode : uint8_t {
        Output = 0,
        Input = 1,
    };

    /// Bring the expander up on the given I2C bus. Safe to call multiple
    /// times — the second and later calls are silent no-ops. The
    /// configurePinsAsOutput bitmask is OR-ed into whatever was previously
    /// set so different consumers can register their own pins without
    /// stomping on each other.
    ///
    /// @param sdaPin GPIO carrying SDA (board fact, supplied by board module)
    /// @param sclPin GPIO carrying SCL
    /// @param outputPinsMask Bitmask of CH422G pins that should be set as
    ///                      outputs at boot. e.g. (1<<2) | (1<<4) for the
    ///                      backlight + SD CS pair.
    /// @return true on success, false if the chip didn't respond.
    bool ensureInit(int8_t sdaPin, int8_t sclPin, uint8_t outputPinsMask);

    /// Drive a pin. No-op if ensureInit() was never called or it failed.
    void writePin(uint8_t pinNumber, uint8_t level);

    /// True iff ensureInit() succeeded at least once.
    bool isReady();

}  // namespace bsp::waveshare::common
