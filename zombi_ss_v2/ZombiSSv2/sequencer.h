#pragma once
#include <Arduino.h>
#include "config.h"
#include "vs_midi.h"
#include "juno_synth.h"

// ── Pattern storage ───────────────────────────────────────────────────────
Pattern    patterns[MAX_PATTERNS];
SongChain  songChain;

volatile uint8_t currentStep = 0;
uint8_t          activeSlot  = 0;
bool             songMode    = false;

static hw_timer_t*   _seqTimer  = nullptr;
static volatile bool _stepPend  = false;
static volatile uint8_t _tickDiv= 0;

static uint8_t _lastNote[SEQ_TRACKS];
static bool    _lastNoteOn[SEQ_TRACKS];

// ── ISR ───────────────────────────────────────────────────────────────────
void IRAM_ATTR _onSeqTick() {
    if (++_tickDiv >= TICKS_PER_STEP) { _tickDiv=0; _stepPend=true; }
}

// ── Default pattern ───────────────────────────────────────────────────────
static void _initPattern(Pattern& p) {
    p.bpm    = BPM_DEFAULT;
    p.length = SEQ_STEPS;
    // T1 GM  – Grand Piano  CH0  C5
    p.tracks[0] = { TRACK_GM,   0, 0,  60, {} };
    // T2 GM  – Finger Bass  CH1  C3
    p.tracks[1] = { TRACK_GM,   1, 33, 36, {} };
    // T3 DRUM – Bass Drum 1 CH9  note36
    p.tracks[2] = { TRACK_DRUM, DRUM_CHANNEL, 0, 36, {} };
    // T4 DRUM – Closed HH   CH9  note42
    p.tracks[3] = { TRACK_DRUM, DRUM_CHANNEL, 0, 42, {} };
    // T5 JUNO – Saw, C4
    p.tracks[4] = { TRACK_JUNO, 0, 0, 60, {} };
    // T6 JUNO – Saw, C3
    p.tracks[5] = { TRACK_JUNO, 0, 0, 48, {} };
    for (uint8_t t=0; t<SEQ_TRACKS; t++)
        for (uint8_t s=0; s<SEQ_STEPS; s++)
            p.tracks[t].steps[s] = {false, 100};
}

// ── Silence all ───────────────────────────────────────────────────────────
void seqPanic() {
    for (uint8_t ch=0; ch<16; ch++) {
        sendCC(ch, 120, 0);
        sendCC(ch, 123, 0);
    }
    junoAllNotesOff();
    for (uint8_t t=0; t<SEQ_TRACKS; t++) _lastNoteOn[t]=false;
}

// ── Apply program changes for all GM tracks in a pattern ─────────────────
static void _applyPatternPC(const Pattern& p) {
    for (uint8_t t=0; t<SEQ_TRACKS; t++)
        if (p.tracks[t].type == TRACK_GM)
            sendProgramChange(p.tracks[t].channel, p.tracks[t].instrument);
}

// ── Step advance (called from loop when _stepPend) ───────────────────────
void processStepAdvance() {
    // Note-off for all currently held notes
    for (uint8_t t=0; t<SEQ_TRACKS; t++) {
        if (_lastNoteOn[t]) {
            const SeqTrack& tr = patterns[activeSlot].tracks[t];
            if (tr.type == TRACK_JUNO)
                junoNoteOff(_lastNote[t]);
            else
                sendNoteOff(tr.channel, _lastNote[t]);
            _lastNoteOn[t] = false;
        }
    }

    // Advance step
    Pattern& p = patterns[activeSlot];
    currentStep = (currentStep + 1) % p.length;

    // Song chain: wrap → advance to next pattern
    if (currentStep==0 && songMode && songChain.len>0) {
        songChain.playPos = (songChain.playPos+1) % songChain.len;
        uint8_t next = songChain.entries[songChain.playPos];
        if (next != activeSlot) {
            activeSlot = next;
            _applyPatternPC(patterns[activeSlot]);
        }
    }

    // Note-on for current step
    Pattern& cur = patterns[activeSlot];
    for (uint8_t t=0; t<SEQ_TRACKS; t++) {
        const SeqStep&  st = cur.tracks[t].steps[currentStep];
        const SeqTrack& tr = cur.tracks[t];
        if (!st.active) continue;

        if (tr.type == TRACK_JUNO) {
            junoNoteOn(tr.note, st.velocity);
        } else {
            sendNoteOn(tr.channel, tr.note, st.velocity);
        }
        _lastNote[t]   = tr.note;
        _lastNoteOn[t] = true;
    }

    needsRedraw = true;
}

// ── BPM ──────────────────────────────────────────────────────────────────
static uint32_t _bpmUs(uint8_t bpm) {
    return 60000000UL / ((uint32_t)bpm * SEQ_PPQN);
}

