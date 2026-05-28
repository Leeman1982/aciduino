#pragma once
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"
#include "gm_names.h"

// SH1106 1.3" 128x64 — full-frame buffer, HW I2C, no reset pin
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ── Helpers ───────────────────────────────────────────────────────────────────
static void _bar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pct) {
    u8g2.drawFrame(x, y, w, h);
    uint8_t fill = (uint8_t)((pct / 100.0f) * (w - 2));
    if (fill) u8g2.drawBox(x + 1, y + 1, fill, h - 2);
}
static void _centre(const char* s, uint8_t y) {
    u8g2.drawStr((128 - u8g2.getStrWidth(s)) / 2, y, s);
}

// Trim trailing spaces from a GM/drum name for cleaner display
static void _trimName(const char* src, char* dst, uint8_t dstLen) {
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
    for (int i = strlen(dst) - 1; i >= 0 && dst[i] == ' '; i--) dst[i] = '\0';
}

// Four bottom tabs — current mode shown inverted
static void _modeTabs() {
    static const char* L[] = {"STEP","CTRL","INST","SONG"};
    u8g2.setFont(u8g2_font_5x7_tr);
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t tx = i * 32;
        if (i == (uint8_t)appMode) {
            u8g2.drawBox(tx, 56, 32, 8);
            u8g2.setDrawColor(0); u8g2.drawStr(tx + 3, 63, L[i]); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 56, 32, 8);
            u8g2.drawStr(tx + 3, 63, L[i]);
        }
    }
}

// ── Boot screen ───────────────────────────────────────────────────────────────
void drawBootScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x18B_tr); _centre("ZOMBI", 20);
    u8g2.setFont(u8g2_font_7x14B_tr); _centre("SS",    35);
    u8g2.setFont(u8g2_font_5x7_tr);
    _centre("GM SEQUENCER v2.0", 48);
    _centre("ESP32 + VS1053B",   58);
    u8g2.sendBuffer();
}

// ── STEP mode ─────────────────────────────────────────────────────────────────
// Layout (y coords are baselines):
//   y=8      Header: "T1 <name>           BPM"
//   y=9      hline
//   y=11-18  Step blocks row 1 (steps 0-7)  — top-left at (4,11)
//   y=22-29  Step blocks row 2 (steps 8-15) — top-left at (4,22)
//   y=31-33  Playhead cursor row
//   y=44-53  Track mini-selector + slot label
//   y=56-63  Mode tabs
void drawStepScreen() {
    Pattern&  p = patterns[activeSlot];
    SeqTrack& t = p.tracks[currentTrack];

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    // ── Header ──────────────────────────────────────────────────────────────
    char trkLbl[3]; snprintf(trkLbl, sizeof(trkLbl), "T%u", currentTrack + 1);
    u8g2.drawStr(0, 8, trkLbl);
    char nm[10]; _trimName((t.channel == DRUM_CHANNEL) ? drumName(t.note) : gmName(t.instrument), nm, sizeof(nm));
    u8g2.drawStr(14, 8, nm);
    char bpmStr[5]; snprintf(bpmStr, sizeof(bpmStr), "%3u", p.bpm);
    u8g2.drawStr(128 - u8g2.getStrWidth(bpmStr), 8, bpmStr);
    u8g2.drawHLine(0, 9, 128);

    // ── Step grid — 2 rows × 8 steps, each block 14w×8h, 1px gap ─────────
    // Block at col c, row r: x = 4 + c*15,  y = 11 + r*11
    for (uint8_t s = 0; s < SEQ_STEPS; s++) {
        uint8_t bx = 4 + (s % 8) * 15;
        uint8_t by = (s < 8) ? 11 : 22;
        if (t.steps[s].active)
            u8g2.drawBox(bx, by, 14, 8);
        else
            u8g2.drawFrame(bx, by, 14, 8);
    }

    // ── Playhead: 3-pixel arrow below current step ─────────────────────────
    if (seqState == SEQ_PLAYING) {
        uint8_t cs = currentStep;
        uint8_t cx = 4 + (cs % 8) * 15 + 6;   // centre of block
        uint8_t cy = (cs < 8) ? 20 : 31;
        u8g2.drawPixel(cx, cy);
        u8g2.drawHLine(cx - 1, cy + 1, 3);
        u8g2.drawHLine(cx - 2, cy + 2, 5);
    }

    // ── Track mini-tabs ────────────────────────────────────────────────────
    u8g2.setFont(u8g2_font_4x6_tr);
    for (uint8_t i = 0; i < SEQ_TRACKS; i++) {
        uint8_t tx = i * 15;
        char lbl[4]; snprintf(lbl, sizeof(lbl), (patterns[activeSlot].tracks[i].channel == DRUM_CHANNEL) ? "T%uD" : " T%u", i+1);
        if (i == currentTrack) {
            u8g2.drawBox(tx, 35, 14, 7);
            u8g2.setDrawColor(0); u8g2.drawStr(tx+1, 41, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 35, 14, 7);
            u8g2.drawStr(tx+1, 41, lbl);
        }
    }

    // Slot / song info
    char slotStr[12];
    if (songMode && songChain.len > 0)
        snprintf(slotStr, sizeof(slotStr), "SNG P%02u/%02u", activeSlot, songChain.len);
    else
        snprintf(slotStr, sizeof(slotStr), "PAT %02u/%02u", activeSlot, MAX_PATTERNS - 1);
    u8g2.drawStr(66, 41, slotStr);

    _modeTabs();
    u8g2.sendBuffer();
}

