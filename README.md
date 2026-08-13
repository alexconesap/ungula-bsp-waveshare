# UngulaBspWaveshare

> **Board support package** for Waveshare ESP32-S3 touch-LCD development boards — one brand, one library, one entry point per model.

> **LLM usage note:** if this library is consumed from a coding AI workflow, explicitly point the agent to `API.md` first. `API.md` is the LLM-facing contract (public API + examples + constraints) and avoids wasting time/tokens scanning source files and this human-oriented README.

> **Warning - Active Development:** This library is under active architecture work to support multiple projects in parallel. Its structure is not finalized yet and may change without notice while this work is in progress. Updates are currently frequent (often daily). Target for structural freeze and stable `v1.0.0`: **June 2026**.

Waveshare ships a family of ESP32-S3 boards that share the same playbook: a CH422G I²C I/O expander fans out the "plumbing" pins (LCD reset, LCD backlight, touch reset, SD chip-select, USB select, audio mute) so the ESP32-S3's own GPIOs are free for RGB panel data and the SPI/UART headers. Every project that uses one of these boards ends up writing the same glue: wake the expander on I²C, register the right output pins, pulse LCD reset, turn the backlight on at the right moment, drive the SD CS line through the expander while the SPI bus does the real work.

This library **is that glue**. One shared CH422G owner, one purpose-named helper per board function (`setBacklight`, `sdCs`, `lcdReset`, `touchReset`), one idempotent `init()` per board model. Host projects stop reinventing the expander dance and stop accidentally double-initialising the chip from two subsystems at once.

## Table of Contents

