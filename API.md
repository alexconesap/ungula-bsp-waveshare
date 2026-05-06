# UngulaBspWaveshare

Board support package for Waveshare ESP32-S3 touch-LCD development boards.
Owns the CH422G I2C I/O expander shared by every board in the family and
exposes one purpose-named entry point per board model. Host code calls
`board::init(...)` once per subsystem (idempotent) instead of re-doing
the expander wake-up, LCD reset pulse, and SD CS plumbing in every
translation unit.

Target: ESP32 (`architectures=esp32`). Depends on `UngulaCore`,
`UngulaHal`, and the external `ESP32_IO_Expander` library (CH422G driver).

---

## Usage

### Use case: bring up the LCD with backlight on at boot

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

void setup() {
    ungula::bsp::waveshare::lcd7::init({
        .enableSdCs       = false,
        .enableLcd        = true,
        .enableTouch      = false,
        .initialBacklight = ungula::bsp::waveshare::common::LEVEL_HIGH,
    });
}

void loop() {}
```

When to use this: any project that drives the RGB panel and wants the
expander, LCD reset pulse, and backlight all handled in one call.

### Use case: SD card over SPI with CS on the expander

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

namespace board = ungula::bsp::waveshare::lcd7;

void setup() {
    board::init({ .enableSdCs = true });
    board::sdCs(true);  // hold CS asserted before SPI bus comes up
    // Wire the SPI bus using board::pins::SD_SPI_SCK / SD_SPI_MOSI /
    // SD_SPI_MISO. Pass -1 for the bus-level CS — it is on the expander.
}

void loop() {}
```

When to use this: onboard microSD slot. The board module owns CS routing;
the SPI bus driver only deals with MOSI/MISO/SCK.

### Use case: two subsystems both need the expander

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

void bootUi() {
    ungula::bsp::waveshare::lcd7::init({
        .enableLcd        = true,
        .enableTouch      = true,
        .initialBacklight = ungula::bsp::waveshare::common::LEVEL_HIGH,
    });
}

void bootSdSink() {
    ungula::bsp::waveshare::lcd7::init({ .enableSdCs = true });  // OR-s SD_CS into existing config
    ungula::bsp::waveshare::lcd7::sdCs(true);
}

void setup() {
    bootUi();
    bootSdSink();  // safe in either order
}

void loop() {}
```

When to use this: multi-subsystem boot where ownership of the expander is
diffuse. `init()` is idempotent and merges output pin masks.

### Use case: error indicator via backlight blink

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>

void onFault() {
    ungula::bsp::waveshare::lcd7::backlightBlink();
}
```

When to use this: visible runtime fault signal. `backlightBlink()` is also
called internally by `init()` as a boot-sanity pulse.

### Use case: manual LCD or touch reset

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h>
#include <ungula/core/time/time_control.h>

void resequenceTouchPanel() {
    ungula::bsp::waveshare::lcd7::touchReset(true);
    ungula::core::time::TimeControl::delayMs(10);
    ungula::bsp::waveshare::lcd7::touchReset(false);
}
```

When to use this: re-init a peripheral without rebooting the board. `init()`
already performs the LCD reset pulse once.

### Use case: 4.3" board (placeholder pin map)

```cpp
#include <ungula/bsp/waveshare/boards/esp32s3_touch_lcd_4_3/board.h>

namespace board = ungula::bsp::waveshare::lcd43;  // identical surface to lcd7

void setup() {
    board::init({ .enableLcd = true, .initialBacklight = 1 });
}

