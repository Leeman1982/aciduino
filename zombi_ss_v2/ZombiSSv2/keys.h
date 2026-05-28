#pragma once
#include <Keypad.h>
#include "config.h"
// sequencer.h, storage.h, juno_synth.h must be included before this

// ── Keypad wiring ─────────────────────────────────────────────────────────
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
// KEY_C3 (GPIO35) and KEY_C4 (GPIO36) are input-only on ESP32.
// The Keypad library requests INPUT_PULLUP but these pins have no internal
// pullup. Add 10kΩ resistors from GPIO35/36 to 3.3V in hardware.

Keypad kp = Keypad(makeKeymap(_kmap), _rowPins, _colPins, KP_ROWS, KP_COLS);

static bool _dHeld = false;

// ── Mode advance ──────────────────────────────────────────────────────────
static void _advanceMode() {
    appMode=(AppMode)(((uint8_t)appMode+1)%(uint8_t)NUM_MODES);
    needsRedraw=true;
}

// ── STEP mode ─────────────────────────────────────────────────────────────
// '1'-'#' → toggle steps 0-14, 'D' short=step15, hold=mode
static const char _STEP_MAP[] = "123A456B789C*0#D";

static void _handleStep(char key, KeyState ks) {
    if (key=='D') {
        if (ks==HOLD)    { _dHeld=true; _advanceMode(); }
        else if (ks==RELEASED) { if (!_dHeld) toggleStep(currentTrack,15); _dHeld=false; }
        return;
    }
    // Hold '*' = clear track
    if (key=='*') {
        if (ks==HOLD)    clearTrack(currentTrack);
        else if (ks==PRESSED) toggleStep(currentTrack, 12);
        return;
    }
    if (ks!=PRESSED) return;
    for (uint8_t i=0;i<15;i++)
        if (_STEP_MAP[i]==key && i!=12) { toggleStep(currentTrack, i); return; }
}

// ── CTRL mode ─────────────────────────────────────────────────────────────
// Row 1: T1 T2 T3 T4   (track select 0-3)
// Row 2: T5 T6 PLY SAV (track 4-5, play, save)
// Row 3: BPM- BPM+ N- N+
// Row 4: V-  V+  REV  MODE
static const uint8_t _REV_STEPS[] = {0, 32, 64, 96, 127};
static uint8_t _revIdx = 2;

static void _handleCtrl(char key, KeyState ks) {
    if (ks!=PRESSED) return;
    Pattern& p = patterns[activeSlot];
    SeqTrack& t = p.tracks[currentTrack];
    switch (key) {
        case '1': currentTrack=0; needsRedraw=true; break;
        case '2': currentTrack=1; needsRedraw=true; break;
        case '3': currentTrack=2; needsRedraw=true; break;
        case 'A': currentTrack=3; needsRedraw=true; break;
        case '4': currentTrack=4; needsRedraw=true; break;
        case '5': currentTrack=5; needsRedraw=true; break;
        case '6':
            if (seqState==SEQ_PLAYING) stopSeq(); else startSeq();
            break;
        case 'B': savePattern(activeSlot); saveSongChain(); break;
        case '7': setSeqBpm(p.bpm>BPM_MIN+4 ? p.bpm-5 : BPM_MIN); break;
        case '8': setSeqBpm(p.bpm<BPM_MAX-4 ? p.bpm+5 : BPM_MAX); break;
        case '9': adjustNote(currentTrack, -1); break;
        case 'C': adjustNote(currentTrack,  1); break;
        case '*': setMasterVolume(volume>9  ? volume-10 : 0); break;
        case '0': setMasterVolume(volume<91 ? volume+10 : 100); break;
        case '#':
            _revIdx=(_revIdx+1)%5;
            reverbLevel=_REV_STEPS[_revIdx];
            applyReverb(reverbLevel);
            needsRedraw=true;
            break;
        case 'D': _advanceMode(); break;
        default: break;
    }
    (void)t;
}

// ── INST mode — GM ────────────────────────────────────────────────────────
static void _handleInstGM(char key, SeqTrack& t) {
    switch (key) {
        case '1': setInstrument(currentTrack, t.instrument>9  ? t.instrument-10 : 0); break;
        case '2': setInstrument(currentTrack, t.instrument>0  ? t.instrument-1  : 0); break;
        case '3': setInstrument(currentTrack, t.instrument<127? t.instrument+1  : 127); break;
        case 'A': setInstrument(currentTrack, t.instrument<118? t.instrument+10 : 127); break;
        case '4': setInstrument(currentTrack,   0); break; // Piano
        case '5': setInstrument(currentTrack,  32); break; // Bass
        case '6': setInstrument(currentTrack,  80); break; // Lead
        case 'B': setInstrument(currentTrack,  48); break; // Strings
        case '7': setInstrument(currentTrack,  56); break; // Brass
        case '8': setInstrument(currentTrack,  88); break; // Pads
        case '9': setInstrument(currentTrack,  52); break; // Choir
        case 'C': setInstrument(currentTrack, 120); break; // SFX
        default: break;
    }
}