- [C++ Compatibility](#c-compatibility)
- [What problem it solves](#what-problem-it-solves)
- [Supported boards](#supported-boards)
- [Quick start — ESP32-S3-Touch-LCD-7](#quick-start-esp32-s3-touch-lcd-7)
  - [Real-world: display + SD both coming up at boot](#real-world-display-sd-both-coming-up-at-boot)
- [API reference](#api-reference)
  - [Per-board module (`ungula::bsp::waveshare::lcd7`, `ungula::bsp::waveshare::lcd43`)](#per-board-module-ungulabspwavesharelcd7-ungulabspwavesharelcd43)
  - [`Config` struct](#config-struct)
  - [Shared CH422G owner (`ungula::bsp::waveshare::common`)](#shared-ch422g-owner-ungulabspwavesharecommon)
- [Structure](#structure)
- [Dependencies](#dependencies)
- [Testing](#testing)
- [Adding a new Waveshare board](#adding-a-new-waveshare-board)
- [Acknowledgements](#acknowledgements)
- [License](#license)
- [Arduino CLI symlink note (rarely relevant)](#arduino-cli-symlink-note-rarely-relevant)

## C++ Compatibility

- **Own source minimum**: `C++17`.
- **Effective minimum for consumers**: `C++17`.
- **Dependency impact**: Declared internal dependencies `UngulaCore` and `UngulaHal` are `C++17`.

## What problem it solves

Without a BSP, the same project ends up with code like this scattered across multiple files:

```cpp
#include <esp_io_expander.hpp>

// Board wiring — has to be known (and kept in sync) at every site that
// touches the expander. These are Waveshare ESP32-S3-Touch-LCD-7 facts.
constexpr int8_t  EXPANDER_SCL_PIN    = 9;
constexpr int8_t  EXPANDER_SDA_PIN    = 8;
constexpr uint8_t CH422G_I2C_ADDRESS  = 0x24;

// CH422G pin assignments on this board.
constexpr uint8_t CH_PIN_LCD_BACKLIGHT = 2;
constexpr uint8_t CH_PIN_LCD_RESET     = 3;
constexpr uint8_t CH_PIN_SD_CS         = 4;

constexpr uint32_t LCD_RESET_PULSE_MS  = 10;

// ---- display setup ----
auto* exp = new esp_expander::CH422G(EXPANDER_SCL_PIN, EXPANDER_SDA_PIN,
                                     CH422G_I2C_ADDRESS);
exp->init();
exp->begin();
exp->pinMode(CH_PIN_LCD_RESET,     OUTPUT);
exp->pinMode(CH_PIN_LCD_BACKLIGHT, OUTPUT);
exp->digitalWrite(CH_PIN_LCD_RESET, LOW);
delay(LCD_RESET_PULSE_MS);
exp->digitalWrite(CH_PIN_LCD_RESET,     HIGH);
exp->digitalWrite(CH_PIN_LCD_BACKLIGHT, HIGH);

// ---- sd_sink setup, different translation unit ----
#include <esp_io_expander.hpp>

// Same constants re-declared here, or worse: copy-pasted and silently
// drifting out of sync with the display setup above.
auto* exp2 = new esp_expander::CH422G(EXPANDER_SCL_PIN, EXPANDER_SDA_PIN,
                                      CH422G_I2C_ADDRESS);  // second owner, oops
exp2->init();
exp2->begin();
exp2->pinMode(CH_PIN_SD_CS, OUTPUT);
exp2->digitalWrite(CH_PIN_SD_CS, LOW);
```

Two problems: two owners of the same chip (depending on the driver, that's undefined behaviour at best), and every consumer has to know the I²C address, the pin numbers, the init sequence, and the fact that CH422G is what's on the other end of the bus. None of those are project concerns — they're board facts.

With the BSP, both sites just say what they need:

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

ungula::bsp::waveshare::lcd7::init({
    .enableSdCs       = true,
    .enableLcd        = true,
    .initialBacklight = 1,
});
```

(Field order follows the struct: `enableSdCs`, `enableLcd`, `enableTouch`,
`initialBacklight`. C++20 designated initialisers must be in declaration order.)

The first call brings the expander up, registers the requested pins as outputs, turns the backlight on, and releases the LCD reset line. Subsequent calls from other subsystems OR their pins into the existing config without touching what's already set up.

Two limits worth knowing before you rely on it:

- `init()` **does not pulse** LCD reset — it only releases it. If your panel
  needs a real reset, do `lcdReset(true)` → 10 ms → `lcdReset(false)` yourself.
- `init()` currently **always reports success** on an ESP32 target. The CH422G
  driver's return codes are discarded, so a missing board or wrong SDA/SCL still
  returns `true` and every later pin write is silently dropped.

## Supported boards

| Board | Header | Namespace | Status |
| --- | --- | --- | --- |
| Waveshare ESP32-S3-Touch-LCD-7 (7″) | `ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h` | `ungula::bsp::waveshare::lcd7` | Verified against real hardware |
| Waveshare ESP32-S3-Touch-LCD-4.3 (4.3″) | `ungula/bsp/waveshare/boards/esp32s3_touch_lcd_4_3/board.h` | `ungula::bsp::waveshare::lcd43` | **Pin map is placeholder — do not flash against real hardware until the schematic is confirmed.** |

Both board modules expose the same API surface, so host code that aliases `namespace board = ungula::bsp::waveshare::lcd7;` can swap models by changing the alias.

## Quick start — ESP32-S3-Touch-LCD-7

The typical wiring: CH422G expander on I²C, RGB panel driven directly by ESP32-S3 pins, microSD over SPI with the CS line routed through the expander. All pin facts come from the board module — the application never hard-codes them.

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>
#include <ungula/hal/spi/spi_master.h>

namespace board = ungula::bsp::waveshare::lcd7;

// Project-level knobs that aren't board facts — keep them named so
// someone reading setup() doesn't have to guess what a literal means.
constexpr uint32_t SD_SPI_FREQ_HZ     = 10000000;
constexpr uint8_t  SD_SPI_MODE        = 0;
constexpr int8_t   SD_SPI_CS_UNUSED   = -1;  // CS is on the expander, not the bus.
constexpr uint8_t  INITIAL_BACKLIGHT  = 1;   // 0 = off, 1 = on

ungula::hal::spi::SpiMaster sdSpi;

void setup() {
    // One call covers: expander wake-up on I2C, backlight + SD CS output
    // registration, initial backlight level, and releasing the LCD reset
    // line. Blocks ~200 ms for the boot blink. Safe to call from other
    // subsystems too — they just OR their pins in.
    board::init({
        .enableSdCs       = true,
        .enableLcd        = true,
        .enableTouch      = false,   // touch stack owned elsewhere
        .initialBacklight = INITIAL_BACKLIGHT,
    });

    // Hold SD CS asserted before bringing the SPI bus up; the bus
    // driver will drive it on each transfer afterwards.
    board::sdCs(true);
    sdSpi.begin(board::pins::SD_SPI_SCK,
                board::pins::SD_SPI_MISO,
                board::pins::SD_SPI_MOSI,
                SD_SPI_CS_UNUSED,
                SD_SPI_FREQ_HZ,
                SD_SPI_MODE);

    // ... mount filesystem, bring up UI, etc.
}

void onFault() {
    // Re-blink the backlight as a visible error indicator.
    board::backlightBlink();
}
```

### Real-world: display + SD both coming up at boot

This is the pattern that motivated the library — the UI subsystem and the SD logging sink both need the expander, and they don't know about each other. Both just declare what they need:

```cpp
// src/boot/ui_boot.cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

void bootUi() {
    ungula::bsp::waveshare::lcd7::init({
        .enableLcd        = true,
        .enableTouch      = true,
        .initialBacklight = 1,
    });
    // Backlight is on, LCD and touch reset lines are released, both pins
    // are registered as outputs. No reset pulse was issued — add one here
    // if the panel needs it.
    startDisplayStack();
}
```

```cpp
// src/boot/sd_boot.cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

void bootSdSink() {
    ungula::bsp::waveshare::lcd7::init({ .enableSdCs = true });
    // Expander is already up from bootUi(); this call only OR-s the
    // SD_CS pin into the output mask and returns.
    ungula::bsp::waveshare::lcd7::sdCs(true);
    mountSdFilesystem();
}
```

Boot-order sensitivity: call order between `bootUi()` and `bootSdSink()` does not matter. Whichever runs first wakes the chip; the other one just adds its pin.

## API reference

### Per-board module (`ungula::bsp::waveshare::lcd7`, `ungula::bsp::waveshare::lcd43`)

| Function | Description |
| --- | --- |
| `init(Config)` | Bring the expander up with exactly the pins the caller needs. Wake-up is idempotent; the backlight/blink side effects are not. Returns `true` unconditionally on target — see the caveat above. |
| `setBacklight(level)` | 0 = off, non-zero = on. No-op before `init()`. |
| `backlightBlink()` | Off → 100 ms → on → 100 ms. **Blocks ~200 ms** and leaves the backlight on. |
| `sdCs(asserted)` | `true` drives CS low (SPI slave selected). Call once before mounting; the SPI bus toggles it afterwards. |
| `lcdReset(asserted)` | `true` = hold reset (LOW), `false` = release (HIGH). Full pulse is assert → 10 ms → release; `init()` only does the release half. |
| `touchReset(asserted)` | Same as `lcdReset()` but for the GT911 touch controller. Same caveat. |
| `pins::*` | Compile-time constants for GPIO-visible pins (I²C bus, SD SPI bus). |
| `expander_pins::*` | CH422G pin assignments — exposed for advanced consumers who want to bit-bang an expander pin the board module doesn't wrap. Prefer the purpose-named helpers above. |

### `Config` struct

Declared in each board header (not in the umbrella):

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

struct Config {
    bool enableSdCs       = false;  // register SD CS as expander output
    bool enableLcd        = false;  // register LCD reset + backlight, release reset
    bool enableTouch      = false;  // register GT911 reset as expander output
    uint8_t initialBacklight = 0;   // 0/1, read only when enableLcd
};
```

The flags are independent on purpose: a project might want the LCD lit for a boot splash without bringing the touch stack up, or might want SD without ever enabling the display.

`initialBacklight` does not survive `init()`: the boot blink that follows ends
with the backlight on, so the pin is HIGH afterwards whatever you passed. Call
`setBacklight(0)` after `init()` if you need the panel dark.

The board headers do not include `ch422g_expander.h`, so
`common::LEVEL_LOW` / `LEVEL_HIGH` are not visible from a board-header-only
include. Use plain `0` / `1`, or include `<ungula/bsp/waveshare.h>` as well.

### Shared CH422G owner (`ungula::bsp::waveshare::common`)

Board modules call this owner; host projects should normally go through the
board module's purpose-named helpers instead. It is public so a consumer can
reach an expander pin the board module doesn't wrap (today only `USB_SEL`).

```cpp
#include <ungula/bsp/waveshare/common/ch422g_expander.h>   // or the umbrella header

namespace ungula::bsp::waveshare::common {

constexpr uint8_t LEVEL_LOW  = 0;
constexpr uint8_t LEVEL_HIGH = 1;

enum class PinMode : uint8_t { Output = 0, Input = 1 };   // declared, not used by any API

bool ensureInit(int8_t sdaPin, int8_t sclPin, uint8_t outputPinsMask);
void writePin(uint8_t pinNumber, uint8_t level);
bool isReady();

}
```

`ensureInit()` takes `(sda, scl)` and swaps them internally for the
`esp_expander::CH422G(scl, sda, addr)` constructor. `outputPinsMask` is a
`uint8_t`, so only CH422G pins 0-7 can be registered. Both `ensureInit()` and
`isReady()` report on the driver object, not on whether the chip answered — see
`API.md` for the full caveat.

## Structure

```text
src/
  ungula_bsp_waveshare.h                        # flat forwarder (Arduino discovery)
  ungula/bsp/
    waveshare.h                                 # umbrella header
    waveshare/
      common/
        ch422g_expander.h / .cpp                # single CH422G owner
      boards/
        esp32s3_touch_lcd_7/
          board.h / .cpp                        # ungula::bsp::waveshare::lcd7
        esp32s3_touch_lcd_4_3/
          board.h / .cpp                        # ungula::bsp::waveshare::lcd43 (pin map TBD)
```

## Dependencies

| Library | Origin | Used for |
| --- | --- | --- |
| UngulaCore | Sibling lib in `cpp-libraries/` | `ungula::core::time` (reset pulse timing, backlight blink) |
| UngulaHal | Sibling lib in `cpp-libraries/` | Nothing at runtime today — declared for future board modules that register GPIO pins via `hal/gpio/gpio_access` |
| ESP32_IO_Expander | **External** (esp-arduino-libs) | Actual CH422G driver. ESP32 target only. |

All three are declared in `library.properties`'s `depends=` field, but
**Arduino CLI does not auto-resolve dependencies** — it only uses
`depends=` as documentation. The host project must make each dependency
available on its own.

For the `UngulaCore` / `UngulaHal` sibling libraries, the usual pattern
is a symlink from `<host_project>/libraries/` into `cpp-libraries/`.

For `ESP32_IO_Expander` (external), the recommended pattern is to pin
it in the host project's `libraries/list.txt` so a build script adds it
as a git submodule on first checkout:

```
# host_project/libraries/list.txt
ESP32_IO_Expander|https://github.com/esp-arduino-libs/ESP32_IO_Expander|0.0.3
```

…or install it globally once with
`arduino-cli lib install ESP32_IO_Expander`. **The BSP does not ship,
bundle, or vendor this code** — the host owns the dependency copy.

## Testing

Host tests cover the pure pin-mask builder in each board's `detail::` namespace — i.e. the "which expander pins get claimed for which enabled subsystems" logic. Nothing else is covered: `board.cpp` and `ch422g_expander.cpp` are not compiled by the test target at all, so the init ordering, the backlight/blink interaction, and the expander wrapper have no host coverage. That path needs a real expander on the wire and is exercised by the host project that flashes the board.

```bash
cd lib_bsp_waveshare/tests
./1_build.sh
./2_run.sh
```

## Adding a new Waveshare board

1. Create `src/ungula/bsp/waveshare/boards/<model>/board.h` + `.cpp` alongside the existing ones.
2. Expose the same API surface (`init`, `setBacklight`, `backlightBlink`, `sdCs`, `lcdReset`, `touchReset`, `pins::`, `expander_pins::`) so host code that aliases the namespace can swap models.
3. Reuse `ungula::bsp::waveshare::common::ensureInit` / `writePin` — don't touch the expander driver directly.
4. Bump `library.properties` / `.version`, update `docs/LIBRARY_VERSIONS.md`.

## Acknowledgements

Thanks to Claude and ChatGPT for helping generate this documentation.

## License

MIT License — see [LICENSE](LICENSE) file.

---

## Arduino CLI symlink note (rarely relevant)

This library ships a flat forwarder header at `src/ungula_bsp_waveshare.h` that
just `#include`s `ungula/bsp/waveshare.h`. `library.properties` `includes=` points
at the forwarder.

It only exists to work around an Arduino CLI quirk: when the library is
consumed through a symlink, the CLI sometimes fails to discover headers
nested under `src/ungula/`. The flat forwarder fixes that scan.

**Host code keeps including the real header**:

```cpp
#include <ungula/bsp/waveshare.h>
```

PlatformIO, ESP-IDF component builds, and plain CMake setups can ignore
the forwarder.
