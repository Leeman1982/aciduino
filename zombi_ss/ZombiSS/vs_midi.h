#pragma once

/*
 * VS1053B GM MIDI engine + MD_MIDIFile parser
 *
 * VS1053 in real-time MIDI mode (RTMIDI plugin auto-loaded by baldram library).
 * SD card (on VS1053 module) is accessed via the standard Arduino SD library on
 * the same SPI bus — each device has its own CS pin so bus sharing is safe.
 *
 * Hardware reverb: send GM CC91 (Reverb Send) to all 16 channels.
 * The VS1053B handles the actual reverb in its DSP — zero ESP32 CPU cost.
 */

#include <VS1053.h>        // baldram/ESP_VS1053_Library
#include <MD_MIDIFile.h>   // majicDesigns/MD_MIDIFile
#include <SD.h>
#include "config.h"

// ── VS1053 instance (CS, DCS, DREQ) ──────────────────────────────────────────
VS1053 vsPlayer(VS_CS, VS_DCS, VS_DREQ);

// ── MD_MIDIFile instance ──────────────────────────────────────────────────────
MD_MIDIFile SMF;

static bool _sdReady = false;

// ── MIDI event callback: MD_MIDIFile → VS1053 ────────────────────────────────
static void onMidiEvent(midi_event *pev) {
    if (!pev || pev->size == 0) return;
    uint8_t d1 = (pev->size > 1) ? pev->data[1] : 0;
    uint8_t d2 = (pev->size > 2) ? pev->data[2] : 0;
    vsPlayer.sendMidiMessage(pev->data[0], d1, d2);
}

// ── Sysex callback (required by MD_MIDIFile, ignored here) ───────────────────
static void onSysexEvent(sysex_event *pev) { (void)pev; }

// ── Send CC91 Reverb Send to all 16 MIDI channels ────────────────────────────
void applyReverb(uint8_t level) {
    for (uint8_t ch = 0; ch < 16; ch++)
        vsPlayer.sendMidiMessage(0xB0 | ch, 91, level);
}

// ── GM All-Notes-Off + All-Sound-Off on all channels ─────────────────────────
void midiPanic() {
    for (uint8_t ch = 0; ch < 16; ch++) {
        vsPlayer.sendMidiMessage(0xB0 | ch, 120, 0);  // CC120 All Sound Off
        vsPlayer.sendMidiMessage(0xB0 | ch, 123, 0);  // CC123 All Notes Off
    }
}

// ── GM Reset (SysEx F0 7E 7F 09 01 F7) ───────────────────────────────────────
static void gmReset() {
    // VS1053 MIDI SysEx is sent byte-by-byte via sendMidiMessage with 0xF0 cmd
    // Simplest approach: send each byte individually using 0x00 padding protocol
    // The baldram library's sendMidiMessage handles variable-length via cmd analysis.
    // For SysEx we send the raw bytes directly via the underlying SPI write.
    // Easiest reliable path: use a brief reset sequence via CC and program change.
    for (uint8_t ch = 0; ch < 16; ch++) {
        vsPlayer.sendMidiMessage(0xB0 | ch, 121, 0);  // CC121 Reset All Controllers
        vsPlayer.sendMidiMessage(0xC0 | ch, 0,   0);  // PC to instrument 0 (Grand Piano)
    }
    // Channel 9 (drums) uses bank 0x78 — set via CC0 bank MSB
    vsPlayer.sendMidiMessage(0xB9, 0, 0x78);  // Bank select MSB on ch9 = drum kit
}

// ── Build song list from SD card root ────────────────────────────────────────
void buildSongList() {
    songCount = 0;
    File root = SD.open("/");
    if (!root) return;
    File f = root.openNextFile();
    while (f && songCount < MAX_SONGS) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            String upper = name;
            upper.toUpperCase();
            if (upper.endsWith(".MID") || upper.endsWith(".MIDI")) {
                strncpy(songNames[songCount], f.name(), 12);
                songNames[songCount][12] = '\0';
                songCount++;
            }
        }
        f = root.openNextFile();
    }
    root.close();
    Serial.printf("[vs_midi] %d MIDI file(s) on SD\n", songCount);
    for (int i = 0; i < songCount; i++)
        Serial.printf("  [%02d] %s\n", i, songNames[i]);
}

