#pragma once
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"
#include "gm_names.h"
#include "juno_synth.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ── Helpers ───────────────────────────────────────────────────────────────
static void _bar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pct) {
    u8g2.drawFrame(x, y, w, h);
    uint8_t fill = (uint8_t)((pct / 100.0f) * (w - 2));
    if (fill) u8g2.drawBox(x+1, y+1, fill, h-2);
}
static void _centre(const char* s, uint8_t y) {
    u8g2.drawStr((128 - u8g2.getStrWidth(s)) / 2, y, s);
}
static void _trimName(const char* src, char* dst, uint8_t len) {
    strncpy(dst, src, len-1); dst[len-1]='\0';
    for (int i=strlen(dst)-1; i>=0 && dst[i]==' '; i--) dst[i]='\0';
}

// Track type suffix: " " = GM, "D" = DRUM, "J" = JUNO
static char _trackSuffix(uint8_t t) {
    switch (patterns[activeSlot].tracks[t].type) {
        case TRACK_DRUM: return 'D';
        case TRACK_JUNO: return 'J';
        default:         return ' ';
    }
}

// 6 mode tabs across 128px
static void _modeTabs() {
    static const char* L[] = {"STEP","CTRL","INST","SONG"};
    u8g2.setFont(u8g2_font_5x7_tr);
    for (uint8_t i=0;i<4;i++) {
        uint8_t tx=i*32;
        if (i==(uint8_t)appMode) {
            u8g2.drawBox(tx, 56, 32, 8);
            u8g2.setDrawColor(0); u8g2.drawStr(tx+3, 63, L[i]); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 56, 32, 8);
            u8g2.drawStr(tx+3, 63, L[i]);
        }
    }
}

// 6 track mini-tabs, 14px each = 84px, slot info at x=86
static void _trackMiniTabs() {
    u8g2.setFont(u8g2_font_4x6_tr);
    for (uint8_t i=0;i<SEQ_TRACKS;i++) {
        uint8_t tx = i * 14;
        char lbl[4]; snprintf(lbl, sizeof(lbl), "T%u%c", i+1, _trackSuffix(i));
        if (i==currentTrack) {
            u8g2.drawBox(tx, 35, 13, 7);
            u8g2.setDrawColor(0); u8g2.drawStr(tx+1, 41, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 35, 13, 7);
            u8g2.drawStr(tx+1, 41, lbl);
        }
    }
    // Slot / song info to the right of tabs
    char slotStr[10];
    if (songMode && songChain.len>0)
        snprintf(slotStr, sizeof(slotStr), "S%02u/%02u", activeSlot, songChain.len);
    else
        snprintf(slotStr, sizeof(slotStr), "P%02u/%02u", activeSlot, MAX_PATTERNS-1);
    u8g2.drawStr(86, 41, slotStr);
}

// ── Boot screen ───────────────────────────────────────────────────────────
void drawBootScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x18B_tr); _centre("ZOMBI",  20);
    u8g2.setFont(u8g2_font_7x14B_tr); _centre("SS v2",  35);
    u8g2.setFont(u8g2_font_5x7_tr);
    _centre("6-TRACK GM+JUNO SEQ",  48);
    _centre("ESP32 VS1053 PCM5102", 58);
    u8g2.sendBuffer();
}

// ── STEP mode ─────────────────────────────────────────────────────────────
void drawStepScreen() {
    Pattern&  p = patterns[activeSlot];
    SeqTrack& t = p.tracks[currentTrack];

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    // Header
    char trkLbl[4]; snprintf(trkLbl, sizeof(trkLbl), "T%u%c", currentTrack+1, _trackSuffix(currentTrack));
    u8g2.drawStr(0, 8, trkLbl);

    char nm[10];
    if (t.type==TRACK_DRUM)
        _trimName(drumName(t.note), nm, sizeof(nm));
    else if (t.type==TRACK_GM)
        _trimName(gmName(t.instrument), nm, sizeof(nm));
    else
        strncpy(nm, junoWaveName(junoOsc.wave1), sizeof(nm)-1);
    u8g2.drawStr(22, 8, nm);

    char bpmStr[5]; snprintf(bpmStr, sizeof(bpmStr), "%3u", p.bpm);
    u8g2.drawStr(128 - u8g2.getStrWidth(bpmStr), 8, bpmStr);
    u8g2.drawHLine(0, 9, 128);

    // Step grid
    for (uint8_t s=0;s<SEQ_STEPS;s++) {
        uint8_t bx = 4 + (s%8)*15;
        uint8_t by = (s<8) ? 11 : 22;
        if (t.steps[s].active)
            u8g2.drawBox(bx, by, 14, 8);
        else
            u8g2.drawFrame(bx, by, 14, 8);
    }

    // Playhead
    if (seqState==SEQ_PLAYING) {
        uint8_t cs=currentStep;
        uint8_t cx=4+(cs%8)*15+6;
        uint8_t cy=(cs<8)?20:31;
        u8g2.drawPixel(cx, cy);
        u8g2.drawHLine(cx-1, cy+1, 3);
        u8g2.drawHLine(cx-2, cy+2, 5);
    }

    _trackMiniTabs();
    _modeTabs();
    u8g2.sendBuffer();
}

