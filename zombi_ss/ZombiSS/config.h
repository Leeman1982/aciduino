#pragma once

// ── SPI bus (shared: VS1053 + SD card) ──────────────────────────────────────
#define SPI_SCK     18
#define SPI_MISO    19
#define SPI_MOSI    23

// ── VS1053B ──────────────────────────────────────────────────────────────────
#define VS_CS       5
#define VS_DCS      17
#define VS_DREQ     34

// ── SD card ──────────────────────────────────────────────────────────────────
#define SD_CS_PIN   16

// ── SH1106 OLED (I2C) ────────────────────────────────────────────────────────
#define I2C_SDA     21
#define I2C_SCL     22

// ── 4×4 keypad ───────────────────────────────────────────────────────────────
#define KEY_R1      13
#define KEY_R2      14
#define KEY_R3      25
#define KEY_R4      4
#define KEY_C1      26
#define KEY_C2      27
#define KEY_C3      32
#define KEY_C4      33

// ── Sequencer ─────────────────────────────────────────────────────────────────
#define SEQ_TRACKS      4
#define SEQ_STEPS       16
#define MAX_PATTERNS    16
#define SONG_CHAIN_LEN  64
#define SEQ_PPQN        24          // pulses per quarter note
#define TICKS_PER_STEP  6           // 16th-note steps (PPQN / 4)
#define BPM_MIN         40
#define BPM_MAX         240
#define BPM_DEFAULT     120

// ── MIDI ──────────────────────────────────────────────────────────────────────
#define DRUM_CHANNEL    9           // 0-indexed (= MIDI channel 10)
#define DRUM_NOTE_MIN   35
#define DRUM_NOTE_MAX   81

// ── Audio defaults ────────────────────────────────────────────────────────────
#define VOLUME_DEFAULT  80          // 0 = mute, 100 = max (user scale)
#define REVERB_DEFAULT  64          // 0-127 GM CC91

// ── Boot ──────────────────────────────────────────────────────────────────────
#define BOOT_SPLASH_MS  2500

// ── Modes (short-press D advances in non-STEP; long-press D advances in STEP)
typedef enum { MODE_STEP=0, MODE_CTRL, MODE_INST, MODE_SONG, NUM_MODES } AppMode;
typedef enum { SEQ_STOPPED=0, SEQ_PLAYING } SeqState;

// ── Core data structures ──────────────────────────────────────────────────────
struct SeqStep {
    bool    active;
    uint8_t velocity;   // 0-127
};

struct SeqTrack {
    uint8_t channel;    // 0-15 (9 = GM percussion)
    uint8_t instrument; // 0-127 GM program (not used on drum channel)
    uint8_t note;       // 0-127; for drums: MIDI note 35-81
    SeqStep steps[SEQ_STEPS];
};

struct Pattern {
    SeqTrack tracks[SEQ_TRACKS];
    uint8_t  length;    // 1-16 active steps
    uint8_t  bpm;       // 40-240
};

struct SongChain {
    uint8_t entries[SONG_CHAIN_LEN]; // pattern slot indices
    uint8_t len;                     // number of active entries
    uint8_t playPos;                 // current position during playback
};

// ── Extern globals (defined in their respective .h files or ZombiSS.ino) ──────
extern bool      sdAvail;
extern AppMode   appMode;
extern SeqState  seqState;
extern uint8_t   currentTrack;   // 0-3
extern uint8_t   activeSlot;     // 0-15
extern uint8_t   volume;         // 0-100 (user scale)
extern uint8_t   reverbLevel;    // 0-127
extern bool      needsRedraw;
extern bool      songMode;       // true = play song chain

extern Pattern   patterns[MAX_PATTERNS];
extern SongChain songChain;
extern volatile uint8_t currentStep; // 0-15, updated in ISR path
