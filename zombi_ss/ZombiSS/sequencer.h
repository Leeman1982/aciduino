#pragma once
#include <Arduino.h>
#include "config.h"
#include "vs_midi.h"   // sendNoteOn/Off/CC/ProgramChange

// ── Pattern storage (defined here, extern'd in config.h) ─────────────────────
Pattern    patterns[MAX_PATTERNS];
SongChain  songChain;

volatile uint8_t currentStep    = 0;
uint8_t          activeSlot     = 0;
bool             songMode       = false;

static hw_timer_t*  _seqTimer   = nullptr;
static volatile bool _stepPend  = false;
static volatile uint8_t _tickDiv = 0;

// Per-track last-played note for correct note-off on the next step
static uint8_t _lastNote[SEQ_TRACKS];
static bool    _lastNoteOn[SEQ_TRACKS];

// ── ISR — no SPI here, only flag ─────────────────────────────────────────────
void IRAM_ATTR _onSeqTick() {
    if (++_tickDiv >= TICKS_PER_STEP) {
        _tickDiv  = 0;
        _stepPend = true;
    }
}

// ── Default pattern (all silent, sane defaults) ───────────────────────────────
static void _initPattern(Pattern& p) {
    p.bpm    = BPM_DEFAULT;
    p.length = SEQ_STEPS;
    // T0: Grand Piano  CH0  C5(60)
    p.tracks[0] = {0,  0,  60, {}};
    // T1: Finger Bass  CH1  C3(36)
    p.tracks[1] = {1,  33, 36, {}};
    // T2: Square Lead  CH2  C5(60)
    p.tracks[2] = {2,  80, 60, {}};
    // T3: Drums        CH9  Bass Drum 1 (36)
    p.tracks[3] = {DRUM_CHANNEL, 0, 36, {}};
    for (uint8_t t = 0; t < SEQ_TRACKS; t++)
        for (uint8_t s = 0; s < SEQ_STEPS; s++)
            p.tracks[t].steps[s] = {false, 100};
}

// ── Send program changes for all melodic tracks in a pattern ──────────────────
static void _applyPatternPC(const Pattern& p) {
    for (uint8_t t = 0; t < SEQ_TRACKS; t++)
        if (p.tracks[t].channel != DRUM_CHANNEL)
            sendProgramChange(p.tracks[t].channel, p.tracks[t].instrument);
}

// ── Silence all voices ────────────────────────────────────────────────────────
void seqPanic() {
    for (uint8_t ch = 0; ch < 16; ch++) {
        sendCC(ch, 120, 0);   // All Sound Off
        sendCC(ch, 123, 0);   // All Notes Off
    }
    for (uint8_t t = 0; t < SEQ_TRACKS; t++) _lastNoteOn[t] = false;
}

// ── Advance one step — called from loop() when _stepPend is set ──────────────
void processStepAdvance() {
    // Send note-off for whatever was playing
    for (uint8_t t = 0; t < SEQ_TRACKS; t++) {
        if (_lastNoteOn[t]) {
            sendNoteOff(patterns[activeSlot].tracks[t].channel, _lastNote[t]);
            _lastNoteOn[t] = false;
        }
    }

    // Advance step counter
    Pattern& p = patterns[activeSlot];
    currentStep = (currentStep + 1) % p.length;

    // Song chain: on wrap, load next chained pattern
    if (currentStep == 0 && songMode && songChain.len > 0) {
        songChain.playPos = (songChain.playPos + 1) % songChain.len;
        uint8_t next = songChain.entries[songChain.playPos];
        if (next != activeSlot) {
            activeSlot = next;
            _applyPatternPC(patterns[activeSlot]);
        }
    }

    // Note-on for the new step
    Pattern& cur = patterns[activeSlot];
    for (uint8_t t = 0; t < SEQ_TRACKS; t++) {
        const SeqStep& st = cur.tracks[t].steps[currentStep];
        if (st.active) {
            uint8_t note = cur.tracks[t].note;
            sendNoteOn(cur.tracks[t].channel, note, st.velocity);
            _lastNote[t]   = note;
            _lastNoteOn[t] = true;
        }
    }

    needsRedraw = true;
}

// ── BPM → timer period ───────────────────────────────────────────────────────
static uint32_t _bpmUs(uint8_t bpm) {
    return 60000000UL / ((uint32_t)bpm * SEQ_PPQN);
}

void setSeqBpm(uint8_t bpm) {
    bpm = constrain(bpm, BPM_MIN, BPM_MAX);
    patterns[activeSlot].bpm = bpm;
    if (_seqTimer) timerAlarmWrite(_seqTimer, _bpmUs(bpm), true);
    needsRedraw = true;
}

