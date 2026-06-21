/*!
 *  @file       mux_keypad.h
 *  Project     Aciduino V2 - 4x4 keypad over a CD74HC4067 analog multiplexer
 *  @brief      Reads 16 momentary keys through a single CD74HC4067 (16->1 mux,
 *              4 select lines + 1 signal) and feeds them into the uCtrl page
 *              navigation system - so the keypad drives the full Aciduino menu
 *              without consuming 16 GPIOs.
 *
 *  Each key shorts its mux channel to GND; the shared SIG line needs an external
 *  pull-up (10k to 3V3) because GPIO34-39 have no internal pull-ups. Pressed = LOW.
 *
 *  NAV keys are injected as uCtrl digital events (same path the on-board nav
 *  buttons use): uCtrl.page->processEvent(control_id, pressed, DIGITAL_EVENT),
 *  where pressed = 1 on press / 0 on release (matches uCtrl Din semantics).
 *  ACTION keys call a global Aciduino transport function on press.
 *
 *  @license    MIT
 */
#ifndef __ACIDUINO_MUX_KEYPAD_H__
#define __ACIDUINO_MUX_KEYPAD_H__

#include <Arduino.h>
#include "../uCtrl/uCtrl.h"
#include "vs1053_midi.h"   // for vs1053_panic()

//============================================
// Pinout (overridable from the port header)
//============================================
#ifndef MUX_PIN_S0
#define MUX_PIN_S0   4
#endif
#ifndef MUX_PIN_S1
#define MUX_PIN_S1   15
#endif
#ifndef MUX_PIN_S2
#define MUX_PIN_S2   13
#endif
#ifndef MUX_PIN_S3
#define MUX_PIN_S3   12
#endif
#ifndef MUX_PIN_SIG
#define MUX_PIN_SIG  35   // input-only + external 10k pull-up
#endif

#ifndef MUX_KEYPAD_SCAN_MS
#define MUX_KEYPAD_SCAN_MS  2     // debounce / scan period
#endif

// A key either injects a uCtrl navigation control, or fires a one-shot action.
enum MuxKeyKind { KEY_NAV, KEY_ACTION };
typedef struct {
  uint8_t kind;       // KEY_NAV or KEY_ACTION
  int16_t code;       // NAV: BUTTONS_INTERFACE_CONTROLS id | ACTION: index into action table
} MuxKeyMap;

// One-shot actions (called on press only)
static void muxAction_panic()        { vs1053_panic(); }
enum { ACT_PLAYSTOP, ACT_REC, ACT_PREV_TRACK, ACT_NEXT_TRACK, ACT_PANIC };
static void muxRunAction(int16_t a) {
  switch (a) {
    case ACT_PLAYSTOP:   Aciduino::playStop();      break;
    case ACT_REC:        Aciduino::recToggle();     break;
    case ACT_PREV_TRACK: Aciduino::previousTrack(); break;
    case ACT_NEXT_TRACK: Aciduino::nextTrack();     break;
    case ACT_PANIC:      muxAction_panic();         break;
  }
}

