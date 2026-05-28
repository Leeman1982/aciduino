/*!
 *  @file   ZombiSSv2.ino
 *  @brief  ZOMBI SS v2 — 6-track sequencer: GM MIDI + Juno polyphonic synth
 *
 *  Tracks T1-T4: General MIDI → VS1053B synthesizer (SPI)
 *  Tracks T5-T6: Juno software synth → PCM5102 DAC (I2S, Core 1 FreeRTOS)
 *
 *  Hardware
 *  ────────────────────────────────────────────────────────────────────────
 *  MCU         ESP32 WROOM-32
 *  GM Synth    VS1053B shield  (SPI: SCK=18 MISO=19 MOSI=23 CS=5 DCS=17 DREQ=34)
 *  SD card     CS=16  (shares SPI bus)
 *  Juno synth  PCM5102 DAC  (I2S: BCK=26 WS=25 DATA=4)
 *              PCM5102 wiring: SCK→GND (internal PLL), XSMT→3.3V (unmute)
 *  OLED        SH1106 128×64 I2C  (SDA=21 SCL=22)
 *  Keypad      4×4 matrix combo module with OLED
 *              Rows R1-R4 → GPIO 13, 14, 15, 27
 *              Cols C1-C4 → GPIO 32, 33, 35, 36
 *              !! GPIO35 (C3) and GPIO36 (C4) are input-only:
 *                 add 10kΩ pullup resistors from each to 3.3V !!
 *
 *  Libraries required
 *  ────────────────────────────────────────────────────────────────────────
 *  ESP_VS1053_Library  (baldram)
 *  U8g2                (Oliver Kraus)
 *  Keypad              (Mark Stanley)
 *  SD                  (bundled with ESP32 core)
 *
 *  Band-limited oscillators (optional upgrade)
 *  ────────────────────────────────────────────────────────────────────────
 *  Copy BL_Oscillator_ESP32.h from github.com/Leeman1982/ultimatesynthrp2350
 *  into this sketch folder, then add:
 *      #define BL_OSCILLATOR_AVAILABLE
 *  before the #include "juno_synth.h" line below.
 *
 *  Arduino IDE settings
 *  ────────────────────────────────────────────────────────────────────────
 *  Board            : ESP32 Dev Module
 *  Partition Scheme : Default 4MB with spiffs
 *  Flash Frequency  : 80 MHz   Upload Speed : 921600
 *  CPU Frequency    : 240 MHz
 *
 *  Keypad layout by mode
 *  ────────────────────────────────────────────────────────────────────────
 *  STEP  : 16 keys = 16 steps (toggle). Hold * = clear track.
 *          D short=step16, D hold=next mode.
 *  CTRL  : 1-A=T1-T4, 4-5=T5-T6, 6=Play/Stop, B=Save
 *          7/8=BPM±5, 9/C=Note±1, */0=Vol±10%, #=Reverb, D=mode
 *  INST  : GM:  ±10/±1 scroll + bank jumps
 *          DRUM: ±5/±1 scroll + family jumps
 *          JUNO: 1-A=Filter cutoff, 4-B=Waveform, 7-C=Res+Atk, D=mode
 *  SONG  : 1-C=append+load P0-P11, *=save, 0=toggle song mode,
 *          #=delete last chain entry, D=mode
 */

#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "display.h"     // Wire.begin + u8g2 — must come before juno_synth
// To use real BL oscillators, uncomment:
// #define BL_OSCILLATOR_AVAILABLE
#include "juno_synth.h"  // Juno engine + FreeRTOS audio task
#include "vs_midi.h"     // VS1053B GM MIDI
#include "sequencer.h"   // ESP32 hw_timer step engine
#include "storage.h"     // SD card save/load
#include "keys.h"        // 4×4 keypad

// ── Global state (extern'd in config.h) ──────────────────────────────────
AppMode  appMode      = MODE_STEP;
SeqState seqState     = SEQ_STOPPED;
uint8_t  currentTrack = 0;
bool     needsRedraw  = true;

// ── Boot timing ───────────────────────────────────────────────────────────
static uint32_t _bootStart  = 0;
static bool     _booting    = true;
static uint32_t _lastDispMs = 0;

// ─────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println(F("\n=== ZOMBI SS v2 BOOT ==="));

    // 1. I2C + OLED splash (display first so user sees boot immediately)
    initDisplay();
    drawBootScreen();
    _bootStart = millis();

    // 2. SPI bus (VS1053 + SD share this)
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    // 3. VS1053B GM MIDI engine
    initVsMidi();

    // 4. Juno synth — initialises I2S and spawns audio task on Core 1
    initJunoSynth();

    // 5. Step sequencer (timer init, alarm disabled until startSeq)
    initSequencer();

    // 6. SD card — loads saved patterns + song chain
    initStorage();

    // 7. Keypad
    initKeys();

    Serial.println(F("[main] Boot complete"));
}

// ─────────────────────────────────────────────────────────────────────────
void loop() {
    // Hold boot splash for BOOT_SPLASH_MS
    if (_booting) {
        if (millis() - _bootStart < BOOT_SPLASH_MS) return;
        _booting    = false;
        needsRedraw = true;
    }

    // Step sequencer pump (checks volatile _stepPend flag from timer ISR)
    runSequencer();

    // Key scan
    processKeys();

    // Juno parameter smoothing (interpolates cutoff, reso, vol toward targets)
    smoothJunoParams();

    // Periodic OLED refresh for playhead animation while playing
    uint32_t now = millis();
    if (seqState==SEQ_PLAYING && (now-_lastDispMs >= 80)) {
        _lastDispMs = now;
        needsRedraw = true;
    }

    // OLED update
    updateDisplay();
}
