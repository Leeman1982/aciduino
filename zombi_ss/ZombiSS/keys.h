#pragma once

#include <Keypad.h>
#include "config.h"

/*
 * Physical 4×4 layout (matches module header R1→top, R4→bottom):
 *
 *   [1][2][3][A]   Song 1–3        / NEXT track
 *   [4][5][6][B]   Song 4–6        / PREV track
 *   [7][8][9][C]   Song 7–9        / PLAY / PAUSE toggle
 *   [*][0][#][D]   VOL−  STOP  VOL+  REVERB cycle
 *
 * Holding [*]+[#] simultaneously is not supported (matrix limitation).
 */

static const byte KP_ROWS = 4;
static const byte KP_COLS = 4;

static char keyMap[KP_ROWS][KP_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static byte rowPins[KP_ROWS] = {KEY_R1, KEY_R2, KEY_R3, KEY_R4};
static byte colPins[KP_COLS] = {KEY_C1, KEY_C2, KEY_C3, KEY_C4};

static Keypad kp = Keypad(makeKeymap(keyMap), rowPins, colPins, KP_ROWS, KP_COLS);

// Forward declarations (implemented in vs_midi.h, compiled in same unit)
void midiPlay();
void midiPause();
void midiStop();
void midiNext();
void midiPrev();
void midiSelectSong(int idx);
void midiSetVolume(uint8_t v);
void midiSetReverb(uint8_t level);

void initKeys() {
    kp.setDebounceTime(50);
}

void processKeys() {
    if (appState == BOOT) return;

    char key = kp.getKey();
    if (key == NO_KEY) return;

    switch (key) {
        // ── Direct song select (1–9) ─────────────────────────────────────
        case '1': midiSelectSong(0); break;
        case '2': midiSelectSong(1); break;
        case '3': midiSelectSong(2); break;
        case '4': midiSelectSong(3); break;
        case '5': midiSelectSong(4); break;
        case '6': midiSelectSong(5); break;
        case '7': midiSelectSong(6); break;
        case '8': midiSelectSong(7); break;
        case '9': midiSelectSong(8); break;

        // ── Transport ────────────────────────────────────────────────────
        case 'A': midiNext(); break;
        case 'B': midiPrev(); break;
        case 'C':
            if (appState == PLAYING) midiPause();
            else                     midiPlay();
            break;
        case '0': midiStop(); break;

        // ── Volume (step ±5) ─────────────────────────────────────────────
        case '#': midiSetVolume(volume + 5); break;
        case '*': midiSetVolume(volume > 5 ? volume - 5 : 0); break;

        // ── Reverb cycle: off → 50% → 100% → off ─────────────────────────
        case 'D':
            if      (reverbLevel == 0)   midiSetReverb(64);
            else if (reverbLevel <= 64)  midiSetReverb(127);
            else                         midiSetReverb(0);
            break;

        default: break;
    }
}
