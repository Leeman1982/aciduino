# Aciduino V2 — ESP32-WROOM + VS1053B GM MIDI build

This build turns Aciduino V2 (TB-303 / TR-808 step sequencer) into a self-contained
groovebox: the sequencer drives a **VS1053B** General-MIDI module (which has its own
audio output jack), controlled by a **rotary encoder + 4×4 keypad** with a **1.3"
SH1106 OLED**. A 5-pin **MIDI DIN OUT** and **USB serial MIDI** are also available so
external gear can be sequenced at the same time (dual output by routing tracks to
different ports).

Select this build in `AciduinoV2.ino`:
```cpp
#include "src/ports/esp32/wroom_vs1053.h"
```

## Arduino IDE setup
- **Board:** ESP32 Dev Module (esp32 by Espressif core).
- **Libraries:** `U8g2` (Oliver Kraus). MIDI + uCtrl + uClock are bundled as
  submodules under `src/` — run `git submodule update --init` once after cloning.
- The VS1053 driver and keypad scanner are in `src/devices/` (no extra libraries).

## Wiring (ESP32-WROOM-32)

| Block | Signal → ESP32 GPIO |
|---|---|
| **SH1106 OLED (I2C)** | SDA→21, SCL→22, VCC→3V3, GND→GND (addr 0x3C) |
| **VS1053B (VSPI)** | SCK→18, MISO(SO)→19, MOSI(SI)→23 |
| **VS1053B control** | XCS→5, XDCS→33, DREQ→34, XRESET→32 |
| **VS1053B power** | 5V→VIN (board has its own regulator), GND common |
| **Rotary encoder** | A→25, B→26 (push SW→27, currently spare) |
| **Keypad via CD74HC4067** | S0→4, S1→15, S2→13, S3→12, SIG→35 |
| **CD74HC4067 power** | VCC→3V3, GND→GND, EN→GND |
| **MIDI DIN OUT** | Serial2 TX→17 → MIDI jack (3.3V opto/220Ω network) |
| **USB serial MIDI** | over the USB cable (Serial / TX0=1, RX0=3) |
| **BPM LED** | onboard blue LED (GPIO2) |

Notes:
- VS1053B is 3.3V logic → wire **directly**, no level shifter. Audio comes out of the
  VS1053 board's own 3.5mm jack.
- **GPIO35 (mux SIG) has no internal pull-up** → add an external **10kΩ pull-up to
  3V3** on the SIG line. Each keypad key shorts its mux channel to GND (pressed = LOW).
- Wire the 16 keypad keys (as 16 individual buttons: one side to a 4067 channel Y0–Y15,
  the other side common to GND) — *not* as a scanned row/column matrix.
- Strapping pins 12/15 are used as mux select lines; if USB flashing ever gets flaky,
  move S2 off GPIO12.

## Keypad map (default — edit `src/devices/mux_keypad.h`)

Keys map by **mux channel** Y0…Y15. Default order assumes the keypad face read
left→right, top→bottom (`1 2 3 A / 4 5 6 B / 7 8 9 C / * 0 # D`):

| Key | Function | Key | Function |
|---|---|---|---|
| 1 | Page/F1 | A | Shift |
| 2 | Up | B | Play/Stop |
| 3 | Page/F2 | C | Prev track |
| 4 | Left | * | Next track |
| 5 | Down | 0 | Panic (all-notes-off) |
| 6 | Right | # | Shift (dup) |
| 7 | Value − | D | Play/Stop (dup) |
| 8 | Rec | | |
| 9 | Value + | | |

- **Rotary encoder** = value −/+ (the comfortable changer); turn to edit the selected
  parameter. Hold **Shift** + Value−/+ to change the selected track.
- Re-order `muxKeyMap[]` to match however you physically wire keys to the 4067.

## Routing audio to the VS1053 (dual output)

On the **System** page, each track has an output **port** selector. After this build,
the ports are: `midi1` (USB), `midi2` (DIN out), `midi3` (**VS1053B**, on-board audio).
- Route 303 tracks to **midi3** to hear them on the VS1053's jack.
- Route the 808 track to **midi3** and set its channel to **10** for GM drums.
- Route other tracks to `midi1`/`midi2` to play external gear simultaneously.

At boot the firmware sends sensible GM programs to the VS1053 (Synth Bass 1 on
channels 1–4, drum kit on channel 10); change instruments live with Program Change.

## Bring-up tips
1. Confirm the OLED first (I2C scan should find 0x3C).
2. The firmware verifies VS1053 real-time MIDI mode internally (`SCI_AUDATA == 0xAC45`).
   If you get no sound: check XRESET is driven high, DREQ is connected, and SPI wiring.
3. Press **0** (Panic) if a note ever hangs.

## Roadmap (Phase 2, not built yet)
On-board SF2 softsynth (`MTK_Synth.sf2` → ESP32 internal DAC via TinySoundFont on
core 0), exposed as a 4th MIDI port for a second independent audio output.