// ── Load a song by index and prime the parser ─────────────────────────────────
static bool _loadSong(int idx) {
    if (!_sdReady || idx < 0 || idx >= songCount) return false;
    SMF.close();
    midiPanic();

    int8_t err = SMF.load(songNames[idx]);
    if (err != MD_MIDIFile::E_OK) {
        Serial.printf("[vs_midi] Load error %d: %s\n", err, songNames[idx]);
        return false;
    }
    playStartMs = millis();
    Serial.printf("[vs_midi] Loaded: %s\n", songNames[idx]);
    return true;
}

// ── Init ──────────────────────────────────────────────────────────────────────
void initVsMidi() {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    // VS1053 setup
    vsPlayer.begin();
    vsPlayer.switchToMidi();                       // loads RTMIDI plugin
    vsPlayer.setVolume(100 - volume);              // baldram: 0=max, 100=muted → invert
    gmReset();
    applyReverb(reverbLevel);

    // SD card
    _sdReady = SD.begin(SD_CS_PIN);
    if (!_sdReady) {
        Serial.println("[vs_midi] SD card not found — check wiring and FAT32 format");
        appState = STOPPED;
        return;
    }

    buildSongList();

    // MD_MIDIFile
    SMF.begin(&SD);
    SMF.setMidiHandler(onMidiEvent);
    SMF.setSysexHandler(onSysexEvent);

    if (songCount > 0) {
        _loadSong(0);
        appState = STOPPED;
    }
}

// ── Audio pump — call from loop() ────────────────────────────────────────────
void runMidi() {
    if (appState != PLAYING || !_sdReady || songCount == 0) return;

    if (SMF.isEOF()) {
        // Current song finished — auto-advance
        midiPanic();
        songIndex = (songIndex + 1) % songCount;
        _loadSong(songIndex);
        needsRedraw = true;
        return;
    }

    SMF.processEvents(millis());
}

// ── Transport ─────────────────────────────────────────────────────────────────
void midiPlay() {
    if (songCount == 0) return;
    if (appState == PAUSED || appState == STOPPED) {
        // MD_MIDIFile has no resume — restart from beginning of current song
        _loadSong(songIndex);
    }
    appState = PLAYING;
    needsRedraw = true;
}

void midiPause() {
    if (appState != PLAYING) return;
    midiPanic();
    appState = PAUSED;
    needsRedraw = true;
}

void midiStop() {
    midiPanic();
    SMF.close();
    if (songCount > 0) _loadSong(songIndex);  // reset position for next play
    appState = STOPPED;
    needsRedraw = true;
}

void midiNext() {
    bool wasPlaying = (appState == PLAYING);
    songIndex = (songIndex + 1) % songCount;
    _loadSong(songIndex);
    appState = wasPlaying ? PLAYING : STOPPED;
    needsRedraw = true;
}

void midiPrev() {
    bool wasPlaying = (appState == PLAYING);
    songIndex = (songIndex - 1 + songCount) % songCount;
    _loadSong(songIndex);
    appState = wasPlaying ? PLAYING : STOPPED;
    needsRedraw = true;
}

void midiSelectSong(int idx) {
    if (idx >= songCount) return;
    songIndex = idx;
    _loadSong(songIndex);
    appState = PLAYING;
    needsRedraw = true;
}

// volume 0–100 where 100 = loudest
void midiSetVolume(uint8_t v) {
    volume = (v > 100) ? 100 : v;
    vsPlayer.setVolume(100 - volume);  // invert for baldram library
    needsRedraw = true;
}

// level 0–127 (GM CC91 range)
void midiSetReverb(uint8_t level) {
    reverbLevel = level;
    applyReverb(reverbLevel);
    needsRedraw = true;
}
