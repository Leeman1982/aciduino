#pragma once
#include <Keypad.h>
#include "config.h"
// sequencer.h, storage.h, vs_midi.h must be included before this

// ── Keypad wiring ─────────────────────────────────────────────────────────────
static const byte KP_ROWS = 4;
static const byte KP_COLS = 4;

static char _kmap[KP_ROWS][KP_COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
static byte _rowPins[KP_ROWS] = { KEY_R1, KEY_R2, KEY_R3, KEY_R4 };
static byte _colPins[KP_COLS] = { KEY_C1, KEY_C2, KEY_C3, KEY_C4 };

Keypad kp = Keypad(makeKeymap(_kmap), _rowPins, _colPins, KP_ROWS, KP_COLS);

// D-key hold flag (STEP mode: short-release=step15, hold=advance mode)
static bool _dHeld = false;

// ── Mode advance ──────────────────────────────────────────────────────────────
static void _advanceMode() {
    appMode = (AppMode)(((uint8_t)appMode + 1) % (uint8_t)NUM_MODES);
    needsRedraw = true;
}

// ── STEP mode ─────────────────────────────────────────────────────────────────
// Keys map to steps 0-15 in reading order; D-key: hold→mode, release→step 15
static const char _STEP_MAP[] = "123A456B789C*0#D";

static void _handleStep(char key, KeyState ks) {
    if (key == 'D') {
        if (ks == HOLD) {
            _dHeld = true;
            _advanceMode();
        } else if (ks == RELEASED) {
            if (!_dHeld) toggleStep(currentTrack, 15);
            _dHeld = false;
        }
        return;
    }
    if (ks != PRESSED) return;
    for (uint8_t i = 0; i < 15; i++) {
        if (_STEP_MAP[i] == key) { toggleStep(currentTrack, i); return; }
    }
}

// ── CTRL mode ─────────────────────────────────────────────────────────────────
// '1'/'2'/'3'/'A' → select track 0/1/2/3
// '4' → Play/Stop toggle   '5' → Stop+Panic   '6' → Clear track   'B' → Save
// '7'/'8' → BPM -5/+5      '9'/'C' → Note -1/+1
// '*'/'0' → Vol -10/+10    '#' → Reverb cycle   'D' → mode advance
static const uint8_t _REV_STEPS[] = {0, 32, 64, 96, 127};
static uint8_t _revIdx = 2; // index into _REV_STEPS matching REVERB_DEFAULT=64

static void _handleCtrl(char key, KeyState ks) {
    if (ks != PRESSED) return;
    Pattern& p = patterns[activeSlot];
    switch (key) {
        case '1': currentTrack = 0; needsRedraw = true; break;
        case '2': currentTrack = 1; needsRedraw = true; break;
        case '3': currentTrack = 2; needsRedraw = true; break;
        case 'A': currentTrack = 3; needsRedraw = true; break;

        case '4':
            if (seqState == SEQ_PLAYING) stopSeq(); else startSeq();
            break;
        case '5': stopSeq(); seqPanic(); break;
        case '6': clearTrack(currentTrack); break;
        case 'B': savePattern(activeSlot); saveSongChain(); break;

        case '7': setSeqBpm(p.bpm > BPM_MIN + 4 ? p.bpm - 5 : BPM_MIN); break;
        case '8': setSeqBpm(p.bpm < BPM_MAX - 4 ? p.bpm + 5 : BPM_MAX); break;
        case '9': adjustNote(currentTrack, -1); break;
        case 'C': adjustNote(currentTrack,  1); break;

        case '*': setMasterVolume(volume > 9  ? volume - 10 : 0); break;
        case '0': setMasterVolume(volume < 91 ? volume + 10 : 100); break;
        case '#':
            _revIdx = (_revIdx + 1) % 5;
            reverbLevel = _REV_STEPS[_revIdx];
            applyReverb(reverbLevel);
            needsRedraw = true;
            break;
        case 'D': _advanceMode(); break;
        default: break;
    }
}

// ── INST mode ─────────────────────────────────────────────────────────────────
// Melodic:
//   '1'=GM−10  '2'=GM−1  '3'=GM+1  'A'=GM+10
//   '4'=Piano(0)  '5'=Bass(32)  '6'=Lead(80)   'B'=Strings(48)
//   '7'=Brass(56) '8'=Pads(88)  '9'=Choir(52)  'C'=SFX(120)
//   'D'=mode
// Drum (ch9):
//   '1'=−5  '2'=−1  '3'=+1  'A'=+5
//   '4'=Kick(36)  '5'=Snare(38)  '6'=ClosedHH(42)  'B'=FloorTom(45)
//   '7'=Crash(49) '8'=Cowbell(56)'9'=OpenConga(63)  'C'=Claves(75)
//   'D'=mode

static void _handleInst(char key, KeyState ks) {
    if (ks != PRESSED) return;
    SeqTrack& t = patterns[activeSlot].tracks[currentTrack];
    bool isDrum = (t.channel == DRUM_CHANNEL);

    if (isDrum) {
        switch (key) {
            case '1': setInstrument(currentTrack, t.note > DRUM_NOTE_MIN+4 ? t.note-5 : DRUM_NOTE_MIN); break;
            case '2': setInstrument(currentTrack, t.note > DRUM_NOTE_MIN   ? t.note-1 : DRUM_NOTE_MIN); break;
            case '3': setInstrument(currentTrack, t.note < DRUM_NOTE_MAX   ? t.note+1 : DRUM_NOTE_MAX); break;
            case 'A': setInstrument(currentTrack, t.note < DRUM_NOTE_MAX-4 ? t.note+5 : DRUM_NOTE_MAX); break;
            case '4': setInstrument(currentTrack, 36); break;
            case '5': setInstrument(currentTrack, 38); break;
            case '6': setInstrument(currentTrack, 42); break;
            case 'B': setInstrument(currentTrack, 45); break;
            case '7': setInstrument(currentTrack, 49); break;
            case '8': setInstrument(currentTrack, 56); break;
            case '9': setInstrument(currentTrack, 63); break;
            case 'C': setInstrument(currentTrack, 75); break;
            case 'D': _advanceMode(); break;
            default:  break;
        }
    } else {
        switch (key) {
            case '1': setInstrument(currentTrack, t.instrument > 9   ? t.instrument-10 : 0);   break;
            case '2': setInstrument(currentTrack, t.instrument > 0   ? t.instrument-1  : 0);   break;
            case '3': setInstrument(currentTrack, t.instrument < 127 ? t.instrument+1  : 127); break;
            case 'A': setInstrument(currentTrack, t.instrument < 118 ? t.instrument+10 : 127); break;
            case '4': setInstrument(currentTrack,   0); break; // Piano
            case '5': setInstrument(currentTrack,  32); break; // Bass
            case '6': setInstrument(currentTrack,  80); break; // Lead Synths
            case 'B': setInstrument(currentTrack,  48); break; // Strings
            case '7': setInstrument(currentTrack,  56); break; // Brass
            case '8': setInstrument(currentTrack,  88); break; // Pads
            case '9': setInstrument(currentTrack,  52); break; // Choir
            case 'C': setInstrument(currentTrack, 120); break; // SFX
            case 'D': _advanceMode(); break;
            default:  break;
        }
    }
}

// ── SONG mode ─────────────────────────────────────────────────────────────────
// Keys '1'..'C' (12 keys) → append slot i to chain and navigate to it
// '*' → save all   '0' → toggle song mode   '#' → delete last chain entry
// 'D' → mode advance
static const char _SONG_SLOT_KEYS[] = "123A456B789C";

static void _handleSong(char key, KeyState ks) {
    if (ks != PRESSED) return;
    for (uint8_t i = 0; i < 12; i++) {
        if (_SONG_SLOT_KEYS[i] == key) {
            chainAppend(i);
            loadSlot(i);
            return;
        }
    }
    switch (key) {
        case '*': savePattern(activeSlot); saveSongChain(); break;
        case '0': songMode = !songMode; needsRedraw = true; break;
        case '#': chainRemoveLast(); break;
        case 'D': _advanceMode(); break;
        default: break;
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
void initKeys() {
    kp.setDebounceTime(50);
    kp.setHoldTime(400);
}

void processKeys() {
    if (!kp.getKeys()) return;
    for (uint8_t i = 0; i < LIST_MAX; i++) {
        if (kp.key[i].kstate == IDLE && !kp.key[i].stateChanged) continue;
        char     key = kp.key[i].kchar;
        KeyState ks  = kp.key[i].kstate;
        switch (appMode) {
            case MODE_STEP: _handleStep(key, ks); break;
            case MODE_CTRL: _handleCtrl(key, ks); break;
            case MODE_INST: _handleInst(key, ks); break;
            case MODE_SONG: _handleSong(key, ks); break;
            default: break;
        }
    }
}
