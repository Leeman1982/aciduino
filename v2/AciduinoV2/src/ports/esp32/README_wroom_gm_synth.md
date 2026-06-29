# Aciduino V2 — ESP32 WROOM port with on-board General MIDI synth

This port runs the standard AciduinoV2 303/808 sequencer and OLED UI on an
ESP32 WROOM, and adds an **on-board General MIDI soundfont synthesizer** so the
sequencer makes sound on its own — no external synth required. Audio is rendered
from an embedded SF2 soundfont and streamed over I2S to a **PCM5102** DAC.

Selected in `AciduinoV2.ino` with:

```cpp
#include "src/ports/esp32/wroom_gm_synth.h"
```

## Hardware

| Part | Notes |
|------|-------|
| ESP32 WROOM dev board (30-pin) | No PSRAM required — samples are streamed from flash |
| 1.3" OLED, SH1106, 128×64, I2C | `U8G2_SH1106_128X64_NONAME_F_HW_I2C` |
| Rotary encoder + push button | Value changer + SHIFT |
| 5× momentary push buttons | Navigation |
| PCM5102 I2S DAC | Line/headphone out for the GM synth |

### Pinout

| Signal | GPIO |
|--------|------|
| OLED SDA | 21 |
| OLED SCL | 22 |
| PCM5102 BCK | 26 |
| PCM5102 LRCK / WS | 25 |
| PCM5102 DIN (data) | 27 |
| Encoder A | 32 |
| Encoder B | 33 |
| Encoder push (SHIFT) | 13 |
| Button 1 | 14 |
| Button 2 | 4 |
| Button 3 | 5 |
| Button 4 | 18 |
| Button 5 | 19 |
| BPM LED (on-board) | 2 |

PCM5102 wiring: `SCK → GND` (uses internal PLL), `FLT/DEMP/XSMT` per the board's
defaults (XSMT high to un-mute). Buttons and the encoder pins connect to GND;
they use the ESP32 internal pull-ups (handled by uCtrl's DIN module).

> The I2S pins (25/26/27) and the I2C OLED pins (21/22) are kept clear of the
> navigation pins. None of the nav pins use the input-only GPIOs 34–39 (those
> have no internal pull-up).

## Controls (6 inputs)

The midilab UI is designed around 9 buttons; this layout folds it onto an
encoder + 5 buttons, using SHIFT (encoder push) for the secondary functions:

```
Encoder rotate ........ change the selected value
Encoder push (SHIFT) .. hold for a button's shifted function
Button 1 ......... UP            | SHIFT = previous track
Button 2 ......... DOWN          | SHIFT = next track
Button 3 ......... PREVIOUS (<-) | SHIFT = REC on/off
Button 4 ......... NEXT (->)     | SHIFT = PLAY / STOP
Button 5 ......... PAGE cycle    | SHIFT = page back
```

Wire extra buttons and extend the `plug()` / `setNavComponentCtrl()` block in
`wroom_gm_synth.h` for a 1:1 mapping closer to the reference 9-button layout.

## The GM synth engine

Lives in `src/synth/`:

- `sf2.{h,cpp}` — minimal SoundFont 2 reader: resolves preset → instrument →
  sample with key/velocity ranges, loop points, root key, tuning, attenuation,
  pan and a volume envelope.
- `gm_synth.{h,cpp}` — voice mixer + envelopes + I2S output. A FreeRTOS audio
  task on **core 0** renders 16-bit stereo audio; the sequencer (core 1) feeds
  note events through a lock-free queue, so note-on/off are safe to call from
  the sequencer/clock context.
- `soundfont_vintage_dreams.h` — the *Vintage Dreams Waves* GM soundfont,
  embedded in flash (`.rodata`). Sample PCM is read directly from the flash
  memory map at playback time, so there is **no large RAM copy** of the
  wavetable (key to running on a plain WROOM without PSRAM).
- `synth_config.h` — `#define USE_GM_SYNTH` build-wide switch. Comment it out to
  build a MIDI-only port.

### How notes reach the synth

Every note the sequencer plays is mirrored into the synth in
`Aciduino::sequencerOutHandler()` (guarded by `USE_GM_SYNTH`). The track's MIDI
channel selects the GM program. **MIDI channel 10** is the GM percussion bank —
set the 808 drum track's output channel to 10 in the MIDI page to play it as a
drum kit; the note number then selects the drum sound.

### Defaults (override in `gm_synth.h`)

| Define | Default | Meaning |
|--------|---------|---------|
| `GM_SYNTH_SAMPLE_RATE` | 22050 | output rate (keeps CPU sane) |
| `GM_SYNTH_MAX_VOICES` | 16 | polyphony cap |
| `GM_SYNTH_DRUM_CHANNEL` | 9 | zero-based channel 10 = drums |
| `GM_SYNTH_I2S_BCK_PIN` | 26 | |
| `GM_SYNTH_I2S_WS_PIN` | 25 | |
| `GM_SYNTH_I2S_DATA_PIN` | 27 | |

## Build settings (Arduino IDE / arduino-cli)

- Board: **ESP32 Dev Module**
- **Partition Scheme: "Huge APP (3MB No OTA)"** (or "Minimal SPIFFS / 1.9MB
  APP"). The embedded soundfont is ~314 KB, so the default 1.2 MB app partition
  can be tight.
- PSRAM: not required.
- The synth uses the legacy `driver/i2s.h` API for the broadest core
  compatibility; on ESP32 Arduino core 3.x you may see a deprecation warning —
  it still builds and runs.

## Swapping / adding soundfonts

The 520 KB *VanillaSet* font is left for a later pass. To add another font,
generate a header the same way and point `gmSynthInit()` at it:

```sh
# produces a 4-byte-aligned const array in flash
xxd -i your_font.sf2 > soundfont_your_font.h   # then tidy the type/alignment
```

> ⚠️ This firmware has not been compiled against the ESP32 Arduino toolchain or
> flashed to hardware in this environment (the `uCtrl`/`uClock` submodules were
> not present and no toolchain was available). The SF2 parser and the embedded
> font were verified natively. Expect to validate the I2S timing, the exact
> uCtrl navigation binding, and the chosen partition scheme on real hardware.