// ── CTRL mode ─────────────────────────────────────────────────────────────
void drawCtrlScreen() {
    Pattern&  p = patterns[activeSlot];
    SeqTrack& t = p.tracks[currentTrack];

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    char hdr[22]; snprintf(hdr, sizeof(hdr), "CTRL P:%02u  BPM:%3u", activeSlot, p.bpm);
    u8g2.drawStr(0, 8, hdr);
    u8g2.drawHLine(0, 9, 128);

    // Track selector row 1 (T1-T4)
    for (uint8_t i=0;i<4;i++) {
        uint8_t tx=i*32;
        char lbl[5]; snprintf(lbl, sizeof(lbl), "T%u%c ", i+1, _trackSuffix(i));
        if (i==currentTrack) {
            u8g2.drawBox(tx, 11, 31, 9);
            u8g2.setDrawColor(0); u8g2.drawStr(tx+2, 19, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 11, 31, 9);
            u8g2.drawStr(tx+2, 19, lbl);
        }
    }
    // Track selector row 2 (T5-T6)
    for (uint8_t i=4;i<SEQ_TRACKS;i++) {
        uint8_t tx=(i-4)*32;
        char lbl[5]; snprintf(lbl, sizeof(lbl), "T%u%c ", i+1, _trackSuffix(i));
        if (i==currentTrack) {
            u8g2.drawBox(tx, 21, 31, 9);
            u8g2.setDrawColor(0); u8g2.drawStr(tx+2, 29, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(tx, 21, 31, 9);
            u8g2.drawStr(tx+2, 29, lbl);
        }
    }

    // Track detail
    u8g2.setFont(u8g2_font_4x6_tr);
    if (t.type==TRACK_DRUM) {
        char dn[22]; snprintf(dn, sizeof(dn), "DRM:%3u", t.note);
        u8g2.drawStr(0, 37, dn);
        char dname[10]; _trimName(drumName(t.note), dname, sizeof(dname));
        u8g2.drawStr(36, 37, dname);
    } else if (t.type==TRACK_GM) {
        char nn[5]; noteName(t.note, nn, sizeof(nn));
        char mi[24]; snprintf(mi, sizeof(mi), "GM:%3u NOTE:%-4s CH:%u", t.instrument+1, nn, t.channel+1);
        u8g2.drawStr(0, 37, mi);
    } else {
        // JUNO track
        char nn[5]; noteName(t.note, nn, sizeof(nn));
        char ji[24]; snprintf(ji, sizeof(ji), "JUNO  NOTE:%-4s", nn);
        u8g2.drawStr(0, 37, ji);
        u8g2.drawStr(80, 37, junoWaveName(junoOsc.wave1));
    }

    u8g2.setFont(u8g2_font_5x7_tr);
    // Volume bar
    u8g2.drawStr(0, 46, "V");
    _bar(8, 40, 55, 5, volume);
    char vs[6]; snprintf(vs, sizeof(vs), "%3u%%", volume);
    u8g2.drawStr(66, 46, vs);

    // Reverb bar
    u8g2.drawStr(0, 54, "R");
    _bar(8, 48, 55, 5, (reverbLevel*100U)/127U);
    char rs[6]; snprintf(rs, sizeof(rs), "%3u%%", (reverbLevel*100U)/127U);
    u8g2.drawStr(66, 54, rs);

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(96, 46, seqState==SEQ_PLAYING ? "[PLY]" : "[STP]");

    _modeTabs();
    u8g2.sendBuffer();
}

// ── INST mode ─────────────────────────────────────────────────────────────
void drawInstScreen() {
    SeqTrack& t = patterns[activeSlot].tracks[currentTrack];

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    if (t.type==TRACK_JUNO) {
        // ── JUNO patch editor ──────────────────────────────────────────
        char hdr[22]; snprintf(hdr, sizeof(hdr), "JUNO T%u  PATCH", currentTrack+1);
        u8g2.drawStr(0, 8, hdr);
        u8g2.drawHLine(0, 9, 128);

        u8g2.setFont(u8g2_font_7x14B_tr);
        _centre(junoWaveName(junoOsc.wave1), 26);

        u8g2.setFont(u8g2_font_5x7_tr);
        char fc[22]; snprintf(fc, sizeof(fc), "FC:%4.0f  Res:%3.0f%%",
            junoFilt.cutoff, junoFilt.reso * 100.0f / J_MAX_RESO);
        _centre(fc, 36);

        char atk[22]; snprintf(atk, sizeof(atk), "Atk:%4ums  Rel:%4ums",
            junoEnv.ampAttack, junoEnv.ampRelease);
        _centre(atk, 44);

        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(0, 50, "[-FC][-fc][+fc][+FC] SAW SQR TRI SIN");
        u8g2.drawStr(0, 55, "[Res-][Res+][Atk-][Atk+]  [D]=MODE");

    } else if (t.type==TRACK_DRUM) {
        // ── Drum browser (same as v1) ──────────────────────────────────
        char hdr[22]; snprintf(hdr, sizeof(hdr), "INST T%u (DRUMS)", currentTrack+1);
        u8g2.drawStr(0, 8, hdr);
        u8g2.drawHLine(0, 9, 128);

        u8g2.setFont(u8g2_font_7x14B_tr);
        char bigName[14]; _trimName(drumName(t.note), bigName, sizeof(bigName));
        _centre(bigName, 28);

        u8g2.setFont(u8g2_font_5x7_tr);
        char numStr[20]; snprintf(numStr, sizeof(numStr), "NOTE %u / %u", t.note, DRUM_NOTE_MAX);
        _centre(numStr, 39);

        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(0, 49, "[-5][-1][+1][+5]  KCK SNR HHC FTM");
        u8g2.drawStr(0, 55, "                  CRS CBL CON PRC");

    } else {
        // ── GM instrument browser (same as v1) ────────────────────────
        char hdr[22]; snprintf(hdr, sizeof(hdr), "INST T%u (GM)  ", currentTrack+1);
        u8g2.drawStr(0, 8, hdr);
        u8g2.drawHLine(0, 9, 128);

        u8g2.setFont(u8g2_font_7x14B_tr);
        char bigName[14]; _trimName(gmName(t.instrument), bigName, sizeof(bigName));
        _centre(bigName, 28);

        u8g2.setFont(u8g2_font_5x7_tr);
        char numStr[20]; snprintf(numStr, sizeof(numStr), "GM %u / 128", t.instrument+1);
        _centre(numStr, 39);

        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(0, 49, "[-10][-1][+1][+10] PNO BAS SYN STR");
        u8g2.drawStr(0, 55, "[D]=NEXT MODE      BRS PAD CHR SFX");
    }

    _modeTabs();
    u8g2.sendBuffer();
}

// ── SONG mode ─────────────────────────────────────────────────────────────
void drawSongScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);

    char hdr[26]; snprintf(hdr, sizeof(hdr), "SONG P:%02u %s  [%u]",
        activeSlot, songMode?"CHN":"SGL", songChain.len);
    u8g2.drawStr(0, 8, hdr);
    u8g2.drawHLine(0, 9, 128);

    u8g2.setFont(u8g2_font_4x6_tr);
    for (uint8_t i=0;i<12;i++) {
        uint8_t bx=(i%4)*32, by=11+(i/4)*8;
        char lbl[5]; snprintf(lbl, sizeof(lbl), "P%02u", i);
        if (i==activeSlot) {
            u8g2.drawBox(bx, by, 30, 7);
            u8g2.setDrawColor(0); u8g2.drawStr(bx+3, by+6, lbl); u8g2.setDrawColor(1);
        } else {
            u8g2.drawFrame(bx, by, 30, 7);
            u8g2.drawStr(bx+3, by+6, lbl);
        }
    }

    u8g2.drawHLine(0, 36, 128);
    if (songChain.len==0) {
        u8g2.drawStr(0, 43, "Chain: empty  [0]=append [TOG]=mode");
    } else {
        char chain[34]="";
        uint8_t show=min((uint8_t)8, songChain.len);
        for (uint8_t i=0;i<show;i++) {
            char e[6]; snprintf(e, sizeof(e), i==0?"P%02u":">P%02u", songChain.entries[i]);
            strncat(chain, e, sizeof(chain)-strlen(chain)-1);
        }
        if (songChain.len>8) strncat(chain,"...", sizeof(chain)-strlen(chain)-1);
        u8g2.drawStr(0, 43, chain);
    }

    u8g2.drawStr(0, 55, "[SAV]save [LNK]link [DEL]del  [D]>");

    _modeTabs();
    u8g2.sendBuffer();
}

// ── Init & dispatcher ─────────────────────────────────────────────────────
void initDisplay() {
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
}

void updateDisplay() {
    if (!needsRedraw) return;
    needsRedraw=false;
    switch (appMode) {
        case MODE_STEP: drawStepScreen();  break;
        case MODE_CTRL: drawCtrlScreen();  break;
        case MODE_INST: drawInstScreen();  break;
        case MODE_SONG: drawSongScreen();  break;
        default:        drawStepScreen();  break;
    }
}
