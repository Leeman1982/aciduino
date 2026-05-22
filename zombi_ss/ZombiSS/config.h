#pragma once

// ── SPI bus (shared: VS1053 + SD card) ──────────────────────────────────────
#define SPI_SCK         18
#define SPI_MISO        19
#define SPI_MOSI        23

// ── VS1053B ──────────────────────────────────────────────────────────────────
// XCS  (command chip-select)
#define VS_CS           5
// XDCS (data chip-select / SDI)
#define VS_DCS          17
// DREQ (data-request, input)
#define VS_DREQ         34
// RESET: wire VS1053 !RESET through 10 kΩ to 3.3 V — no GPIO needed

// ── SD card (on VS1053 module) ───────────────────────────────────────────────
#define SD_CS_PIN       16

// ── SH1106 OLED — I2C ────────────────────────────────────────────────────────
#define I2C_SDA         21
#define I2C_SCL         22

// ── 4×4 keypad matrix ────────────────────────────────────────────────────────
// Rows (OUTPUT, driven LOW to scan)
#define KEY_R1          13   // top row   → '1' '2' '3' 'A'
#define KEY_R2          14
#define KEY_R3          25
#define KEY_R4          4    // bottom row → '*' '0' '#' 'D'
// Cols (INPUT_PULLUP)
#define KEY_C1          26
#define KEY_C2          27
#define KEY_C3          32
#define KEY_C4          33

// ── Audio defaults ───────────────────────────────────────────────────────────
// volume: 0 = muted, 100 = max (internally inverted for VS1053 register)
#define VOLUME_DEFAULT  80
// reverbLevel: 0–127, sent as GM CC91 to all 16 MIDI channels
#define REVERB_DEFAULT  64

// ── Limits ────────────────────────────────────────────────────────────────────
#define MAX_SONGS       64   // max .mid files indexed from SD root
#define BOOT_SPLASH_MS  2500
#define DISPLAY_RATE_MS 50   // periodic OLED refresh while playing (~20 fps)

// ── State machine ─────────────────────────────────────────────────────────────
typedef enum { BOOT, STOPPED, PLAYING, PAUSED } AppState;

// ── Shared globals (defined in ZombiSS.ino) ──────────────────────────────────
extern AppState  appState;
extern int       songIndex;
extern int       songCount;
extern char      songNames[MAX_SONGS][13];  // 8.3 name + null
extern uint8_t   reverbLevel;
extern uint8_t   volume;                    // 0–100 (user-facing scale)
extern bool      needsRedraw;
extern uint32_t  playStartMs;
