// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#include <gtest/gtest.h>

#include <bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

namespace {

    namespace board = bsp::ws::lcd7;
    using board::Config;
    using board::detail::outputPinsMaskFor;

    constexpr uint8_t bit(uint8_t pin) {
        return static_cast<uint8_t>(1U << pin);
    }

    TEST(Lcd7PinMaskTest, EmptyConfigClaimsNothing) {
        EXPECT_EQ(outputPinsMaskFor(Config{}), 0U);
    }

    TEST(Lcd7PinMaskTest, LcdClaimsBacklightAndReset) {
        Config cfg;
        cfg.enableLcd = true;
        const uint8_t expected =
                bit(board::expander_pins::LCD_BL) | bit(board::expander_pins::LCD_RST);
        EXPECT_EQ(outputPinsMaskFor(cfg), expected);
    }

    TEST(Lcd7PinMaskTest, TouchClaimsOnlyTouchReset) {
        Config cfg;
        cfg.enableTouch = true;
        EXPECT_EQ(outputPinsMaskFor(cfg), bit(board::expander_pins::TP_RST));
    }

    TEST(Lcd7PinMaskTest, SdClaimsOnlySdCs) {
        Config cfg;
        cfg.enableSdCs = true;
        EXPECT_EQ(outputPinsMaskFor(cfg), bit(board::expander_pins::SD_CS));
    }

    TEST(Lcd7PinMaskTest, AllEnabledIsUnionOfParts) {
        Config cfg;
        cfg.enableLcd = true;
        cfg.enableTouch = true;
        cfg.enableSdCs = true;
        const uint8_t expected =
                bit(board::expander_pins::LCD_BL) | bit(board::expander_pins::LCD_RST) |
                bit(board::expander_pins::TP_RST) | bit(board::expander_pins::SD_CS);
        EXPECT_EQ(outputPinsMaskFor(cfg), expected);
    }

    TEST(Lcd7PinMaskTest, InitialBacklightDoesNotAffectMask) {
        // initialBacklight is a default-state knob, not a pin-selection knob.
        Config a;
        a.enableLcd = true;
        a.initialBacklight = 0;
        Config b = a;
        b.initialBacklight = 1;
        EXPECT_EQ(outputPinsMaskFor(a), outputPinsMaskFor(b));
    }

    TEST(Lcd7PinMaskTest, UsbSelectIsNeverClaimedByConfig) {
        // The BSP exposes USB_SEL in expander_pins for advanced callers,
        // but the Config flags never register it. Lock that so a future
        // "enableUsb" flag is an intentional decision, not a slip.
        Config cfg;
        cfg.enableLcd = true;
        cfg.enableTouch = true;
        cfg.enableSdCs = true;
        const uint8_t usbBit = bit(board::expander_pins::USB_SEL);
        EXPECT_EQ(outputPinsMaskFor(cfg) & usbBit, 0U);
    }

}  // namespace