// ── CTRL mode ─────────────────────────────────────────────────────────────────
// Row 1 → [T1][T2][T3][T4] track select
// Row 2 → PLY  STP  CLR  SAV
// Row 3 → BPM- BPM+ N-   N+
// Row 4 → VOL- VOL+ REV-cycle  D=next-mode
void drawCtrlScreen() {
    Pattern&  p = patterns[activeSlot];
    SeqTrack& t = p.tracks[currentTrack];

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    // Header
    char hdr[22]; snprintf(hdr, sizeof(hdr), "CTRL P:%02u  BPM:%3u", activeSlot, p.bpm);
    u8g2.drawStr(0, 8, hdr);
    u8g2.drawHLine(0, 9, 128);

    // Track selector boxes
    for (uint8_t i = 0; i < SEQ_TRACKS; i++) {
        uint8_t tx = i * 32;
        char lbl[5];
        bool isd = (patterns[activeSlot].tracks[i].channel == DRUM_CHANNEL);
        snprintf(lbl, sizeof(lbl), isd ? "T%uD" : " T%u ", i+1);
        if (i == currentTrack) {
            u8g2.drawBox(tx, 11, 31, 9);
            u8g2.setDrawColor(0); u8g2.drawStr(tx+2, 19, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 11, 31, 9);
            u8g2.drawStr(tx+2, 19, lbl);
        }
    }

    // Track detail
    if (t.channel == DRUM_CHANNEL) {
        char dn[22]; snprintf(dn, sizeof(dn), "DRM:%3u ", t.note);
        u8g2.drawStr(0, 29, dn);
        char dname[10]; _trimName(drumName(t.note), dname, sizeof(dname));
        u8g2.drawStr(50, 29, dname);
    } else {
        char nn[5]; noteName(t.note, nn, sizeof(nn));
        char mi[22]; snprintf(mi, sizeof(mi), "GM:%3u NOTE:%-4s CH:%u", t.instrument+1, nn, t.channel+1);
        u8g2.drawStr(0, 29, mi);
    }

    // VOL bar
    u8g2.drawStr(0, 41, "V");
    _bar(8, 34, 64, 6, volume);
    char vs[5]; snprintf(vs, sizeof(vs), "%3u%%", volume);
    u8g2.drawStr(76, 41, vs);

    // REV bar
    u8g2.drawStr(0, 51, "R");
    _bar(8, 44, 64, 6, (reverbLevel * 100U) / 127U);
    char rs[5]; snprintf(rs, sizeof(rs), "%3u%%", (reverbLevel * 100U) / 127U);
    u8g2.drawStr(76, 51, rs);

    // Key-hint row
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(96, 51, seqState == SEQ_PLAYING ? "[PLY]" : "[STP]");

    _modeTabs();
    u8g2.sendBuffer();
}