void loop() {}
```

When to use this: code targeting the 4.3" model. The API mirrors `lcd7`
exactly — switch boards by changing the `namespace board =` alias.

WARNING: pin map and CH422G assignments in `ungula::bsp::waveshare::lcd43` are placeholders.
Do not flash against real hardware until the schematic is confirmed
(see `TODO(waveshare-4.3)` markers in `board.h`).

---

## Supported boards

| Board | Header | Namespace | Status |
| --- | --- | --- | --- |
| Waveshare ESP32-S3-Touch-LCD-7 | `ungula/bsp/waveshare/boards/esp32s3_touch_lcd_7/board.h` | `ungula::bsp::waveshare::lcd7` | Verified on hardware |
| Waveshare ESP32-S3-Touch-LCD-4.3 | `ungula/bsp/waveshare/boards/esp32s3_touch_lcd_4_3/board.h` | `ungula::bsp::waveshare::lcd43` | Pin map placeholder |

Both modules expose the same symbols (`init`, `setBacklight`,
`backlightBlink`, `sdCs`, `lcdReset`, `touchReset`, `pins::*`,
`expander_pins::*`).

---

## Pin map

### `ungula::bsp::waveshare::lcd7::pins` (verified)

| Constant | Value | Function |
| --- | --- | --- |
| `EXPANDER_SDA` | 8 | I2C SDA to CH422G + GT911 |
| `EXPANDER_SCL` | 9 | I2C SCL to CH422G + GT911 |
| `SD_SPI_MOSI` | 11 | SPI MOSI (microSD + header) |
| `SD_SPI_SCK`  | 12 | SPI SCK |
| `SD_SPI_MISO` | 13 | SPI MISO |

### `ungula::bsp::waveshare::lcd43::pins` (placeholder — same values pending schematic)

Same constants as `lcd7`, marked `TODO(waveshare-4.3)`.

### CH422G expander pin assignments

Identical layout in both boards (4.3" assumed, not confirmed):

| Constant | Bit | Function |
| --- | --- | --- |
| `expander_pins::TP_RST`  | 1 | GT911 touch reset |
| `expander_pins::LCD_BL`  | 2 | LCD backlight (LOW = off) |
| `expander_pins::LCD_RST` | 3 | LCD reset |
| `expander_pins::SD_CS`   | 4 | SD card chip-select |
| `expander_pins::USB_SEL` | 5 | USB host/device select |

`USB_SEL` has no purpose-named helper. Reach it via
`ungula::bsp::waveshare::common::writePin(expander_pins::USB_SEL, level)` only if
no other entry point covers the use case.

---

## Public types

### `ungula::bsp::waveshare::lcd7::Config` (and identical `ungula::bsp::waveshare::lcd43::Config`)

```cpp
struct Config {
    bool    enableSdCs       = false;
    bool    enableLcd        = false;
    bool    enableTouch      = false;
    uint8_t initialBacklight = 0;
};
```

Field meaning:

- `enableSdCs` — register `SD_CS` as expander output.
- `enableLcd` — register `LCD_RST` + `LCD_BL` as outputs, pulse LCD reset,
  apply `initialBacklight`.
- `enableTouch` — register `TP_RST` as expander output.
- `initialBacklight` — `0` or `1`. Applied only when `enableLcd` is true.
  A `backlightBlink()` is performed regardless (visible boot signal).

Flags are independent: any combination is valid. Subsequent `init()` calls
OR their masks into the existing expander config.

### `ungula::bsp::waveshare::common::PinMode`

```cpp
enum class PinMode : uint8_t { Output = 0, Input = 1 };
```

Direction value for the shared expander layer. Board modules currently
register only outputs.

### `ungula::bsp::waveshare::common` constants

```cpp
constexpr uint8_t LEVEL_LOW  = 0;
constexpr uint8_t LEVEL_HIGH = 1;
```

Logical levels — use these instead of Arduino `LOW` / `HIGH` to avoid
pulling Arduino macros into headers.

---

## Public functions

### Per-board (`ungula::bsp::waveshare::lcd7`, `ungula::bsp::waveshare::lcd43`)

#### `bool init(const Config& cfg)`

- **Purpose**: bring the CH422G up on the board's I2C pins, register the
  output pins implied by `cfg`, pulse LCD reset (when `enableLcd`), apply
  `initialBacklight` (when `enableLcd`), perform a `backlightBlink()`.
- **Parameters**: `cfg` — see `Config` above.
- **Returns**: `true` on success. `false` only if the CH422G did not ACK
  on I2C (missing board, wrong SDA/SCL).
- **Side effects**: I2C transactions to address `0x24`; expander output
  pin state changes; ~10 ms LCD reset pulse via `ungula::core::time::TimeControl`.
- **Idempotent**: yes. Subsequent calls only OR new pins into the mask
  and may re-apply `initialBacklight`.
- **Usage notes**: must run after Arduino `setup()` is reached but before
  any subsystem reads/writes through the expander.

#### `void setBacklight(uint8_t level)`

- 0 = off, 1 = on. No-op if `init()` has not succeeded.

#### `void backlightBlink()`

- Quick off/on pulse on `LCD_BL`. No-op if `init()` has not succeeded.

#### `void sdCs(bool asserted)`

- `true` drives SD CS LOW (slave selected on SPI). No-op if `init()` has
  not succeeded. Call once before mounting the SPI SD filesystem.

#### `void lcdReset(bool asserted)`

- `true` holds LCD reset asserted. Typical sequence: `lcdReset(true)` →
  10 ms wait → `lcdReset(false)`. `init()` already does this once.

#### `void touchReset(bool asserted)`

- Same shape as `lcdReset`, for the GT911 touch controller.

### Shared expander (`ungula::bsp::waveshare::common`)

The board modules use these internally. Host code should reach them only
when no purpose-named board helper exists for the pin (e.g. `USB_SEL`).

#### `bool ensureInit(int8_t sdaPin, int8_t sclPin, uint8_t outputPinsMask)`

- Wakes the CH422G on the given I2C pins. OR-s `outputPinsMask` into the
  current output configuration.
- **Returns**: `true` on success, `false` if the chip did not respond.
- Idempotent — repeated calls are silent no-ops aside from the mask OR.

#### `void writePin(uint8_t pinNumber, uint8_t level)`

- Drive a CH422G pin. No-op if `ensureInit()` has not succeeded.

#### `bool isReady()`

- True iff `ensureInit()` succeeded at least once.

### `detail::outputPinsMaskFor(const Config&)` (per board)

`constexpr` helper that maps a `Config` to the expander output mask.
Public for host tests (see `tests/test_lcd7_pin_mask.cpp`,
`tests/test_lcd43_pin_mask.cpp`). Application code should not call it.

---

## Lifecycle

1. Power on, Arduino `setup()` reached.
2. One or more subsystems call `bsp::ws::<model>::init(cfg)` with the
   pins they need. Order does not matter; flags are merged.
3. First successful `init()` brings up the CH422G, registers requested
   outputs, pulses LCD reset, applies `initialBacklight`, blinks backlight.
4. Operate: call `setBacklight` / `sdCs` / `lcdReset` / `touchReset` as
   needed. All are no-ops before a successful `init()`.
5. There is no shutdown / teardown call. The expander stays initialized
   for the lifetime of the boot.

Violation behavior:

- Calling helpers before `init()` succeeds → silent no-op.
- Calling `init()` with the wrong SDA/SCL or no board attached → returns
  `false`; subsequent helpers remain no-ops until a later `init()` call
  succeeds.

---

## Error handling

- `init()` and `ensureInit()` return `bool`. `false` means I2C ACK failure.
  No exceptions are thrown; errno is not used.
- All pin-write helpers are silent no-ops when the expander is not ready.
  No error code is returned. Use `ungula::bsp::waveshare::common::isReady()` to
  query state explicitly.
- No logging is performed inside the library (per project rule —
  logging is the host's responsibility).

---

## Threading / timing / hardware notes

- I2C address: CH422G at `0x24` (declared internally; not exposed).
- LCD reset pulse: ~10 ms, sourced from `ungula::core::time::TimeControl` rather than
  Arduino `delay()`.
- Not interrupt-safe: do not call any function from an ISR. The CH422G
  driver issues blocking I2C transactions.
- Not thread-safe: no internal mutex. Host code must serialize calls if
  multiple FreeRTOS tasks invoke board helpers concurrently.
- Heap: the CH422G driver instance is allocated once on first
  `ensureInit()`. Per project rule, treat `init()` as a `setup()`-time
  call only — do not invoke for the first time after `setup()` returns.

---

## Internals not part of the public API

- `ungula::bsp::waveshare::common::ensureInit` / `writePin` / `isReady` —
  technically reachable, but host code should go through the board
  module's purpose-named helpers. Direct use is allowed only for
  expander pins with no purpose-named wrapper (currently `USB_SEL`).
- `detail::outputPinsMaskFor` (per board) — `constexpr` helper for tests.
- `ch422g_expander.cpp` internals (driver pointer, init flag) — not
  exposed; do not include or extern.
- `board.cpp` translation units — implementation detail.
- The umbrella header `<ungula/bsp/waveshare.h>` exists only to make
  Arduino CLI discover the library. It does not pull in any board
  module — application code must include the specific board header.

---

## Recommended improvements (proposed — not yet implemented)

The library is mostly deep, but a few sharp edges stand out:

1. **Distinct return type for `init()` failure modes.** Currently `bool`
   collapses "chip absent", "wrong pins", and "first call already
   succeeded" into the same value. Proposed: an `InitResult` enum
   (`Ok`, `AlreadyInitialized`, `I2cNack`, `ConfigConflict`).
2. **`USB_SEL` purpose-named helper.** `expander_pins::USB_SEL` is the
   only board-level pin without a wrapper, forcing host code into the
   `common::writePin` back-door. Proposed: `void usbSelect(bool host)`
   on each board module.
3. **Compile-time board selector.** A small wrapper header that picks
   `lcd7` vs `lcd43` from a build flag would let host code drop the
   `namespace board = ...;` alias dance. Proposed:
   `<ungula/bsp/waveshare/board.h>` driven by `BOARD_WAVESHARE_S3_LCD7` /
   `BOARD_WAVESHARE_S3_LCD43`.
4. **`Config` validation.** `initialBacklight` accepts any `uint8_t` but
   only `0` and `1` are meaningful. Proposed: `enum class Backlight :
   uint8_t { Off = 0, On = 1 };` field.
5. **4.3" pin map.** Confirm the schematic and remove the
   `TODO(waveshare-4.3)` placeholders.

Treat every item above as a proposal, not as existing API.

---

## LLM usage rules

- Use only the symbols documented here. Do not include
  `ungula/bsp/waveshare/common/ch422g_expander.h` directly unless writing a new
  board module inside this library.
- Prefer the per-board purpose-named helpers (`setBacklight`, `sdCs`,
  `lcdReset`, `touchReset`) over `common::writePin`.
- Never instantiate `esp_expander::CH422G` from host code — the BSP
  owns the chip. Two owners produces undefined behavior.
- Pin numbers come from `bsp::ws::<model>::pins::*`. Do not hard-code
  GPIOs.
- `init()` is the only entry point that must run before other helpers
  do anything. Helpers are no-ops, not errors, before then.
- `delay()` / `millis()` / `digitalWrite()` are not used by this
  library; do not introduce them when extending it (use
  `ungula::core::time::TimeControl` and the shared expander instead).
- If a needed feature is missing (e.g. USB-host select), say so
  explicitly rather than reaching past the public surface.
- Preserve the namespace path (`ungula::bsp::waveshare::lcd7`, `ungula::bsp::waveshare::common`)
  exactly — these are the documented entry points.
