/*!
 *  @file   ZombiSS.ino
 *  @brief  ZOMBI SS — 4-track GM MIDI step sequencer with VS1053B synth
 *
 *  Hardware
 *  ────────
 *  MCU     : ESP32 WROOM-32
 *  Synth   : VS1053B (GM MIDI + hardware DSP reverb via CC91)
 *  Display : SH1106 1.3" 128×64 OLED (I2C)
 *  Input   : 4×4 keypad (combo module with OLED)
 *  Storage : microSD (FAT32) for pattern + song chain persistence
 *
 *  Wiring
 *  ────────────────────────────────────────────────────────────────────────────
 *  VS1053  SCK  → GPIO 18   VS1053  MISO → GPIO 19   VS1053  MOSI → GPIO 23
 *  VS1053  XCS  → GPIO  5   VS1053  XDCS → GPIO 17   VS1053  DREQ → GPIO 34
 *  VS1053  !RST → 10 kΩ pullup to 3.3 V (no GPIO)
 *  VS1053  XSMT → 3.3 V (unmute)
 *  SD card CS   → GPIO 16
 *  OLED    SDA  → GPIO 21   OLED SCL → GPIO 22
 *  Keypad rows  R1–R4 → GPIO 13, 14, 25, 4
 *  Keypad cols  C1–C4 → GPIO 26, 27, 32, 33
 *
 *  Libraries (install via Arduino Library Manager)
 *  ────────────────────────────────────────────────
 *  ESP_VS1053_Library  by baldram
 *  U8g2                by Oliver Kraus
 *  Keypad              by Mark Stanley & Alexander Brevig
 *  SD                  bundled with ESP32 Arduino core
 *
 *  Arduino IDE settings
 *  ────────────────────────────────────────────────
 *  Board            : ESP32 Dev Module
 *  Partition Scheme : Default 4MB with spiffs (or any ≥200 KB SRAM free)
 *  Flash Frequency  : 80 MHz   Upload Speed : 921600
 *
 *  Keypad layout (all 4 modes share the same physical matrix)
 *  ──────────────────────────────────────────────────────────
 *  STEP  : [1-9,A-C,*,0,#]=toggle steps  [D]=hold→mode / release→step16
 *  CTRL  : [1-3,A]=track  [4-6,B]=ply/stp/clr/sav  [7-9,C]=bpm/note  [*,0,#,D]
 *  INST  : [1-3,A]=±10/±1 browsing  [4-9,B-C]=bank jumps  [D]=mode
 *  SONG  : [1-9,A-C]=append+load slots 0-11  [*,0,#,D]=sav/mode/del/mode
 */

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// Include order matters: display calls Wire.begin(); sequencer/storage need
// vs_midi types; keys.h depends on all sequencer helpers.
#include "display.h"
#include "vs_midi.h"
#include "sequencer.h"
#include "storage.h"
#include "keys.h"

// ── Global state (declared extern in config.h) ────────────────────────────────
AppMode  appMode     = MODE_STEP;
SeqState seqState    = SEQ_STOPPED;
uint8_t  currentTrack = 0;
bool     needsRedraw  = true;

// ── Boot timing ───────────────────────────────────────────────────────────────
static uint32_t _bootStart   = 0;
static bool     _booting     = true;
static uint32_t _lastDispMs  = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println(F("\n=== ZOMBI SS BOOT ==="));

    // 1. I2C + OLED — splash shows immediately while slower inits proceed
    initDisplay();
    drawBootScreen();
    _bootStart = millis();

    // 2. SPI bus (shared: VS1053 + SD)
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    // 3. VS1053 MIDI engine
    initVsMidi();

    // 4. Step sequencer (timer init — alarm disabled until startSeq)
    initSequencer();

    // 5. SD card + load saved patterns and song chain
    initStorage();

    // 6. Keypad
    initKeys();

    Serial.println(F("[main] Boot init done"));
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    // ── Boot splash hold ─────────────────────────────────────────────────────
    if (_booting) {
        if (millis() - _bootStart < BOOT_SPLASH_MS) return;
        _booting    = false;
        needsRedraw = true;
    }

    // ── Sequencer step pump (checks volatile _stepPend flag) ─────────────────
    runSequencer();

    // ── Keypad scan ───────────────────────────────────────────────────────────
    processKeys();

    // ── Periodic redraw while playing (playhead animation) ───────────────────
    uint32_t now = millis();
    if (seqState == SEQ_PLAYING && (now - _lastDispMs >= 80)) {
        _lastDispMs = now;
        needsRedraw = true;
    }

    // ── OLED update ───────────────────────────────────────────────────────────
    updateDisplay();
}
