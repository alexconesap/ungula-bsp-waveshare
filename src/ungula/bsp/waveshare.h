// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#pragma once

/// @brief Umbrella header — drag this into a sketch to make the Arduino CLI
/// discover the library. Application code should include the specific
/// board header it targets, e.g.:
///
///   #include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>
///
/// Brand-level layout (Waveshare uses the same playbook across their
/// ESP32-S3 LCD line — CH422G I/O expander on a fixed I2C address, similar
/// reset/backlight sequencing, similar SD CS routing through the expander),
/// so one library covers every supported model and the per-board headers
/// only carry pin maps + Config + init().

#include <ungula/bsp/waveshare/common/ch422g_expander.h>
