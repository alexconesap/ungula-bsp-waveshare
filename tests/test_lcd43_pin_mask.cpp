// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#include <gtest/gtest.h>

#include <bsp/waveshare/boards/esp32s3_touch_lcd_4_3/board.h>

namespace {

    namespace board = bsp::ws::lcd43;
    using board::Config;
    using board::detail::outputPinsMaskFor;

    constexpr uint8_t bit(uint8_t pin) {
        return static_cast<uint8_t>(1U << pin);
    }

    // 4.3" pin map is flagged as placeholder in the header, but the mask
    // *logic* (which subsystems claim which pins) still matches the 7"
    // playbook. These tests guard the contract, not the pin numbers.

    TEST(Lcd43PinMaskTest, EmptyConfigClaimsNothing) {
        EXPECT_EQ(outputPinsMaskFor(Config{}), 0U);
    }

    TEST(Lcd43PinMaskTest, LcdClaimsBacklightAndReset) {
        Config cfg;
        cfg.enableLcd = true;
        const uint8_t expected =
                bit(board::expander_pins::LCD_BL) | bit(board::expander_pins::LCD_RST);
        EXPECT_EQ(outputPinsMaskFor(cfg), expected);
    }

    TEST(Lcd43PinMaskTest, AllEnabledIsUnionOfParts) {
        Config cfg;
        cfg.enableLcd = true;
        cfg.enableTouch = true;
        cfg.enableSdCs = true;
        const uint8_t expected = bit(board::expander_pins::LCD_BL) |
                                 bit(board::expander_pins::LCD_RST) |
                                 bit(board::expander_pins::TP_RST) |
                                 bit(board::expander_pins::SD_CS);
        EXPECT_EQ(outputPinsMaskFor(cfg), expected);
    }

}  // namespace
