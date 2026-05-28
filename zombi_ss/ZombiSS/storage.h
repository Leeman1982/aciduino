#pragma once
#include <SD.h>
#include "config.h"
// sequencer.h and vs_midi.h must be included before this (for patterns[] etc.)

// ── Defined here (extern'd in config.h) ──────────────────────────────────────
bool sdAvail = false;

static const uint8_t MAGIC_PAT[4] = {'Z','S','S','P'};
static const uint8_t MAGIC_SNG[4] = {'Z','S','S','G'};

static void _patPath(uint8_t slot, char* buf) {
    snprintf(buf, 12, "/P%02u.ZSS", slot % MAX_PATTERNS);
}

// ── Save current contents of patterns[slot] to SD ────────────────────────────
void savePattern(uint8_t slot) {
    if (!sdAvail) { Serial.println("[stor] No SD — cannot save"); return; }
    char path[12]; _patPath(slot, path);
    SD.remove(path);
    File f = SD.open(path, FILE_WRITE);
    if (!f) { Serial.printf("[stor] Open failed: %s\n", path); return; }
    f.write(MAGIC_PAT, 4);
    f.write((const uint8_t*)&patterns[slot], sizeof(Pattern));
    f.close();
    Serial.printf("[stor] Saved P%02u → %s (%u bytes)\n",
                  slot, path, sizeof(Pattern));
}

// ── Load all pattern slots from SD (called at startup) ───────────────────────
void loadAllPatterns() {
    if (!sdAvail) return;
    char path[12];
    for (uint8_t i = 0; i < MAX_PATTERNS; i++) {
        _patPath(i, path);
        File f = SD.open(path, FILE_READ);
        if (!f) continue;
        uint8_t magic[4];
        f.read(magic, 4);
        if (memcmp(magic, MAGIC_PAT, 4) == 0 && f.size() >= 4 + sizeof(Pattern)) {
            f.read((uint8_t*)&patterns[i], sizeof(Pattern));
            // Clamp BPM sanity
            patterns[i].bpm    = constrain(patterns[i].bpm, BPM_MIN, BPM_MAX);
            patterns[i].length = constrain(patterns[i].length, 1, SEQ_STEPS);
            Serial.printf("[stor] Loaded P%02u from %s\n", i, path);
        }
        f.close();
    }
}

// ── Save / load song chain ────────────────────────────────────────────────────
void saveSongChain() {
    if (!sdAvail) return;
    SD.remove("/SONG.ZSS");
    File f = SD.open("/SONG.ZSS", FILE_WRITE);
    if (!f) return;
    f.write(MAGIC_SNG, 4);
    f.write((const uint8_t*)&songChain, sizeof(SongChain));
    f.close();
    Serial.printf("[stor] Saved song chain (%u entries)\n", songChain.len);
}

void loadSongChain() {
    if (!sdAvail) return;
    File f = SD.open("/SONG.ZSS", FILE_READ);
    if (!f) return;
    uint8_t magic[4];
    f.read(magic, 4);
    if (memcmp(magic, MAGIC_SNG, 4) == 0 && f.size() >= 4 + sizeof(SongChain)) {
        f.read((uint8_t*)&songChain, sizeof(SongChain));
        songChain.playPos = 0;
        Serial.printf("[stor] Loaded song chain (%u entries)\n", songChain.len);
    }
    f.close();
}

// ── Init: mount SD, load all saved data ──────────────────────────────────────
void initStorage() {
    sdAvail = SD.begin(SD_CS_PIN);
    if (!sdAvail) {
        Serial.println("[stor] SD not found — patterns will not persist");
        return;
    }
    Serial.println("[stor] SD mounted");
    loadAllPatterns();
    loadSongChain();
}