//============================================
// Default key map.
// Index = mux channel (0..15). Wire your keypad keys to channels in this order
// (keypad face reading left->right, top->bottom): 1 2 3 A / 4 5 6 B / 7 8 9 C / * 0 # D.
// Re-order freely to match how you physically wire the keypad to the 4067.
//============================================
static const MuxKeyMap muxKeyMap[16] = {
  /* ch0  '1' */ { KEY_NAV,    PAGE_BUTTON_1    },   // F1 / page
  /* ch1  '2' */ { KEY_NAV,    UP_BUTTON        },
  /* ch2  '3' */ { KEY_NAV,    PAGE_BUTTON_2    },   // F2 / page
  /* ch3  'A' */ { KEY_NAV,    SHIFT_BUTTON     },
  /* ch4  '4' */ { KEY_NAV,    PREVIOUS_BUTTON  },   // LEFT
  /* ch5  '5' */ { KEY_NAV,    DOWN_BUTTON      },
  /* ch6  '6' */ { KEY_NAV,    NEXT_BUTTON      },   // RIGHT
  /* ch7  'B' */ { KEY_ACTION, ACT_PLAYSTOP     },
  /* ch8  '7' */ { KEY_NAV,    GENERIC_BUTTON_1 },   // value - / (shift) prev track
  /* ch9  '8' */ { KEY_ACTION, ACT_REC          },
  /* ch10 '9' */ { KEY_NAV,    GENERIC_BUTTON_2 },   // value + / (shift) next track
  /* ch11 'C' */ { KEY_ACTION, ACT_PREV_TRACK   },
  /* ch12 '*' */ { KEY_ACTION, ACT_NEXT_TRACK   },
  /* ch13 '0' */ { KEY_ACTION, ACT_PANIC        },
  /* ch14 '#' */ { KEY_NAV,    SHIFT_BUTTON     },   // spare: duplicate shift
  /* ch15 'D' */ { KEY_ACTION, ACT_PLAYSTOP     },   // spare: duplicate play/stop
};

static uint16_t muxKeyState = 0;        // debounced state bitmap (1 = pressed)
static uint16_t muxKeyRaw   = 0;        // last raw sample
static uint32_t muxLastScan = 0;

static inline void muxKeypadInit() {
  pinMode(MUX_PIN_S0, OUTPUT);
  pinMode(MUX_PIN_S1, OUTPUT);
  pinMode(MUX_PIN_S2, OUTPUT);
  pinMode(MUX_PIN_S3, OUTPUT);
  pinMode(MUX_PIN_SIG, INPUT);          // external pull-up required on SIG
  muxKeyState = 0;
  muxKeyRaw = 0;
}

static inline void muxSelect(uint8_t ch) {
  digitalWrite(MUX_PIN_S0, (ch >> 0) & 0x01);
  digitalWrite(MUX_PIN_S1, (ch >> 1) & 0x01);
  digitalWrite(MUX_PIN_S2, (ch >> 2) & 0x01);
  digitalWrite(MUX_PIN_S3, (ch >> 3) & 0x01);
}

// Call from loop(). Scans all 16 channels, debounces (two equal samples), and
// dispatches edges to uCtrl navigation / Aciduino actions.
static inline void muxKeypadScan() {
  uint32_t now = millis();
  if (now - muxLastScan < MUX_KEYPAD_SCAN_MS) return;
  muxLastScan = now;

  uint16_t sample = 0;
  for (uint8_t ch = 0; ch < 16; ch++) {
    muxSelect(ch);
    delayMicroseconds(5);                       // mux settle
    if (digitalRead(MUX_PIN_SIG) == LOW) {      // pressed = LOW (key -> GND)
      sample |= (uint16_t)1 << ch;
    }
  }

  // simple debounce: a bit must read the same twice in a row to change state
  uint16_t stable = (sample & muxKeyRaw);            // pressed only if held two scans
  uint16_t released = (~sample & ~muxKeyRaw);        // released only if clear two scans
  muxKeyRaw = sample;

  for (uint8_t ch = 0; ch < 16; ch++) {
    uint16_t mask = (uint16_t)1 << ch;
    bool wasPressed = muxKeyState & mask;
    if (!wasPressed && (stable & mask)) {
      // press edge
      muxKeyState |= mask;
      const MuxKeyMap &k = muxKeyMap[ch];
      if (k.kind == KEY_NAV) {
        uCtrl.page->processEvent((uint8_t)k.code, 1, uctrl::module::DIGITAL_EVENT);
      } else {
        muxRunAction(k.code);
      }
    } else if (wasPressed && (released & mask)) {
      // release edge
      muxKeyState &= ~mask;
      const MuxKeyMap &k = muxKeyMap[ch];
      if (k.kind == KEY_NAV) {
        uCtrl.page->processEvent((uint8_t)k.code, 0, uctrl::module::DIGITAL_EVENT);
      }
    }
  }
}

#endif // __ACIDUINO_MUX_KEYPAD_H__