// ── INST mode ─────────────────────────────────────────────────────────────────
// Row 1 → navigation −large / −1 / +1 / +large
// Row 2-3 → bank/family jump shortcuts
// Row 4 → (D = next mode)
void drawInstScreen() {
    SeqTrack& t = patterns[activeSlot].tracks[currentTrack];
    bool isDrum = (t.channel == DRUM_CHANNEL);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    // Header
    char hdr[22];
    snprintf(hdr, sizeof(hdr), "INST T%u %s", currentTrack+1, isDrum ? "(DRUMS)" : "(GM)  ");
    u8g2.drawStr(0, 8, hdr);
    u8g2.drawHLine(0, 9, 128);

    // Big instrument name
    u8g2.setFont(u8g2_font_7x14B_tr);
    char bigName[14]; _trimName(isDrum ? drumName(t.note) : gmName(t.instrument), bigName, sizeof(bigName));
    _centre(bigName, 28);

    // Number indicator
    u8g2.setFont(u8g2_font_5x7_tr);
    char numStr[20];
    if (isDrum) snprintf(numStr, sizeof(numStr), "NOTE %u / %u", t.note, DRUM_NOTE_MAX);
    else        snprintf(numStr, sizeof(numStr), "GM %u / 128", t.instrument + 1);
    _centre(numStr, 39);

    // Key hints
    u8g2.setFont(u8g2_font_4x6_tr);
    if (isDrum) {
        u8g2.drawStr(0, 49, "[-5][-1][+1][+5]  KCK SNR HAT TOM");
        u8g2.drawStr(0, 55, "                  CYM BWL CON PRC");
    } else {
        u8g2.drawStr(0, 49, "[-10][-1][+1][+10] PNO BAS SYN STR");
        u8g2.drawStr(0, 55, "[D]=NEXT MODE      BRS PAD CHR SFX");
    }

    _modeTabs();
    u8g2.sendBuffer();
}

// ── SONG mode ─────────────────────────────────────────────────────────────────
// Row 1-3 → load pattern slots 0-11
// Row 4   → SAV  LNK  DEL  [D]=next-mode   (CLR via long SAV hold not implemented)
void drawSongScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    // Header
    char hdr[24]; snprintf(hdr, sizeof(hdr), "SONG P:%02u %s  [%u]",
        activeSlot, songMode ? "CHN" : "SGL", songChain.len);
    u8g2.drawStr(0, 8, hdr);
    u8g2.drawHLine(0, 9, 128);

    // Pattern slot grid 3×4 (slots 0-11) as small boxes
    u8g2.setFont(u8g2_font_4x6_tr);
    for (uint8_t i = 0; i < 12; i++) {
        uint8_t bx = (i % 4) * 32;
        uint8_t by = 11 + (i / 4) * 8;
        char lbl[5]; snprintf(lbl, sizeof(lbl), "P%02u", i);
        if (i == activeSlot) {
            u8g2.drawBox(bx, by, 30, 7);
            u8g2.setDrawColor(0); u8g2.drawStr(bx+3, by+6, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(bx, by, 30, 7);
            u8g2.drawStr(bx+3, by+6, lbl);
        }
    }

    // Chain visualization (up to 12 entries shown)
    u8g2.drawHLine(0, 36, 128);
    u8g2.setFont(u8g2_font_4x6_tr);
    if (songChain.len == 0) {
        u8g2.drawStr(0, 43, "Chain: empty  [LNK]=append active");
    } else {
        char chain[34] = "";
        uint8_t show = min((uint8_t)8, songChain.len);
        for (uint8_t i = 0; i < show; i++) {
            char e[6]; snprintf(e, sizeof(e), i == 0 ? "P%02u" : ">P%02u", songChain.entries[i]);
            strncat(chain, e, sizeof(chain) - strlen(chain) - 1);
        }
        if (songChain.len > 8) strncat(chain, "...", sizeof(chain) - strlen(chain) - 1);
        u8g2.drawStr(0, 43, chain);
    }

    // Bottom hints
    u8g2.drawStr(0, 55, "[SAV]save [LNK]link [DEL]del  [D]>");

    _modeTabs();
    u8g2.sendBuffer();
}

// ── Init & dispatcher ────────────────────────────────────────────────────────
void initDisplay() {
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
}

void updateDisplay() {
    if (!needsRedraw) return;
    needsRedraw = false;
    switch (appMode) {
        case MODE_STEP:    drawStepScreen();  break;
        case MODE_CTRL:    drawCtrlScreen();  break;
        case MODE_INST:    drawInstScreen();  break;
        case MODE_SONG:    drawSongScreen();  break;
        default:           drawStepScreen();  break;
    }
}