void setSeqBpm(uint8_t bpm) {
    bpm = constrain(bpm, BPM_MIN, BPM_MAX);
    patterns[activeSlot].bpm = bpm;
    if (_seqTimer) timerAlarmWrite(_seqTimer, _bpmUs(bpm), true);
    needsRedraw = true;
}

// ── Start / stop ──────────────────────────────────────────────────────────
void startSeq() {
    if (seqState==SEQ_PLAYING) return;
    currentStep=0; _tickDiv=0; _stepPend=false;
    if (songMode && songChain.len>0) {
        songChain.playPos=0;
        activeSlot=songChain.entries[0];
    }
    memset(_lastNoteOn, 0, sizeof(_lastNoteOn));
    _applyPatternPC(patterns[activeSlot]);
    timerAlarmWrite(_seqTimer, _bpmUs(patterns[activeSlot].bpm), true);
    timerAlarmEnable(_seqTimer);
    seqState=SEQ_PLAYING; needsRedraw=true;
}

void stopSeq() {
    if (seqState==SEQ_STOPPED) return;
    timerAlarmDisable(_seqTimer);
    seqPanic();
    currentStep=0; _stepPend=false;
    seqState=SEQ_STOPPED; needsRedraw=true;
}

// ── Load slot ─────────────────────────────────────────────────────────────
void loadSlot(uint8_t slot) {
    slot = slot % MAX_PATTERNS;
    bool wasPlaying = (seqState==SEQ_PLAYING);
    if (wasPlaying) { timerAlarmDisable(_seqTimer); seqPanic(); }
    activeSlot = slot;
    if (wasPlaying) {
        currentStep=0; _tickDiv=0; _stepPend=false;
        memset(_lastNoteOn,0,sizeof(_lastNoteOn));
        _applyPatternPC(patterns[activeSlot]);
        timerAlarmWrite(_seqTimer,_bpmUs(patterns[activeSlot].bpm),true);
        timerAlarmEnable(_seqTimer);
    }
    needsRedraw=true;
}

// ── Edit helpers ──────────────────────────────────────────────────────────
void toggleStep(uint8_t track, uint8_t step) {
    if (track>=SEQ_TRACKS || step>=SEQ_STEPS) return;
    patterns[activeSlot].tracks[track].steps[step].active ^= true;
    needsRedraw=true;
}

void clearTrack(uint8_t track) {
    if (track>=SEQ_TRACKS) return;
    for (uint8_t s=0;s<SEQ_STEPS;s++)
        patterns[activeSlot].tracks[track].steps[s].active=false;
    needsRedraw=true;
}

void setInstrument(uint8_t track, uint8_t val) {
    if (track>=SEQ_TRACKS) return;
    SeqTrack& t = patterns[activeSlot].tracks[track];
    if (t.type==TRACK_DRUM) {
        t.note = constrain(val, DRUM_NOTE_MIN, DRUM_NOTE_MAX);
    } else if (t.type==TRACK_GM) {
        t.instrument = val & 0x7F;
        sendProgramChange(t.channel, t.instrument);
    }
    // TRACK_JUNO: instrument change handled via junoOsc globals
    needsRedraw=true;
}

void adjustNote(uint8_t track, int8_t delta) {
    if (track>=SEQ_TRACKS) return;
    SeqTrack& t = patterns[activeSlot].tracks[track];
    if (t.type==TRACK_DRUM)
        t.note = constrain((int)t.note+delta, DRUM_NOTE_MIN, DRUM_NOTE_MAX);
    else
        t.note = constrain((int)t.note+delta, 0, 127);
    needsRedraw=true;
}

// ── Song chain helpers ─────────────────────────────────────────────────────
void chainAppend(uint8_t slot) {
    if (songChain.len>=SONG_CHAIN_LEN) return;
    songChain.entries[songChain.len++] = slot % MAX_PATTERNS;
    needsRedraw=true;
}
void chainRemoveLast() {
    if (songChain.len==0) return;
    songChain.len--; needsRedraw=true;
}
void chainClear() {
    songChain.len=0; songChain.playPos=0; needsRedraw=true;
}

// ── Init ──────────────────────────────────────────────────────────────────
void initSequencer() {
    for (uint8_t i=0;i<MAX_PATTERNS;i++) _initPattern(patterns[i]);
    songChain={};
    memset(_lastNote,  0,sizeof(_lastNote));
    memset(_lastNoteOn,0,sizeof(_lastNoteOn));
    _seqTimer = timerBegin(0, 80, true);   // 1 MHz
    timerAttachInterrupt(_seqTimer, &_onSeqTick, true);
    timerAlarmWrite(_seqTimer, _bpmUs(BPM_DEFAULT), true);
    Serial.println(F("[seq] Sequencer ready"));
}

void runSequencer() {
    if (!_stepPend || seqState!=SEQ_PLAYING) return;
    _stepPend=false;
    processStepAdvance();
}
