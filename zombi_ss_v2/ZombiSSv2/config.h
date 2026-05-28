#pragma once

// ── SPI bus (shared: VS1053 + SD) ──────────────────────────────────────────
#define SPI_SCK     18
#define SPI_MISO    19
#define SPI_MOSI    23

// ── VS1053B ──────────────────────────────────────────────────────────────────
#define VS_CS       5
#define VS_DCS      17
#define VS_DREQ     34   // input-only GPIO, fine

// ── SD card ──────────────────────────────────────────────────────────────────
#define SD_CS_PIN   16

// ── SH1106 OLED (I2C) ────────────────────────────────────────────────────────
#define I2C_SDA     21
#define I2C_SCL     22

// ── I2S output for Juno synth (PCM5102 DAC) ──────────────────────────────────
// BCK/WS freed from v1 keypad; DATA moved from I2C-22 to GPIO-4
#define I2S_BCK_PIN   26   // freed from v1 keypad C1
#define I2S_WS_PIN    25   // freed from v1 keypad R3
#define I2S_DATA_PIN   4   // freed from v1 keypad R4

// ── 4×4 keypad — reassigned to avoid I2S conflicts ───────────────────────────
// Rows: any output-capable GPIO
#define KEY_R1   13
#define KEY_R2   14
#define KEY_R3   15   // was 25; GPIO15 safe as output (strapping only affects JTAG TDO)
#define KEY_R4   27   // was 4
// Cols: INPUT_PULLUP capable GPIOs
#define KEY_C1   32   // was 26
#define KEY_C2   33   // was 27
// NOTE: GPIO35/36 are input-only — no internal pullup. Add 10kΩ to 3.3V on each.
#define KEY_C3   35
#define KEY_C4   36

// ── Sequencer ────────────────────────────────────────────────────────────────
#define SEQ_TRACKS      6
#define SEQ_STEPS      16
#define MAX_PATTERNS   16
#define SONG_CHAIN_LEN 64
#define SEQ_PPQN       24
#define TICKS_PER_STEP  6
#define BPM_MIN        40
#define BPM_MAX       240
#define BPM_DEFAULT   120

// ── MIDI ──────────────────────────────────────────────────────────────────────
#define DRUM_CHANNEL    9
#define DRUM_NOTE_MIN  35
#define DRUM_NOTE_MAX  81

// ── Audio ─────────────────────────────────────────────────────────────────────
#define VOLUME_DEFAULT   80   // 0=mute, 100=max (GM user scale)
#define REVERB_DEFAULT   64   // 0-127 GM CC91

// ── Juno synth defaults ──────────────────────────────────────────────────────
#define JUNO_SAMPLE_RATE  44100.0f
#define JUNO_BUF_SIZE     64
#define JUNO_VOICES        6

// ── Boot ──────────────────────────────────────────────────────────────────────
#define BOOT_SPLASH_MS  2500

// ── Track types ──────────────────────────────────────────────────────────────
typedef enum { TRACK_GM=0, TRACK_DRUM, TRACK_JUNO } TrackType;

// ── Mode enum ────────────────────────────────────────────────────────────────
typedef enum { MODE_STEP=0, MODE_CTRL, MODE_INST, MODE_SONG, NUM_MODES } AppMode;
typedef enum { SEQ_STOPPED=0, SEQ_PLAYING } SeqState;

// ── Data structures ──────────────────────────────────────────────────────────
struct SeqStep {
    bool    active;
    uint8_t velocity;
};

struct SeqTrack {
    TrackType type;
    uint8_t   channel;     // MIDI channel (GM/DRUM only)
    uint8_t   instrument;  // GM program 0-127 (GM tracks only)
    uint8_t   note;        // MIDI note 0-127
    SeqStep   steps[SEQ_STEPS];
};

struct Pattern {
    SeqTrack tracks[SEQ_TRACKS];
    uint8_t  length;  // 1-16 active steps
    uint8_t  bpm;
};

struct SongChain {
    uint8_t entries[SONG_CHAIN_LEN];
    uint8_t len;
    uint8_t playPos;
};

// ── Extern globals ─────────────────────────────────────────────────────────
extern bool      sdAvail;
extern AppMode   appMode;
extern SeqState  seqState;
extern uint8_t   currentTrack;
extern uint8_t   activeSlot;
extern uint8_t   volume;
extern uint8_t   reverbLevel;
extern bool      needsRedraw;
extern bool      songMode;

extern Pattern   patterns[MAX_PATTERNS];
extern SongChain songChain;
extern volatile uint8_t currentStep;
