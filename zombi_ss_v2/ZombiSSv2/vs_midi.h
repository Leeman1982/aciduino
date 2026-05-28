#pragma once
#include <VS1053.h>
#include "config.h"

VS1053 vsPlayer(VS_CS, VS_DCS, VS_DREQ);

uint8_t volume      = VOLUME_DEFAULT;
uint8_t reverbLevel = REVERB_DEFAULT;

inline void sendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
    vsPlayer.sendMidiMessage(0x90 | (ch & 0x0F), note & 0x7F, vel & 0x7F);
}
inline void sendNoteOff(uint8_t ch, uint8_t note) {
    vsPlayer.sendMidiMessage(0x80 | (ch & 0x0F), note & 0x7F, 0);
}
inline void sendCC(uint8_t ch, uint8_t cc, uint8_t val) {
    vsPlayer.sendMidiMessage(0xB0 | (ch & 0x0F), cc & 0x7F, val & 0x7F);
}
inline void sendProgramChange(uint8_t ch, uint8_t prog) {
    vsPlayer.sendMidiMessage(0xC0 | (ch & 0x0F), prog & 0x7F, 0);
}

void applyReverb(uint8_t level) {
    for (uint8_t ch=0; ch<16; ch++) sendCC(ch, 91, level);
}

void setMasterVolume(uint8_t v) {
    volume = (v > 100) ? 100 : v;
    vsPlayer.setVolume(100 - volume);
    needsRedraw = true;
}

void initVsMidi() {
    vsPlayer.begin();
    vsPlayer.switchToMidi();
    vsPlayer.setVolume(100 - VOLUME_DEFAULT);
    for (uint8_t ch=0; ch<16; ch++) {
        sendCC(ch, 121, 0);
        if (ch != DRUM_CHANNEL) sendProgramChange(ch, 0);
    }
    applyReverb(REVERB_DEFAULT);
    Serial.println(F("[vs] VS1053 RTMIDI ready"));
}
