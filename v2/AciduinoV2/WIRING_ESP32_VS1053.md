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
- **Libraries:** `U8g2` (Oliver Kraus). MIDI + uCtrl + uClock are **bundled in
  `src/`** (vendored into this branch), so a plain ZIP download or clone compiles
  with no `git submodule` step required.
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

## Output routing & instrument selection

The VS1053 is plugged as the **first** MIDI port, so the unit plays through the
on-board VS1053 **out of the box** — no routing needed. The MIDI ports are:
`midi1` = **VS1053B** (on-board audio jack), `midi2` = USB serial MIDI,
`midi3` = 5-pin DIN out.

Defaults on a freshly flashed unit:
- 303 tracks 1–4 → VS1053, MIDI channels 1–4, instrument **Synth Bass 1** (GM 39).
- 808 drum track → VS1053, MIDI **channel 10**, **standard GM drum kit**.

Everything is editable on the **System** page → subpage 2 ("track config") per track:
- **out**   — output port (`midi1`/`midi2`/`midi3`).
- **channel** — MIDI channel (set to 10 for GM drums).
- **instr**  — GM instrument / Program Change (1–128). Scrolling **auditions the
  sound live**. For the drum track this selects the kit (e.g. 26 = TR-808 kit).

For **dual output**, route some tracks to `midi2`/`midi3` to drive external gear
while others play the VS1053 at the same time.

## Adjusting VS1053 sound parameters

The **MIDI** page exposes a General-MIDI control set the VS1053 responds to; these
CCs are sent to whichever port the selected track targets (the VS1053 when routed
there): **cutoff** (CC74), **reso** (CC71), **attack/decay/release** (CC73/75/72),
**volume** (CC7), **pan** (CC10), **reverb** (CC91), **chorus** (CC93),
**mod** (CC1), **express** (CC11), **sustain** (CC64). Drum track: volume, pan,
reverb, chorus. Map a control to a pot/encoder or edit its value directly.

## Bring-up tips
1. Confirm the OLED first (I2C scan should find 0x3C).
2. The firmware verifies VS1053 real-time MIDI mode internally (`SCI_AUDATA == 0xAC45`).
   If you get no sound: check XRESET is driven high, DREQ is connected, and SPI wiring.
3. Press **0** (Panic) if a note ever hangs.

## Roadmap (Phase 2, not built yet)
On-board SF2 softsynth (`MTK_Synth.sf2` → ESP32 internal DAC via TinySoundFont on
core 0), exposed as a 4th MIDI port for a second independent audio output.
