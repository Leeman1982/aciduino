#pragma once
#include <SD.h>
#include "config.h"

bool sdAvail = false;

// v2 magic bytes (different from v1 ZSSP/ZSSG to prevent cross-loading)
static const uint8_t MAGIC_PAT2[4] = {'Z','S','2','P'};
static const uint8_t MAGIC_SNG2[4] = {'Z','S','2','G'};

static void _patPath(uint8_t slot, char* buf) {
    snprintf(buf, 12, "/P%02u.ZS2", slot % MAX_PATTERNS);
}

void savePattern(uint8_t slot) {
    if (!sdAvail) { Serial.println(F("[stor] No SD")); return; }
    char path[12]; _patPath(slot, path);
    SD.remove(path);
    File f = SD.open(path, FILE_WRITE);
    if (!f) { Serial.printf("[stor] Open failed: %s\n", path); return; }
    f.write(MAGIC_PAT2, 4);
    f.write((const uint8_t*)&patterns[slot], sizeof(Pattern));
    f.close();
    Serial.printf("[stor] Saved P%02u → %s\n", slot, path);
}

void loadAllPatterns() {
    if (!sdAvail) return;
    char path[12];
    for (uint8_t i=0;i<MAX_PATTERNS;i++) {
        _patPath(i, path);
        File f = SD.open(path, FILE_READ);
        if (!f) continue;
        uint8_t magic[4];
        f.read(magic, 4);
        if (memcmp(magic, MAGIC_PAT2, 4)==0 && f.size()>=4+sizeof(Pattern)) {
            f.read((uint8_t*)&patterns[i], sizeof(Pattern));
            patterns[i].bpm    = constrain(patterns[i].bpm,    BPM_MIN, BPM_MAX);
            patterns[i].length = constrain(patterns[i].length, 1, SEQ_STEPS);
            Serial.printf("[stor] Loaded P%02u\n", i);
        }
        f.close();
    }
}

void saveSongChain() {
    if (!sdAvail) return;
    SD.remove("/SONG.ZS2");
    File f = SD.open("/SONG.ZS2", FILE_WRITE);
    if (!f) return;
    f.write(MAGIC_SNG2, 4);
    f.write((const uint8_t*)&songChain, sizeof(SongChain));
    f.close();
    Serial.printf("[stor] Saved song chain (%u entries)\n", songChain.len);
}

void loadSongChain() {
    if (!sdAvail) return;
    File f = SD.open("/SONG.ZS2", FILE_READ);
    if (!f) return;
    uint8_t magic[4];
    f.read(magic, 4);
    if (memcmp(magic, MAGIC_SNG2, 4)==0 && f.size()>=4+sizeof(SongChain)) {
        f.read((uint8_t*)&songChain, sizeof(SongChain));
        songChain.playPos=0;
        Serial.printf("[stor] Loaded song chain (%u entries)\n", songChain.len);
    }
    f.close();
}

void initStorage() {
    sdAvail = SD.begin(SD_CS_PIN);
    if (!sdAvail) { Serial.println(F("[stor] No SD — patterns won't persist")); return; }
    Serial.println(F("[stor] SD mounted"));
    loadAllPatterns();
    loadSongChain();
}
