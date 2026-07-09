# Aciduino V2 — ESP32 WROOM controller for a SAM2695 GM module

This port runs the standard AciduinoV2 303/808 sequencer and OLED UI on an
ESP32 WROOM and uses it purely as a **MIDI controller**. There is **no internal
synthesis** — all sound comes from an external **General MIDI module** (the
Nulllab / SAM2695 "GM 2.0, 128 tones" board), driven over a hardware serial MIDI
port. The module renders audio to its own amplified speaker.

Selected in `AciduinoV2.ino` with:

```cpp
#include "src/ports/esp32/wroom_sam_gm.h"
```

## Hardware

| Part | Notes |
|------|-------|
| ESP32 WROOM dev board (30-pin) | |
| 1.3" OLED, SH1106, 128×64, I2C | `U8G2_SH1106_128X64_NONAME_F_HW_I2C` |
| Rotary encoder + push button | Value changer + SHIFT |
| 6× momentary push buttons | Navigation + transport |
| SAM2695 GM module (Nulllab GM 2.0) | TTL serial MIDI in @ 31250 baud |

### Pinout

| Signal | GPIO |
|--------|------|
| OLED SDA | 21 |
| OLED SCL | 22 |
| MIDI out → SAM module (Serial2 TX) | 17 |
| Encoder A | 32 |
| Encoder B | 33 |
| Encoder push (SHIFT) | 13 |
| Button 1 (PAGE) | 14 |
| Button 2 (UP) | 4 |
| Button 3 (DOWN) | 5 |
| Button 4 (PREVIOUS) | 18 |
| Button 5 (NEXT) | 19 |
| Button 6 (PLAY/STOP) | 23 |
| BPM LED (on-board) | 2 |

### SAM2695 module connection

The module's 3-pin **MIDI** connector is a TTL serial MIDI input (31250 baud):

```
ESP32 GPIO17 (Serial2 TX) ──> module MIDI signal / RX
ESP32 GND                 ──> module GND
5V (module micro-USB or a 5V rail) powers the module
```

The ESP32 TX is 3.3V, which the SAM2695 UART accepts — a direct wire works, no
MIDI opto-isolator / DIN circuit needed. Buttons and the encoder connect to GND
and use the ESP32 internal pull-ups (handled by uCtrl's DIN module). None of the
nav pins use the input-only GPIOs 34–39.

## Controls (encoder + 6 buttons)

```
Encoder rotate ........ change the selected value
Encoder push (SHIFT) .. hold for a button's shifted function
Button 1 ......... PAGE cycle     | SHIFT = page back
Button 2 ......... UP             | SHIFT = previous track
Button 3 ......... DOWN           | SHIFT = next track
Button 4 ......... PREVIOUS (<-)
Button 5 ......... NEXT (->)
Button 6 ......... PLAY / STOP    | SHIFT = REC on/off
```

The midilab UI is designed for 9 buttons; this folds it onto 7 inputs, so the
secondary roles (track switch, page-back, rec) live on SHIFT. The exact uCtrl
navigation binding is a starting point — verify it on hardware once the
`uCtrl`/`uClock` submodules are pulled in, and adjust the `setNavComponentCtrl()`
block in `wroom_sam_gm.h` if needed.

## Getting sound out of the SAM2695

The SAM2695 boots to the standard General MIDI map:

- **Channel 1** = Acoustic Grand Piano … the 303 tracks default to channels 1–4.
- **Channel 10** = drum kit. Set the **808 track's output channel to 10** in the
  MIDI page to play it as drums — the note number then selects the drum sound.

Set each track's channel from the MIDI page. To change a track's instrument,
send a Program Change on its channel from a connected controller/PC (via the USB
serial bridge, `USE_MIDI1`), or extend the firmware to send Program Changes.

## Build settings

- Board: **ESP32 Dev Module**
- Default partition scheme is fine (no embedded soundfont in this build).
- PSRAM: not required.

### `uClock` is vendored — no submodule step needed

`src/uClock` is plain, committed source in this repo (not a git submodule
anymore), built from a fork with its `examples/` folder removed. A normal
clone or "Download ZIP" of this repo gives you a working `uClock` with no
extra steps.

### `uCtrl` is still a submodule: delete its `examples/` folder after init

`uCtrl` is a full library repo vendored via git submodule under `src/`, and it
ships its own `examples/` folder with unrelated demo sketches (some
Teensy/AVR-specific). Arduino's compiler recursively sweeps up **every**
`.c`/`.cpp` file under the sketch tree — not just the ones this firmware
`#include`s — so those examples get pulled into the build and fail with
missing-header errors (e.g. `usb_names.h`, which Teensyduino auto-generates
and only exists for Teensy USB-MIDI builds).

Once the submodule is populated (`git submodule update --init --recursive`,
or a manual copy), delete:

```
src/uCtrl/examples
```

Safe to remove — nothing in the Aciduino firmware itself references it.

> ⚠️ This firmware has not been compiled against the ESP32 Arduino toolchain or
> flashed in this environment (the `uCtrl`/`uClock` submodules were not present
> and no toolchain was available). Validate the serial-MIDI wiring to the module
> and the uCtrl navigation binding on real hardware.
