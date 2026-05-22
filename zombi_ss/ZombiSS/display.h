#pragma once

#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"

// SH1106 1.3" 128×64 — full-frame buffer, hardware I2C, no reset pin wired
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ── Helpers ───────────────────────────────────────────────────────────────────
// pct: 0–100; uses drawFrame + drawBox (no drawCircle — SH1106 column-offset quirk)
static void drawBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pct) {
    u8g2.drawFrame(x, y, w, h);
    uint8_t fill = (uint8_t)((pct / 100.0f) * (w - 2));
    if (fill > 0) u8g2.drawBox(x + 1, y + 1, fill, h - 2);
}

static void centreStr(const char *s, uint8_t y) {
    u8g2.drawStr((128 - u8g2.getStrWidth(s)) / 2, y, s);
}

// ── Boot screen ───────────────────────────────────────────────────────────────
void drawBootScreen() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_9x18B_tr);
    centreStr("ZOMBI", 20);

    u8g2.setFont(u8g2_font_7x14B_tr);
    centreStr("SS", 35);

    u8g2.setFont(u8g2_font_5x7_tr);
    centreStr("GM MIDI PLAYER v2.0", 48);
    centreStr("ESP32 + VS1053B", 58);

    u8g2.sendBuffer();
}

// ── No-files screen ───────────────────────────────────────────────────────────
void drawNoFilesScreen() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 8, "ZOMBI SS");
    u8g2.drawHLine(0, 9, 128);

    u8g2.setFont(u8g2_font_7x14B_tr);
    centreStr("NO MIDI FILES", 32);

    u8g2.setFont(u8g2_font_5x7_tr);
    centreStr("Add .MID to SD card", 48);
    centreStr("FAT32  root dir", 58);

    u8g2.sendBuffer();
}

// ── Main player screen ────────────────────────────────────────────────────────
void drawMainScreen() {
    // ── Derive display values ─────────────────────────────────────────────────
    const char *stateStr = "STOP";
    if      (appState == PLAYING) stateStr = "PLAY";
    else if (appState == PAUSED)  stateStr = "PAUS";

    // Song name: strip extension, cap at 14 chars for 6x10 font
    char sName[15] = "---";
    if (songCount > 0 && songIndex < songCount) {
        strncpy(sName, songNames[songIndex], 14);
        sName[14] = '\0';
        for (int i = strlen(sName) - 1; i > 0; i--) {
            if (sName[i] == '.') { sName[i] = '\0'; break; }
        }
    }

    // Elapsed time (wall-clock approximation)
    uint32_t elSec = 0;
    if (appState == PLAYING && playStartMs > 0)
        elSec = (millis() - playStartMs) / 1000UL;
    char timeBuf[7];
    snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", elSec / 60, elSec % 60);

    // Song counter
    char cntBuf[8];
    snprintf(cntBuf, sizeof(cntBuf), "%02d/%02d", songIndex + 1, songCount);

    // Volume / reverb percentages
    uint8_t volPct = volume;                      // already 0–100
    uint8_t revPct = (reverbLevel * 100U) / 127U;

    // ── Draw ─────────────────────────────────────────────────────────────────
    u8g2.clearBuffer();

    // Header bar: "ZOMBI SS" left, state right
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 8, "ZOMBI SS");
    u8g2.drawStr(128 - u8g2.getStrWidth(stateStr) - 2, 8, stateStr);
    u8g2.drawHLine(0, 9, 128);

    // Song name (larger font)
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 23, sName);

    // Elapsed time | file counter
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 34, timeBuf);
    u8g2.drawStr(128 - u8g2.getStrWidth(cntBuf) - 2, 34, cntBuf);

    u8g2.drawHLine(0, 36, 128);

    // Volume row: label + bar + value
    u8g2.drawStr(2, 46, "VOL");
    drawBar(22, 39, 70, 7, volPct);
    char volStr[5];
    snprintf(volStr, sizeof(volStr), "%3d%%", volPct);
    u8g2.drawStr(96, 46, volStr);

    // Reverb row: label + bar + value
    u8g2.drawStr(2, 57, "REV");
    drawBar(22, 50, 70, 7, revPct);
    char revStr[5];
    snprintf(revStr, sizeof(revStr), "%3d%%", revPct);
    u8g2.drawStr(96, 57, revStr);

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

    if (appState == BOOT)    { drawBootScreen();   return; }
    if (songCount == 0)      { drawNoFilesScreen(); return; }
    drawMainScreen();
}