// ── INST mode — DRUM ──────────────────────────────────────────────────────
static void _handleInstDrum(char key, SeqTrack& t) {
    switch (key) {
        case '1': setInstrument(currentTrack, t.note>DRUM_NOTE_MIN+4?t.note-5:DRUM_NOTE_MIN); break;
        case '2': setInstrument(currentTrack, t.note>DRUM_NOTE_MIN  ?t.note-1:DRUM_NOTE_MIN); break;
        case '3': setInstrument(currentTrack, t.note<DRUM_NOTE_MAX  ?t.note+1:DRUM_NOTE_MAX); break;
        case 'A': setInstrument(currentTrack, t.note<DRUM_NOTE_MAX-4?t.note+5:DRUM_NOTE_MAX); break;
        case '4': setInstrument(currentTrack, 36); break; // Kick
        case '5': setInstrument(currentTrack, 38); break; // Snare
        case '6': setInstrument(currentTrack, 42); break; // Closed HH
        case 'B': setInstrument(currentTrack, 45); break; // Floor Tom
        case '7': setInstrument(currentTrack, 49); break; // Crash
        case '8': setInstrument(currentTrack, 56); break; // Cowbell
        case '9': setInstrument(currentTrack, 63); break; // Open Conga
        case 'C': setInstrument(currentTrack, 75); break; // Claves
        default: break;
    }
}

// ── INST mode — JUNO patch controls ──────────────────────────────────────
// Row 1: FC-100 FC-10 FC+10 FC+100
// Row 2: SAW    SQR   TRI   SIN
// Row 3: Res-   Res+  Atk-  Atk+
// D: mode
static void _handleInstJuno(char key) {
    const float FC_STEP_LG = 100.0f, FC_STEP_SM = 10.0f;
    const float RES_STEP   = 0.05f;
    const uint16_t ATK_STEP = 10;

    switch (key) {
        case '1':
            junoFilt.cutoff = constrain(junoFilt.cutoff - FC_STEP_LG, J_MIN_CUTOFF, J_MAX_CUTOFF);
            needsRedraw=true; break;
        case '2':
            junoFilt.cutoff = constrain(junoFilt.cutoff - FC_STEP_SM, J_MIN_CUTOFF, J_MAX_CUTOFF);
            needsRedraw=true; break;
        case '3':
            junoFilt.cutoff = constrain(junoFilt.cutoff + FC_STEP_SM, J_MIN_CUTOFF, J_MAX_CUTOFF);
            needsRedraw=true; break;
        case 'A':
            junoFilt.cutoff = constrain(junoFilt.cutoff + FC_STEP_LG, J_MIN_CUTOFF, J_MAX_CUTOFF);
            needsRedraw=true; break;
        case '4': junoSetWaveform(0); needsRedraw=true; break; // Sawtooth
        case '5': junoSetWaveform(1); needsRedraw=true; break; // Square
        case '6': junoSetWaveform(2); needsRedraw=true; break; // Triangle
        case 'B': junoSetWaveform(3); needsRedraw=true; break; // Sine
        case '7':
            junoFilt.reso = constrain(junoFilt.reso - RES_STEP, J_MIN_RESO, J_MAX_RESO);
            needsRedraw=true; break;
        case '8':
            junoFilt.reso = constrain(junoFilt.reso + RES_STEP, J_MIN_RESO, J_MAX_RESO);
            needsRedraw=true; break;
        case '9':
            junoEnv.ampAttack = constrain(junoEnv.ampAttack > ATK_STEP ?
                (int)junoEnv.ampAttack - ATK_STEP : 1, 1, 2000);
            junoApplyEnvTimes(); needsRedraw=true; break;
        case 'C':
            junoEnv.ampAttack = constrain((int)junoEnv.ampAttack + ATK_STEP, 1, 2000);
            junoApplyEnvTimes(); needsRedraw=true; break;
        default: break;
    }
}

static void _handleInst(char key, KeyState ks) {
    if (ks!=PRESSED) return;
    if (key=='D') { _advanceMode(); return; }
    SeqTrack& t = patterns[activeSlot].tracks[currentTrack];
    if      (t.type==TRACK_GM)   _handleInstGM(key, t);
    else if (t.type==TRACK_DRUM) _handleInstDrum(key, t);
    else                          _handleInstJuno(key);
}

// ── SONG mode ─────────────────────────────────────────────────────────────
static const char _SONG_SLOT_KEYS[] = "123A456B789C";

static void _handleSong(char key, KeyState ks) {
    if (ks!=PRESSED) return;
    for (uint8_t i=0;i<12;i++) {
        if (_SONG_SLOT_KEYS[i]==key) { chainAppend(i); loadSlot(i); return; }
    }
    switch (key) {
        case '*': savePattern(activeSlot); saveSongChain(); break;
        case '0': songMode=!songMode; needsRedraw=true; break;
        case '#': chainRemoveLast(); break;
        case 'D': _advanceMode(); break;
        default: break;
    }
}

// ── Public API ────────────────────────────────────────────────────────────
void initKeys() {
    kp.setDebounceTime(50);
    kp.setHoldTime(400);
}

void processKeys() {
    if (!kp.getKeys()) return;
    for (uint8_t i=0;i<LIST_MAX;i++) {
        if (kp.key[i].kstate==IDLE && !kp.key[i].stateChanged) continue;
        char     key=kp.key[i].kchar;
        KeyState ks =kp.key[i].kstate;
        switch (appMode) {
            case MODE_STEP: _handleStep(key, ks); break;
            case MODE_CTRL: _handleCtrl(key, ks); break;
            case MODE_INST: _handleInst(key, ks); break;
            case MODE_SONG: _handleSong(key, ks); break;
            default: break;
        }
    }
}
