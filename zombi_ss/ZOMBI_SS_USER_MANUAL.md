# ZOMBI SS — User Manual
### 4-Track GM MIDI Step Sequencer · ESP32 + VS1053B

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Hardware & Wiring](#2-hardware--wiring)
3. [Setup & Installation](#3-setup--installation)
4. [First Boot](#4-first-boot)
5. [Interface Overview](#5-interface-overview)
6. [Core Concepts](#6-core-concepts)
7. [STEP Mode — Programming Patterns](#7-step-mode--programming-patterns)
8. [CTRL Mode — Transport & Mix Controls](#8-ctrl-mode--transport--mix-controls)
9. [INST Mode — Instrument Browser](#9-inst-mode--instrument-browser)
10. [SONG Mode — Pattern Chain Editor](#10-song-mode--pattern-chain-editor)
11. [Saving & Loading](#11-saving--loading)
12. [GM Instrument Reference](#12-gm-instrument-reference)
13. [Workflow Walkthroughs](#13-workflow-walkthroughs)
14. [Quick Reference Card](#14-quick-reference-card)
15. [Troubleshooting](#15-troubleshooting)

---

## 1. Introduction

ZOMBI SS is a **standalone 4-track General MIDI step sequencer** built on an ESP32 WROOM microcontroller. It plays back General MIDI (GM) sounds in real time through a VS1053B synthesizer chip, which has its own onboard DSP — including hardware reverb — so the ESP32 is free to handle the UI and timing.

**What it does:**
- 4 independent tracks, each playing one GM instrument or one drum sound
- 16 steps per track, programmable on/off per step
- 128 GM melodic instruments + 47 GM drum/percussion sounds, fully browsable
- 16 pattern slots (P00–P15) that can each have a different BPM, different instruments, and different step sequences
- Song chains: link patterns together in any order for a complete arrangement
- All patterns and the song chain save to microSD and reload automatically on startup
- Hardware reverb via VS1053B DSP — no CPU cost, five preset levels
- Master volume control (0–100%)

**What it is not:**
- It does not play audio files or MP3s
- It does not record audio
- It is not a polyphonic keyboard; each track plays one note per step

---

## 2. Hardware & Wiring

### Parts Required

| Component | Notes |
|-----------|-------|
| ESP32 WROOM-32 dev board | Any 38-pin ESP32 DevKit works |
| VS1053B shield / breakout | The baldram ESP_VS1053_Library shield, or any VS1053B wired to the SPI pins below |
| SH1106 1.3" 128×64 OLED | The combo module that integrates the OLED and 4×4 keypad on one PCB |
| 4×4 keypad matrix | Integrated on the combo module, or wired separately |
| microSD card | FAT32 formatted; any capacity works (files are tiny) |
| 10 kΩ resistor | VS1053B RESET pullup (if your board doesn't have it onboard) |

### Wiring Table

**VS1053B → ESP32**

| VS1053B Pin | ESP32 GPIO | Notes |
|-------------|-----------|-------|
| SCK | 18 | SPI clock (shared with SD) |
| MISO | 19 | SPI data in (shared with SD) |
| MOSI | 23 | SPI data out (shared with SD) |
| XCS (chip select) | 5 | VS1053B command CS |
| XDCS (data chip select) | 17 | VS1053B data CS |
| DREQ (data request) | 34 | Input only — do NOT drive this pin |
| !RST (reset) | — | Connect through 10 kΩ to 3.3 V; no GPIO needed |
| XSMT (soft mute) | — | Connect directly to 3.3 V (unmuted always) |
| 5V / VIN | VIN | Shield draws ~100 mA; use USB power |
| GND | GND | |

**SD Card → ESP32**

| SD Pin | ESP32 GPIO |
|--------|-----------|
| CS | 16 |
| SCK | 18 (shared) |
| MISO | 19 (shared) |
| MOSI | 23 (shared) |

**OLED + Keypad combo module → ESP32**

| Module Pin | ESP32 GPIO |
|------------|-----------|
| SDA | 21 |
| SCL | 22 |
| VCC | 3.3 V |
| GND | GND |
| Row R1 | 13 |
| Row R2 | 14 |
| Row R3 | 25 |
| Row R4 | 4 |
| Col C1 | 26 |
| Col C2 | 27 |
| Col C3 | 32 |
| Col C4 | 33 |

> **PCM5102 note:** ZOMBI SS uses the VS1053B for audio output — you do **not** need a PCM5102 or any I2S DAC. Audio comes from the VS1053B's 3.5 mm headphone jack or onboard amplifier.

---

## 3. Setup & Installation

### 3.1 Install Libraries (Arduino IDE)

Search for and install all four in **Tools → Manage Libraries**:

| Library | Author | Search term |
|---------|--------|-------------|
| ESP_VS1053_Library | baldram | `ESP VS1053` |
| U8g2 | Oliver Kraus | `U8g2` |
| Keypad | Mark Stanley & Alexander Brevig | `Keypad` |
| SD | bundled with ESP32 Arduino core | (pre-installed) |

### 3.2 Board Settings

In **Tools**:

| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Partition Scheme | Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS) |
| Flash Frequency | 80 MHz |
| Upload Speed | 921600 |
| CPU Frequency | 240 MHz (default) |

### 3.3 SD Card Preparation

1. Format the microSD card as **FAT32**
2. The card can be empty — ZOMBI SS creates its own files the first time you save
3. Insert the card before powering on; hot-swapping is not supported

### 3.4 Compile & Flash

Open `zombi_ss/ZombiSS/ZombiSS.ino` in Arduino IDE, select the correct port, and click **Upload**. Compilation takes 20–30 seconds. Watch Serial Monitor at 115200 baud for boot messages.

---

## 4. First Boot

When power is applied:

1. **Splash screen** appears for 2.5 seconds showing:
   ```
         ZOMBI
           SS
     GM SEQUENCER v2.0
      ESP32 + VS1053B
   ```

2. Serial monitor shows the boot sequence:
   ```
   === ZOMBI SS BOOT ===
   [vs] VS1053 RTMIDI ready
   [seq] Sequencer ready
   [stor] SD mounted
   [stor] Loaded P00 from /P00.ZSS   ← only on subsequent boots if saves exist
   ```

3. After the splash, the **STEP screen** appears for Pattern P00, Track T1.

> **If the SD card is not found:** A message is printed to Serial (`SD not found — patterns will not persist`), but the sequencer works normally. All patterns are still editable and playable — they just won't be remembered after power-off.

---

## 5. Interface Overview

### 5.1 The OLED Display (128×64 pixels)

The bottom 8 pixels of the display always show the **four mode tabs**:

```
┌────────────────────────────────┐
│  STEP  │  CTRL  │  INST  │ SONG │  ← current mode shown solid/inverted
└────────────────────────────────┘
```

The remaining 56 pixels show the current mode's screen. Each mode has its own layout, described in detail in Sections 7–10.

### 5.2 The 4×4 Keypad

The physical keypad is labelled:

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │
└───┴───┴───┴───┘
```

The same 16 physical buttons do **different things in each mode**. The active mode is always shown in the tab bar at the bottom of the OLED.

### 5.3 Navigating Between Modes

Press **D** (bottom-right key) to advance to the next mode. The cycle is:

```
STEP → CTRL → INST → SONG → STEP → ...
```

In STEP mode, **hold D for 400 ms** to advance (a short tap on D toggles step 16 instead). In all other modes, a short tap on D advances immediately.

### 5.4 Key Events

The keypad distinguishes three events per key:

| Event | What it means |
|-------|--------------|
| **PRESSED** | Key just pushed down |
| **HOLD** | Key held for 400 ms or more |
| **RELEASED** | Key let go |

Most actions trigger on PRESSED. The D-key in STEP mode is the only key that uses HOLD and RELEASED together to provide dual functionality.

---

## 6. Core Concepts

### 6.1 Tracks

ZOMBI SS has **4 tracks** (T1–T4). Each track is independent and can play any GM instrument or any drum sound. By default:

| Track | Channel | Default Instrument |
|-------|---------|-------------------|
| T1 | CH1 | Acoustic Grand Piano (GM 1) |
| T2 | CH2 | Finger Bass (GM 34) |
| T3 | CH3 | Square Lead (GM 81) |
| T4 | CH10 (drums) | Bass Drum 1 |

CH10 is the **GM percussion channel**. T4 is a drum track by default. On a drum track, you pick a specific drum *note* (e.g. snare, hi-hat, kick) rather than a GM program number.

### 6.2 Steps

Each track has **16 steps**. Each step can be either **active** (plays a note) or **inactive** (silence). All active steps on a track play the same note at the same velocity (default 100). The sequencer plays all 16 steps in sequence, then loops.

### 6.3 BPM

Tempo is stored **per pattern** (not globally). Range is 40–240 BPM. The default for new patterns is 120 BPM.

### 6.4 Patterns

A pattern contains all four tracks (with their step data, instrument assignments, and notes), plus a BPM. There are **16 pattern slots** (P00–P15). You can switch between patterns in SONG mode. When you save a pattern, it writes only that one slot to the SD card.

### 6.5 Song Chains

A song chain is an ordered list of up to **64 pattern slot references**. For example:

```
P00 → P01 → P02 → P01 → P03 → P00
```

When song mode is active and the sequencer is playing, it advances to the next chained pattern automatically each time the current pattern completes its 16 steps. This lets you build a complete arrangement from your individual pattern blocks.

### 6.6 Note Value

Each track has one **base note** (a MIDI note number 0–127, displayed as e.g. `C5` or `A#3`). Every active step on that track plays that note. You change the note in CTRL mode (keys `9`/`C`) or more visually in INST mode.

For a **drum track**, "note" is actually the GM percussion note number (35–81), which selects the drum sound. Adjusting note on a drum track changes which drum plays.

---

## 7. STEP Mode — Programming Patterns

### 7.1 What You See

```
T1 Ac.Piano             120
────────────────────────────
┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐
│██│└──┘└──┘│██││██│└──┘└──┘└──┘  ← Steps 1-8
└──┘                               ← ▲ playhead arrow
┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐
│██│└──┘└──┘└──┘│██│└──┘└──┘│██│  ← Steps 9-16
       ▲ playhead arrow (row 2)
┌──┬──┬──┬──┐        SNG P02/08
│T1│T2│T3│T4│                      ← Track mini-tabs
└──┴──┴──┴──┘
[STEP][CTRL][INST][SONG]            ← Mode tabs
```

- **Header:** Track label (`T1`), instrument name, BPM (right-aligned)
- **Step grid:** 2 rows of 8 blocks. Filled blocks = active steps. Hollow = inactive.
- **Playhead:** A 3-pixel triangle that moves under the current step while playing
- **Track mini-tabs:** Shows which track is selected (inverted); `D` suffix = drum track
- **Slot info:** Shows current pattern (`PAT 00/15`) or song position (`SNG P02/08`) when song mode is on

### 7.2 Key Map — STEP Mode

The 16 keys directly map to the 16 steps in **reading order** (left-to-right, top-to-bottom):

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  →  Steps  1,  2,  3,  4
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  →  Steps  5,  6,  7,  8
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  →  Steps  9, 10, 11, 12
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  →  Steps 13, 14, 15, 16
└───┴───┴───┴───┘
```

Press any key to **toggle** that step on/off. If the step was off, it turns on (filled block). If it was on, it turns off (hollow block).

**D key special behaviour:**
- **Short press (release < 400 ms):** toggles step 16 (bottom-right)
- **Hold (≥ 400 ms):** advances to CTRL mode (step 16 is NOT toggled)

### 7.3 Selecting a Different Track

To work on a different track, switch to **CTRL mode** (tap D and hold until mode changes, or short-tap D when already in another mode). In CTRL mode, keys `1/2/3/A` select tracks T1–T4. Then tap D again to return to STEP mode.

> **Tip:** The track mini-tabs at the bottom of the STEP screen show which track you are editing. You can see at a glance which tracks are drum tracks (they show a `D` suffix, e.g. `T4D`).

### 7.4 While the Sequencer is Playing

The step grid updates in real time. The playhead arrow (▲) sweeps across steps 1–8 on the top row, then 9–16 on the bottom row. You can **edit steps while playing** — changes take effect immediately.

---

## 8. CTRL Mode — Transport & Mix Controls

### 8.1 What You See

```
CTRL P:00  BPM:120
────────────────────────────
┌ T1 ┐┌ T2 ┐┌ T3 ┐┌ T4 ┐    ← Track selector (current inverted)
└────┘└────┘└────┘└────┘
GM: 1  NOTE:C5  CH:1         ← Track detail (melodic track)
  — or —
DRM:036  Bass Drum 1         ← Track detail (drum track)

V [████████░░░░░░░░]  80%    ← Master volume bar
R [████████░░░░░░░░]  50%    ← Reverb bar
                      [STP]  ← Play state hint
[STEP][CTRL][INST][SONG]
```

### 8.2 Key Map — CTRL Mode

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  T1    T2    T3    T4     ← Select active track
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  Play  Stop  Clear  Save  ← Transport & edit
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  BPM-  BPM+  Note-  Note+ ← Tempo & pitch
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  Vol-  Vol+  Rev+   Mode  ← Mix & navigation
└───┴───┴───┴───┘
```

### 8.3 Function Details

**Track Selection — Row 1**

| Key | Action |
|-----|--------|
| `1` | Select Track 1 (T1) |
| `2` | Select Track 2 (T2) |
| `3` | Select Track 3 (T3) |
| `A` | Select Track 4 (T4) |

The selected track is highlighted in the track selector boxes. Its instrument name, note, and channel appear in the detail line below.

**Transport — Row 2**

| Key | Action |
|-----|--------|
| `4` | **Play / Stop toggle.** If stopped, starts from step 1. If playing, stops and resets to step 1. |
| `5` | **Stop + Panic.** Stops playback AND sends All Notes Off / All Sound Off to all 16 MIDI channels. Use this if a note gets stuck. |
| `6` | **Clear track.** Erases all 16 steps for the currently selected track. The track still exists with its instrument — it just plays nothing. |
| `B` | **Save.** Writes the current pattern slot to SD card and saves the song chain. See Section 11. |

**Tempo & Pitch — Row 3**

| Key | Action | Range |
|-----|--------|-------|
| `7` | BPM −5 | 40–240 BPM |
| `8` | BPM +5 | 40–240 BPM |
| `9` | Base note −1 semitone | 0–127 MIDI (or 35–81 for drums) |
| `C` | Base note +1 semitone | 0–127 MIDI (or 35–81 for drums) |

> **Tip:** BPM changes take effect immediately, even while playing. The hardware timer is reprogrammed on the fly with no audible glitch.

> **Note on `9`/`C` for drum tracks:** On a drum track (CH10), these keys change the *drum sound*, not a musical pitch. Note 35 = Acoustic Bass Drum, 36 = Bass Drum 1, 38 = Acoustic Snare, etc. See the drum reference in Section 12.

**Mix — Row 4**

| Key | Action | Range |
|-----|--------|-------|
| `*` | Master volume −10% | 0–100% |
| `0` | Master volume +10% | 0–100% |
| `#` | **Reverb cycle** — steps through 5 preset levels | 0, 25%, 50%, 75%, 100% |
| `D` | Advance to next mode | |

**Reverb presets (cycling with `#`):**

| Press count | Level | Description |
|-------------|-------|-------------|
| 0 (default) | 50% (CC91=64) | Moderate room reverb |
| 1 | 75% (CC91=96) | Large room |
| 2 | 100% (CC91=127) | Maximum hall reverb |
| 3 | 0% (CC91=0) | No reverb (dry) |
| 4 | 25% (CC91=32) | Subtle reverb |
| (wraps) | 50% again | |

Reverb is applied globally to all 16 MIDI channels simultaneously. It uses the VS1053B's onboard DSP and costs zero ESP32 CPU.

---

## 9. INST Mode — Instrument Browser

### 9.1 What You See

```
INST T1 (GM)
────────────────────────────

      Ac.Piano              ← Large instrument name (7×14B font)

      GM 1 / 128            ← Number indicator

[-10][-1][+1][+10] PNO BAS SYN STR
[D]=NEXT MODE      BRS PAD CHR SFX
[STEP][CTRL][INST][SONG]
```

For a drum track the header changes to `(DRUMS)` and the number shows `NOTE 36 / 81`.

### 9.2 Melodic Track Browsing

When the active track is **not** on CH10:

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  GM−10  GM−1  GM+1  GM+10   ← Fine/coarse scroll
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  Piano  Bass  Lead  Strings  ← Bank jump
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  Brass  Pads  Choir  SFX     ← Bank jump
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │   —      —    —    Mode       ← Navigation
└───┴───┴───┴───┘
```

**Scroll keys:**

| Key | Action |
|-----|--------|
| `1` | Go back 10 instruments |
| `2` | Go back 1 instrument |
| `3` | Go forward 1 instrument |
| `A` | Go forward 10 instruments |

Both ends clamp — pressing `1` at GM 1 stays at GM 1; pressing `A` at GM 128 stays at GM 128.

**Bank jump keys (melodic):**

| Key | Jumps to... | GM Number | First instrument |
|-----|-------------|-----------|-----------------|
| `4` | Piano group | GM 1 | Acoustic Grand Piano |
| `5` | Bass group | GM 33 | Acoustic Bass |
| `6` | Lead Synths | GM 81 | Square Lead |
| `B` | Strings | GM 49 | Strings 1 |
| `7` | Brass | GM 57 | Trumpet |
| `8` | Pads | GM 89 | New Age Pad |
| `9` | Choir | GM 53 | Choir Aahs |
| `C` | SFX | GM 121 | Fret Noise |

> **Example:** You want a bass line on T2. Press `2` in CTRL mode to select T2, then tap `D` twice to reach INST mode. Press `5` to jump directly to the Bass group (Acoustic Bass, GM 33). Use `2`/`3` to step through Finger Bass, Pick Bass, Fretless, Slap Bass, etc.

### 9.3 Drum Track Browsing

When the active track is on **CH10** (drum channel):

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  −5    −1    +1    +5       ← Step through drum notes
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  Kick  Snare  HH  FlrTom    ← Family jump
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │ Crash Cowbel Conga Claves   ← Family jump
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │   —     —     —    Mode      ← Navigation
└───┴───┴───┴───┘
```

**Scroll keys (drums):**

| Key | Action |
|-----|--------|
| `1` | Go back 5 drum notes |
| `2` | Go back 1 drum note |
| `3` | Go forward 1 drum note |
| `A` | Go forward 5 drum notes |

Range is clamped to MIDI notes 35–81 (the 47 standard GM percussion notes).

**Family jump keys (drums):**

| Key | Drum sound | MIDI note |
|-----|-----------|-----------|
| `4` | Bass Drum 1 | 36 |
| `5` | Acoustic Snare | 38 |
| `6` | Closed Hi-Hat | 42 |
| `B` | Low Floor Tom | 45 |
| `7` | Crash Cymbal 1 | 49 |
| `8` | Cowbell | 56 |
| `9` | Open Hi Conga | 63 |
| `C` | Claves | 75 |

> **Drum pattern tip:** ZOMBI SS gives you four tracks, so you can run four independent drum parts — for example: T1 = kick, T2 = snare, T3 = hi-hat, T4 = cowbell. Set all four tracks to CH10 and use INST mode to assign a different drum note to each.

> **Important:** The instrument assignment is per-track and per-pattern. Changing T1's instrument in P00 does not affect T1's instrument in P01.

### 9.4 Instrument Changes While Playing

Program change messages are sent to the VS1053B **immediately** when you scroll in INST mode. You will hear the instrument change in real time as the sequencer continues playing.

---

## 10. SONG Mode — Pattern Chain Editor

### 10.1 What You See

```
SONG P:02 CHN  [5]              ← Slot, mode (CHN=chain, SGL=single), chain length
────────────────────────────
┌P00┐┌P01┐┌P02┐┌P03┐           ← Row 1: slots 0-3
└───┘└───┘└───┘└───┘
┌P04┐┌P05┐┌P06┐┌P07┐           ← Row 2: slots 4-7
└───┘└───┘└───┘└───┘
┌P08┐┌P09┐┌P10┐┌P11┐           ← Row 3: slots 8-11
└───┘└───┘└───┘└───┘
────────────────────────────
P00>P01>P02>P01>P03...          ← Chain visualiser (up to 8 entries shown)
[SAV]save [LNK]link [DEL]del
[STEP][CTRL][INST][SONG]
```

The currently active pattern slot (P02 in this example) is shown with a filled/inverted box.

### 10.2 Key Map — SONG Mode

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  Slot 0  Slot 1  Slot 2  Slot 3
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  Slot 4  Slot 5  Slot 6  Slot 7
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  Slot 8  Slot 9  Slot 10 Slot 11
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  Save  SongMode  DelLast  Mode
└───┴───┴───┴───┘
```

### 10.3 Loading a Pattern Slot

Press any of keys `1`–`C` (the 12 pattern keys, slots 0–11) to:
1. **Append** that slot to the end of the song chain
2. **Navigate** to that slot (makes it the active pattern you are viewing/editing)

> **Note:** Only 12 of the 16 slots (P00–P11) are directly accessible from SONG mode's keypad. To access P12–P15, use CTRL mode's Save button after switching slots via the song chain, or expand this mapping in future firmware.

### 10.4 Building a Song Chain

A song chain is built by pressing pattern keys in the order you want them to play. The chain is appended one entry at a time.

**Example:** To create the arrangement `P00 → P01 → P00 → P02 → P01`:

1. Enter SONG mode
2. Press `1` → adds P00, navigates to P00
3. Press `2` → adds P01, navigates to P01
4. Press `1` → adds P00 again, navigates to P00
5. Press `3` → adds P02, navigates to P02
6. Press `2` → adds P01, navigates to P01

The chain visualiser shows: `P00>P01>P00>P02>P01`

The chain can hold up to **64 entries**. The same slot can appear multiple times.

### 10.5 Deleting from the Chain

`#` — removes the **last entry** from the chain. Press repeatedly to unwind the chain from the end.

### 10.6 Toggling Song Mode

`0` — toggles song mode **on/off**.

- **Song mode ON** (header shows `CHN`): When playing, the sequencer automatically advances to the next chained pattern each time the current pattern completes its 16 steps.
- **Song mode OFF** (header shows `SGL`): The active pattern loops indefinitely. The chain is preserved but not followed.

Song mode state is shown in the SONG screen header and also in the STEP screen's slot info area (shows `SNG P02/08` when active).

### 10.7 Saving

`*` — saves the **current active pattern** to SD and saves the **full song chain** to SD. This is identical to pressing `B` in CTRL mode but is conveniently accessible while in SONG mode.

> **Remember to save before power-off.** The sequencer does not auto-save. Unsaved patterns and chain changes are lost when power is removed.

---

## 11. Saving & Loading

### 11.1 What Gets Saved

| File on SD | Contents | Triggered by |
|------------|---------|--------------|
| `/P00.ZSS` | Pattern slot 0 (all 4 tracks, BPM, instruments, steps) | `B` in CTRL or `*` in SONG |
| `/P01.ZSS` – `/P15.ZSS` | Pattern slots 1–15 (same format) | Same, when that slot is active |
| `/SONG.ZSS` | Full song chain (up to 64 entries) | `B` in CTRL or `*` in SONG |

Pattern slots are saved **individually** — saving P00 does not overwrite P01. If you want to save all 16 slots, you need to visit each one and press Save. A future workflow tip: edit all your patterns first, then cycle through slots saving each.

### 11.2 Auto-Load on Boot

At startup, after the VS1053B is initialised, ZOMBI SS reads every existing `.ZSS` pattern file from the SD card. Missing files are silently skipped — if you only have P00.ZSS and P03.ZSS, those two slots are loaded and the rest start fresh with default instruments.

The song chain is loaded from `/SONG.ZSS` if it exists.

### 11.3 File Format

Files use a simple binary format with a 4-byte magic header:

- Pattern files: magic bytes `ZSSP` followed by the raw `Pattern` struct
- Song chain file: magic bytes `ZSSG` followed by the raw `SongChain` struct

These files are small (each pattern is under 200 bytes). The entire 16-slot set plus song chain fits in under 4 KB.

### 11.4 BPM and Pattern Length Clamping

On load, BPM values outside 40–240 are clamped to the valid range. Pattern lengths outside 1–16 are clamped to 1–16. This protects against corrupted SD data.

---

## 12. GM Instrument Reference

### 12.1 Melodic Instruments (GM 1–128)

GM instruments are numbered 1–128 in standard MIDI documentation. ZOMBI SS displays them as `GM 1 / 128` through `GM 128 / 128`.

**Piano (GM 1–8)**
1 Acoustic Grand Piano · 2 Bright Piano · 3 Electric Grand Piano · 4 Honky-Tonk · 5 Electric Piano 1 · 6 Electric Piano 2 · 7 Harpsichord · 8 Clavinet

**Chromatic Percussion (GM 9–16)**
9 Celesta · 10 Glockenspiel · 11 Music Box · 12 Vibraphone · 13 Marimba · 14 Xylophone · 15 Tubular Bells · 16 Dulcimer

**Organ (GM 17–24)**
17 Drawbar Organ · 18 Percussive Organ · 19 Rock Organ · 20 Church Organ · 21 Reed Organ · 22 Accordion · 23 Harmonica · 24 Tango Accordion

**Guitar (GM 25–32)**
25 Nylon Guitar · 26 Steel Guitar · 27 Jazz Guitar · 28 Clean Guitar · 29 Muted Guitar · 30 Overdriven Guitar · 31 Distortion Guitar · 32 Guitar Harmonics

**Bass (GM 33–40)** ← `5` bank jump
33 Acoustic Bass · 34 Finger Bass · 35 Pick Bass · 36 Fretless Bass · 37 Slap Bass 1 · 38 Slap Bass 2 · 39 Synth Bass 1 · 40 Synth Bass 2

**Strings (GM 41–48)** ← `B` bank jump (jumps to 49)
41 Violin · 42 Viola · 43 Cello · 44 Contrabass · 45 Tremolo Strings · 46 Pizzicato Strings · 47 Orchestral Harp · 48 Timpani

**Ensemble Strings (GM 49–56)**
49 Strings 1 · 50 Strings 2 · 51 Synth Strings 1 · 52 Synth Strings 2 · 53 Choir Aahs · 54 Voice Oohs · 55 Synth Voice · 56 Orchestra Hit

**Brass (GM 57–64)** ← `7` bank jump
57 Trumpet · 58 Trombone · 59 Tuba · 60 Muted Trumpet · 61 French Horn · 62 Brass Section · 63 Synth Brass 1 · 64 Synth Brass 2

**Reed / Wind (GM 65–80)**
65 Soprano Sax · 66 Alto Sax · 67 Tenor Sax · 68 Baritone Sax · 69 Oboe · 70 English Horn · 71 Bassoon · 72 Clarinet · 73 Piccolo · 74 Flute · 75 Recorder · 76 Pan Flute · 77 Bottle Blow · 78 Shakuhachi · 79 Whistle · 80 Ocarina

**Lead Synths (GM 81–88)** ← `6` bank jump
81 Square Lead · 82 Sawtooth Lead · 83 Calliope Lead · 84 Chiff Lead · 85 Charang · 86 Voice Lead · 87 5ths Lead · 88 Bass + Lead

**Pads (GM 89–96)** ← `8` bank jump
89 New Age Pad · 90 Warm Pad · 91 Polysynth · 92 Choir Pad · 93 Bowed Glass · 94 Metal Pad · 95 Halo Pad · 96 Sweep Pad

**FX / Atmosphere (GM 97–104)**
97 Rain FX · 98 Soundtrack · 99 Crystal · 100 Atmosphere · 101 Brightness · 102 Goblins · 103 Echoes · 104 Sci-Fi

**Ethnic (GM 105–112)**
105 Sitar · 106 Banjo · 107 Shamisen · 108 Koto · 109 Kalimba · 110 Bagpipe · 111 Fiddle · 112 Shanai

**Percussive (GM 113–120)**
113 Tinkle Bell · 114 Agogo · 115 Steel Drums · 116 Woodblock · 117 Taiko Drum · 118 Melodic Tom · 119 Synth Drum · 120 Reverse Cymbal

**SFX (GM 121–128)** ← `C` bank jump
121 Fret Noise · 122 Breath Noise · 123 Seashore · 124 Bird Tweet · 125 Telephone Ring · 126 Helicopter · 127 Applause · 128 Gunshot

### 12.2 GM Drum Notes (CH10, notes 35–81)

These are available on any track set to CH10 (drum channel). INST mode's browse buttons give quick access to families.

| Note | Name | Jump key |
|------|------|----------|
| 35 | Acoustic Bass Drum | — |
| 36 | Bass Drum 1 | `4` |
| 37 | Side Stick | — |
| 38 | Acoustic Snare | `5` |
| 39 | Hand Clap | — |
| 40 | Electric Snare | — |
| 41 | Low Floor Tom | — |
| 42 | Closed Hi-Hat | `6` |
| 43 | High Floor Tom | — |
| 44 | Pedal Hi-Hat | — |
| 45 | Low Tom | `B` |
| 46 | Open Hi-Hat | — |
| 47 | Low-Mid Tom | — |
| 48 | Hi-Mid Tom | — |
| 49 | Crash Cymbal 1 | `7` |
| 50 | High Tom | — |
| 51 | Ride Cymbal 1 | — |
| 52 | Chinese Cymbal | — |
| 53 | Ride Bell | — |
| 54 | Tambourine | — |
| 55 | Splash Cymbal | — |
| 56 | Cowbell | `8` |
| 57 | Crash Cymbal 2 | — |
| 58 | Vibraslap | — |
| 59 | Ride Cymbal 2 | — |
| 60 | Hi Bongo | — |
| 61 | Low Bongo | — |
| 62 | Mute Hi Conga | — |
| 63 | Open Hi Conga | `9` |
| 64 | Low Conga | — |
| 65 | Hi Timbale | — |
| 66 | Low Timbale | — |
| 67 | Hi Agogo | — |
| 68 | Low Agogo | — |
| 69 | Cabasa | — |
| 70 | Maracas | — |
| 71 | Short Whistle | — |
| 72 | Long Whistle | — |
| 73 | Short Guiro | — |
| 74 | Long Guiro | — |
| 75 | Claves | `C` |
| 76 | Hi Wood Block | — |
| 77 | Low Wood Block | — |
| 78 | Mute Cuica | — |
| 79 | Open Cuica | — |
| 80 | Mute Triangle | — |
| 81 | Open Triangle | — |

---

## 13. Workflow Walkthroughs

### 13.1 Building Your First Beat

**Goal:** Classic 4/4 kick-snare-hihat drum pattern on tracks 1–3.

1. Power on. The STEP screen shows T1 selected.

2. **Set T1 to kick drum.**
   - Hold `D` to go to CTRL mode. Press `1` (select T1).
   - Tap `D` to go to INST mode. T1 defaults to CH1 (melodic).
   - In CTRL mode, you'd normally need to change the channel — but for this walkthrough, T4 is already a drum track (CH10). Let's use T4 for the kick instead.

3. **Switch to T4 (kick).**
   - In CTRL mode, press `A` (select T4). T4 is already on CH10.
   - Press `D` to go to INST mode. Header reads `INST T4 (DRUMS)`.
   - Press `4` to jump directly to Bass Drum 1 (note 36).
   - Press `D` to go to SONG mode. Press `D` again to return to STEP mode.

4. **Program the kick (T4).**
   - In STEP mode, T4 is selected. Press `1`, `5`, `9`, `0` to toggle steps 1, 5, 9, 13 (the four quarter-note beats). Filled blocks confirm the steps are on.

5. **Add a second drum track for snare.**
   - Go to CTRL mode. Select T3 (press `3`).
   - Go to INST mode. Press `5` to jump to Acoustic Snare (note 38).
   - Go back to STEP mode. T3 selected. Press `5`, `0` to activate steps 5 and 13 (beats 2 and 4 — the backbeat).

6. **Add hi-hat on T2.**
   - CTRL mode → press `2` (T2). INST mode → T2 is melodic (CH2). 
   - To make T2 a drum track: this requires changing the channel in `config.h` or using a second drum track approach. By default T4 is the only drum track; T1–T3 are melodic.
   - **Alternative:** Use T4 for hi-hat instead, and accept that the kick is on a melodic track with a melodic instrument set to Taiko Drum or Synth Drum (GM 119). Or use T1 for the kick by setting its instrument to Taiko Drum.

7. **Start playback.**
   - CTRL mode → press `4` (Play). Return to STEP mode (hold `D`).

### 13.2 Programming a Bass Line

**Goal:** A bass line on T2 using Finger Bass.

1. CTRL mode → press `2` (select T2).
2. INST mode → T2 is on CH2 by default. Press `5` to jump to Bass group (GM 33, Acoustic Bass). Press `3` once → GM 34 Finger Bass. Large name confirms: `Finger Bass`.
3. Back to CTRL mode. Use `9`/`C` to set the note. Default is C3 (MIDI 36). For a standard bass line root note, C2 (MIDI 24) might sound better — press `9` twelve times, or use `7` key: no, `7` changes BPM. Use `9` to go down.
   - Actually: `9` = note −1, `C` = note +1. Press `9` twelve times to go from C3 to C2 if needed.
4. STEP mode → T2 selected. Program your bass pattern. For a simple pattern: press `1`, `3`, `9`, `C` to activate steps 1, 3, 9, 11.
5. For a different note on a particular step, T4 would need a second sequencer track with a different note — ZOMBI SS plays one note per track. For melodic variation, use multiple tracks with different notes.

### 13.3 Building a Multi-Pattern Song

**Goal:** Intro (P00), Verse (P01), Chorus (P02), Outro (P00 again).

1. **Build Pattern P00 (intro).**
   - Program your four tracks in STEP and CTRL modes.
   - Set BPM with `7`/`8` in CTRL mode (e.g. 128 BPM).
   - CTRL mode → press `B` to save P00.

2. **Switch to P01.**
   - SONG mode → press `2` (appends P01 to chain AND navigates to P01).
   - The display now shows `PAT 01/15` and all tracks are fresh (default instruments, empty steps).
   - Program your verse pattern. Save with CTRL → `B`.

3. **Switch to P02 (chorus).**
   - SONG mode → press `3` (navigates to P02).
   - Program chorus. Save.

4. **Build the song chain.**
   - SONG mode → start fresh (press `#` to delete entries if the chain already has entries from step 2/3).
   - Build the arrangement: press `1` (P00 = intro), `2` (P01), `2` (P01 again), `3` (P02), `2` (P01), `3` (P02), `1` (P00 = outro).
   - Chain shows: `P00>P01>P01>P02>P01>P02>P00`

5. **Enable song mode and save.**
   - Press `0` to toggle song mode ON. Header shows `CHN`.
   - Press `*` to save.

6. **Start playback.**
   - CTRL mode → press `4`. The sequencer plays P00, then automatically transitions to P01 when P00's 16 steps complete, and so on through the chain. After the last entry (P00), it loops back to the beginning of the chain.

### 13.4 Adjusting Reverb for Atmosphere

- No reverb (dry, punchy): `#` in CTRL mode until `R [░░░░░░░░░░░░░░░░]  0%`
- Subtle room: cycle once → `25%`
- Default (medium room): start state → `50%`
- Dramatic hall reverb: cycle to `75%` or `100%`
- Synth pads and leads often sound best with 75–100% reverb
- Drums and bass usually work better with 0–25% reverb

---

## 14. Quick Reference Card

### Mode Navigation
```
D (hold 400ms in STEP) or D (tap in other modes) → advance mode
Cycle: STEP → CTRL → INST → SONG → STEP → ...
```

### STEP Mode
```
┌─1─┬─2─┬─3─┬─A─┐
│ S1│ S2│ S3│ S4│  Press to toggle steps 1-4
├─4─┼─5─┼─6─┼─B─┤
│ S5│ S6│ S7│ S8│  Press to toggle steps 5-8
├─7─┼─8─┼─9─┼─C─┤
│ S9│S10│S11│S12│  Press to toggle steps 9-12
├─*─┼─0─┼─#─┼─D─┤
│S13│S14│S15│S16│  * 0 # toggle 13-15
└───┴───┴───┴───┘  D: short tap=S16, hold=MODE
```

### CTRL Mode
```
┌─1─┬─2─┬─3─┬─A─┐
│ T1│ T2│ T3│ T4│  Select track
├─4─┼─5─┼─6─┼─B─┤
│PLY│STP│CLR│SAV│  Play/Stop · Stop+Panic · Clear · Save
├─7─┼─8─┼─9─┼─C─┤
│B- │B+ │N- │N+ │  BPM −5 / +5 · Note −1 / +1
├─*─┼─0─┼─#─┼─D─┤
│V- │V+ │REV│MOD│  Vol −10% / +10% · Reverb cycle · Mode
└───┴───┴───┴───┘
```

### INST Mode (Melodic)
```
┌─1─┬─2─┬─3─┬─A─┐
│−10│ −1│ +1│+10│  Scroll instrument
├─4─┼─5─┼─6─┼─B─┤
│PNO│BAS│SYN│STR│  Jump: Piano/Bass/Lead/Strings
├─7─┼─8─┼─9─┼─C─┤
│BRS│PAD│CHR│SFX│  Jump: Brass/Pads/Choir/SFX
├─*─┼─0─┼─#─┼─D─┤
│ — │ — │ — │MOD│
└───┴───┴───┴───┘
```

### INST Mode (Drums)
```
┌─1─┬─2─┬─3─┬─A─┐
│ −5│ −1│ +1│ +5│  Scroll drum note
├─4─┼─5─┼─6─┼─B─┤
│KCK│SNR│HHC│FTM│  Jump: Kick/Snare/ClosedHH/FloorTom
├─7─┼─8─┼─9─┼─C─┤
│CRS│CBL│CON│PRC│  Jump: Crash/Cowbell/OpenConga/Claves
├─*─┼─0─┼─#─┼─D─┤
│ — │ — │ — │MOD│
└───┴───┴───┴───┘
```

### SONG Mode
```
┌─1─┬─2─┬─3─┬─A─┐
│ P0│ P1│ P2│ P3│  Append+load slots 0-3
├─4─┼─5─┼─6─┼─B─┤
│ P4│ P5│ P6│ P7│  Append+load slots 4-7
├─7─┼─8─┼─9─┼─C─┤
│ P8│ P9│P10│P11│  Append+load slots 8-11
├─*─┼─0─┼─#─┼─D─┤
│SAV│TOG│DEL│MOD│  Save · Toggle SongMode · Del last · Mode
└───┴───┴───┴───┘
```

---

## 15. Troubleshooting

### No sound at startup

1. Check VS1053B wiring — XSMT must be tied to 3.3 V (not left floating)
2. Check volume: in CTRL mode, press `0` several times to raise volume to 100%
3. Open Serial Monitor at 115200 baud. Look for `[vs] VS1053 RTMIDI ready`. If missing, there is a wiring problem with VS_CS/VS_DCS/VS_DREQ or the 5 V supply.
4. Verify the VS1053B has the RTMIDI plugin loaded — `switchToMidi()` does this automatically. If Serial shows no VS message at all, SPI is not reaching the chip.

### Sound plays but very distorted or clipping

- Volume is set too high on the VS1053B side. In CTRL mode, press `*` to reduce volume.
- The PCM output from VS1053B may be overdriving your amplifier. Reduce the VS1053B volume first before increasing amplifier gain.

### Notes get stuck (drone that never stops)

- Press `5` in CTRL mode (**Stop + Panic**). This sends MIDI CC120 (All Sound Off) and CC123 (All Notes Off) to all 16 channels, which forces the VS1053B to silence everything immediately.

### SD card not detected

1. Ensure the card is **FAT32** formatted (not exFAT or NTFS)
2. Check the SD_CS wiring (GPIO 16) and SPI connections
3. Serial monitor shows `[stor] SD not found` if mount fails — sequencer still works, just no persistence
4. Try a different SD card; some ultra-high-speed cards have compatibility issues

### Patterns lost after power-off

- You must manually save each pattern. Press `B` in CTRL mode or `*` in SONG mode while the pattern you want to keep is the active slot.
- Check that SD is mounted (Serial shows `[stor] SD mounted` at boot)
- Check there is write space on the card

### Display shows garbled pixels or misaligned content

- This is the SH1106 2-column internal offset affecting rounded shapes. The firmware uses only rectangles and lines to avoid this. If you see circle artifacts, a third-party library call is drawing them — file an issue.
- If the whole display is blank, check I2C wiring (SDA=21, SCL=22) and that VCC is 3.3 V (not 5 V — the SH1106 is a 3.3 V device).

### Keypad keys not responding or wrong keys triggering

- Check row/column wiring. Row R1=GPIO13, R2=14, R3=25, R4=4. Col C1=26, C2=27, C3=32, C4=33.
- GPIO 34 is assigned to VS_DREQ — it is input-only on ESP32. Ensure DREQ is wired to pin 34 and no keypad line overlaps.
- Debounce is set to 50 ms. If multiple steps toggle at once, increase `setDebounceTime()` in `keys.h`.

### BPM drifts or timing sounds irregular

- ZOMBI SS uses the ESP32's hardware timer (1 MHz base clock, 80 MHz oscillator divided by 80). Timing should be very stable.
- Do not run other high-frequency interrupts from your own code that could starve the timer ISR.
- The ISR only sets a 1-byte flag; actual MIDI messages are sent from the main loop, so the timer is not blocked by SPI.

### Serial output floods during playback

- AudioLogger warning messages from the VS1053 library can be suppressed. The firmware is configured for Warning level only. If you see flooding, check if another Serial.print is in a tight loop.

---

*ZOMBI SS is open-source firmware for the ESP32. Modify, fork, and build upon it freely.*
