/*!
 *  @file   ZombiSS.ino
 *  @brief  ZOMBI SS — General MIDI Player with hardware reverb
 *
 *  Hardware
 *  ────────
 *  MCU     : ESP32 WROOM-32
 *  Synth   : VS1053B shield (GM MIDI + hardware DSP reverb)
 *  Display : SH1106 1.3" 128×64 OLED (I2C)
 *  Input   : 4×4 keypad matrix (same combo module as OLED)
 *  Audio   : VS1053 onboard stereo headphone / speaker output
 *  Storage : microSD (FAT32, .MID files in root)
 *
 *  Wiring  (VS1053 shield → ESP32, use jumper wires — NOT Arduino shield mode)
 *  ────────────────────────────────────────────────────────────────────────────
 *  VS1053  SCK  (pin 13 on UNO header) → ESP32 GPIO 18
 *  VS1053  MISO (pin 12)               → ESP32 GPIO 19
 *  VS1053  MOSI (pin 11)               → ESP32 GPIO 23
 *  VS1053  XCS  (pin  6 on shield)     → ESP32 GPIO  5
 *  VS1053  XDCS (pin  7 on shield)     → ESP32 GPIO 17
 *  VS1053  DREQ (pin  2 on shield)     → ESP32 GPIO 34
 *  VS1053  !RST                        → 10 kΩ pullup to 3.3 V (no GPIO)
 *  VS1053  XSMT                        → 3.3 V  (unmute)
 *  SD card CS   (pin  4 on shield)     → ESP32 GPIO 16
 *  VS1053  5V                          → ESP32 VIN (5 V)
 *  VS1053  GND                         → ESP32 GND
 *
 *  OLED combo module header: R4 R3 R2 R1 | C4 C3 C2 C1 | SDA SCL VCC GND
 *  SDA → GPIO 21   SCL → GPIO 22   VCC → 3.3 V
 *  Keypad rows R1–R4 → GPIO 13, 14, 25, 4
 *  Keypad cols C1–C4 → GPIO 26, 27, 32, 33
 *
 *  Libraries (install before compiling)
 *  ─────────────────────────────────────
 *  ESP_VS1053_Library  by baldram      https://github.com/baldram/ESP_VS1053_Library
 *  MD_MIDIFile         by majicDesigns Arduino Library Manager → search "MD_MIDIFile"
 *  U8g2                by Oliver Kraus Arduino Library Manager → search "U8g2"
 *  Keypad              by Mark Stanley Arduino Library Manager → search "Keypad"
 *  SD                  bundled with ESP32 Arduino core
 *
 *  Arduino IDE board settings
 *  ─────────────────────────────────────
 *  Board            : ESP32 Dev Module
 *  Partition Scheme : Default 4MB with spiffs (or any with free SRAM ≥ 200 KB)
 *  Flash Frequency  : 80 MHz
 *  Upload Speed     : 921600
 *
 *  SD card: FAT32 formatted, place .MID files in the root directory.
 *
 *  Keypad layout
 *  ─────────────────────────────────────
 *  [1][2][3][A]   → Song 1, 2, 3     | A = Next track
 *  [4][5][6][B]   → Song 4, 5, 6     | B = Prev track
 *  [7][8][9][C]   → Song 7, 8, 9     | C = Play / Pause
 *  [*][0][#][D]   → Vol−  Stop  Vol+ | D = Reverb cycle (off→50%→100%→off)
 */

#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "display.h"   // Wire.begin() + u8g2 — must come before vs_midi.h
#include "vs_midi.h"   // SPI.begin() + VS1053 + SD + MD_MIDIFile
#include "keys.h"      // Keypad (depends on forward decls from vs_midi.h)

// ── Global state definitions (declared extern in config.h) ────────────────────
AppState  appState    = BOOT;
int       songIndex   = 0;
int       songCount   = 0;
char      songNames[MAX_SONGS][13];
uint8_t   reverbLevel = REVERB_DEFAULT;
uint8_t   volume      = VOLUME_DEFAULT;
bool      needsRedraw = true;
uint32_t  playStartMs = 0;

static uint32_t _bootStart   = 0;
static uint32_t _lastDisplay = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ZOMBI SS BOOT ===");

    // 1. Display first — splash visible immediately during slow SD init
    initDisplay();
    drawBootScreen();
    _bootStart = millis();

    // 2. Keypad
    initKeys();

    // 3. VS1053 + SD + MD_MIDIFile (takes ~200 ms for VS1053 init)
    initVsMidi();
    // appState remains BOOT until splash timeout in loop()
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    // ── Boot splash hold ─────────────────────────────────────────────────────
    if (appState == BOOT) {
        if (millis() - _bootStart >= BOOT_SPLASH_MS) {
            appState  = STOPPED;
            needsRedraw = true;
        }
        return;  // don't process keys or audio during splash
    }

    // ── Audio pump ───────────────────────────────────────────────────────────
    runMidi();

    // ── Key scan ─────────────────────────────────────────────────────────────
    processKeys();

    // ── Periodic display refresh while playing (elapsed time update) ─────────
    uint32_t now = millis();
    if (appState == PLAYING && (now - _lastDisplay >= DISPLAY_RATE_MS)) {
        _lastDisplay = now;
        needsRedraw  = true;
    }

    // ── OLED update ──────────────────────────────────────────────────────────
    updateDisplay();
}