// ── Start / stop ──────────────────────────────────────────────────────────────
void startSeq() {
    if (seqState == SEQ_PLAYING) return;
    currentStep  = 0;
    _tickDiv     = 0;
    _stepPend    = false;
    if (songMode) { songChain.playPos = 0; activeSlot = (songChain.len > 0) ? songChain.entries[0] : activeSlot; }
    memset(_lastNoteOn, 0, sizeof(_lastNoteOn));
    _applyPatternPC(patterns[activeSlot]);
    timerAlarmWrite(_seqTimer, _bpmUs(patterns[activeSlot].bpm), true);
    timerAlarmEnable(_seqTimer);
    seqState    = SEQ_PLAYING;
    needsRedraw = true;
}

void stopSeq() {
    if (seqState == SEQ_STOPPED) return;
    timerAlarmDisable(_seqTimer);
    seqPanic();
    currentStep = 0;
    _stepPend   = false;
    seqState    = SEQ_STOPPED;
    needsRedraw = true;
}

// ── Load a pattern slot (stop/restart if playing) ────────────────────────────
void loadSlot(uint8_t slot) {
    slot = slot % MAX_PATTERNS;
    bool wasPlaying = (seqState == SEQ_PLAYING);
    if (wasPlaying) { timerAlarmDisable(_seqTimer); seqPanic(); }
    activeSlot = slot;
    if (wasPlaying) {
        currentStep = 0; _tickDiv = 0; _stepPend = false;
        memset(_lastNoteOn, 0, sizeof(_lastNoteOn));
        _applyPatternPC(patterns[activeSlot]);
        timerAlarmWrite(_seqTimer, _bpmUs(patterns[activeSlot].bpm), true);
        timerAlarmEnable(_seqTimer);
    }
    needsRedraw = true;
}

// ── Edit helpers ──────────────────────────────────────────────────────────────
void toggleStep(uint8_t track, uint8_t step) {
    if (track >= SEQ_TRACKS || step >= SEQ_STEPS) return;
    patterns[activeSlot].tracks[track].steps[step].active ^= true;
    needsRedraw = true;
}

void clearTrack(uint8_t track) {
    if (track >= SEQ_TRACKS) return;
    for (uint8_t s = 0; s < SEQ_STEPS; s++)
        patterns[activeSlot].tracks[track].steps[s].active = false;
    needsRedraw = true;
}

// For melodic tracks: change GM instrument + send program change immediately.
// For drum track: change drum note (clamped to 35-81).
void setInstrument(uint8_t track, uint8_t val) {
    if (track >= SEQ_TRACKS) return;
    SeqTrack& t = patterns[activeSlot].tracks[track];
    if (t.channel == DRUM_CHANNEL) {
        t.note = (uint8_t)constrain((int)val, DRUM_NOTE_MIN, DRUM_NOTE_MAX);
    } else {
        t.instrument = val & 0x7F;
        sendProgramChange(t.channel, t.instrument);
    }
    needsRedraw = true;
}

// Adjust base note by delta (works for both melodic and drum tracks)
void adjustNote(uint8_t track, int8_t delta) {
    if (track >= SEQ_TRACKS) return;
    SeqTrack& t = patterns[activeSlot].tracks[track];
    if (t.channel == DRUM_CHANNEL) {
        t.note = (uint8_t)constrain((int)t.note + delta, DRUM_NOTE_MIN, DRUM_NOTE_MAX);
    } else {
        t.note = (uint8_t)constrain((int)t.note + delta, 0, 127);
    }
    needsRedraw = true;
}

// ── Song chain helpers ────────────────────────────────────────────────────────
void chainAppend(uint8_t slot) {
    if (songChain.len >= SONG_CHAIN_LEN) return;
    songChain.entries[songChain.len++] = slot % MAX_PATTERNS;
    needsRedraw = true;
}
void chainRemoveLast() {
    if (songChain.len == 0) return;
    songChain.len--;
    needsRedraw = true;
}
void chainClear() {
    songChain.len = 0; songChain.playPos = 0;
    needsRedraw = true;
}

// ── Init ──────────────────────────────────────────────────────────────────────
void initSequencer() {
    for (uint8_t i = 0; i < MAX_PATTERNS; i++) _initPattern(patterns[i]);
    songChain = {};
    memset(_lastNote,   0, sizeof(_lastNote));
    memset(_lastNoteOn, 0, sizeof(_lastNoteOn));

    _seqTimer = timerBegin(0, 80, true);        // 80 MHz / 80 = 1 MHz
    timerAttachInterrupt(_seqTimer, &_onSeqTick, true);
    timerAlarmWrite(_seqTimer, _bpmUs(BPM_DEFAULT), true);
    // Timer alarm stays disabled until startSeq()
    Serial.println("[seq] Sequencer ready");
}

// ── Main loop pump ────────────────────────────────────────────────────────────
void runSequencer() {
    if (!_stepPend || seqState != SEQ_PLAYING) return;
    _stepPend = false;
    processStepAdvance();
}
