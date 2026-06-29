#include "../../uCtrl/uCtrl.h"

#include "../../aciduino.hpp"

// On-board GM soundfont synth
#include "../../synth/gm_synth.h"

// =====================================================================
// Aciduino V2 - ESP32 WROOM port with on-board General MIDI synth
// =====================================================================
//
// Target hardware:
//   - ESP32 WROOM dev board (30 pin)
//   - 1.3" OLED (SH1106, 128x64, I2C)
//   - 1x rotary encoder with push button (value changer + SHIFT)
//   - 5x momentary push buttons (navigation)
//   - PCM5102 I2S DAC for audio out of the GM synth
//
// The sequencer/UI is the standard AciduinoV2 303/808 engine. Every note the
// sequencer plays is also rendered by the on-board GM synth (see
// src/synth/) and sent to the PCM5102. Set a track's MIDI channel to 10 in
// the MIDI page to play it through the GM percussion bank (good for the 808
// drum track).
//
// CONTROL SCHEME (6 inputs vs the 9-button reference layout)
// ----------------------------------------------------------
//   Encoder rotate ........ change the selected value
//   Encoder push (SHIFT) .. hold for the shifted function of a button
//   Button 1 ......... UP            | SHIFT = previous track
//   Button 2 ......... DOWN          | SHIFT = next track
//   Button 3 ......... PREVIOUS (<-) | SHIFT = REC on/off
//   Button 4 ......... NEXT (->)     | SHIFT = PLAY/STOP
//   Button 5 ......... PAGE cycle    | SHIFT = page back
//
// Because the midilab UI is designed around 9 buttons, the secondary roles
// (track switch, transport, page-back) are reached with SHIFT. Wire extra
// buttons and extend the plug()/setNavComponentCtrl() block below for a 1:1
// layout.

//============================================
// Aciduino Features Setup
//============================================

#define LED_BUILTIN       2
#define USE_BPM_LED       LED_BUILTIN

// main navigation: encoder as the value changer
#define USE_CHANGER_ENCODER
//#define USE_CHANGER_POT

#define FLIP_DISPLAY

// MIDI: keep a serial-to-midi bridge and a real DIN MIDI port available.
#define USE_SERIAL_MIDI_115200
#define USE_MIDI1 // USB serial-to-midi bridge
#define USE_MIDI3 // hardware MIDI (Serial2)

//============================================
// PINOUT Setup
//============================================
//
// I2C OLED  : SDA=21, SCL=22 (ESP32 hardware I2C defaults)
// I2S PCM5102: BCK=26, LRCK/WS=25, DIN=27   (see synth/gm_synth.h)
//
// Keep the audio + display pins clear of the nav pins below.

// rotary encoder (A/B). plugged first so they map to ENCODER_DEC/INC.
#define NAV_ENCODER_DEC_PIN       32
#define NAV_ENCODER_INC_PIN       33

// encoder push button -> SHIFT
#define NAV_SHIFT_PIN             13

// 5 momentary navigation buttons
#define NAV_BTN1_PIN              14
#define NAV_BTN2_PIN               4
#define NAV_BTN3_PIN               5
#define NAV_BTN4_PIN              18
#define NAV_BTN5_PIN              19

//============================================
// Managed Devices Setup
//============================================

// Display device: 1.3" SH1106 OLED over hardware I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Midi devices
#if defined(USE_MIDI1) // USB serial bridge
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI1);
#endif
#if defined(USE_MIDI3) // hardware DIN MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI3);
#endif

// the main interface choice
#include "../../interface/midilab/main.h"

void initPort() {
  //
  // OLED setup
  //
  uCtrl.initOled(&u8g2);
#if defined(FLIP_DISPLAY)
  uCtrl.oled->flipDisplay(1);
#endif

  //
  // Storage setup
  //
  uCtrl.initStorage();

  //
  // DIN Module (buttons / encoder)
  //
  uCtrl.initDin();

  // IMPORTANT: plug order defines the button ids (see consts.h enum).
  // encoder first (ENCODER_DEC, ENCODER_INC)
  uCtrl.din->plug(NAV_ENCODER_DEC_PIN);
  uCtrl.din->plug(NAV_ENCODER_INC_PIN);
  // then SHIFT_BUTTON ... onwards, in enum order
  uCtrl.din->plug(NAV_SHIFT_PIN);   // SHIFT_BUTTON
  uCtrl.din->plug(NAV_BTN1_PIN);    // PAGE_BUTTON_1   (used as UP)
  uCtrl.din->plug(NAV_BTN2_PIN);    // PAGE_BUTTON_2   (used as DOWN)
  uCtrl.din->plug(NAV_BTN3_PIN);    // GENERIC_BUTTON_1 (used as PREVIOUS)
  uCtrl.din->plug(NAV_BTN4_PIN);    // GENERIC_BUTTON_2 (used as NEXT)
  uCtrl.din->plug(NAV_BTN5_PIN);    // NEXT_BUTTON      (used as PAGE cycle)

  // encoder pair (odd-id start, in pair order)
  uCtrl.din->encoder(ENCODER_DEC, ENCODER_INC);

  //
  // DOUT Module
  //
  uCtrl.initDout();
#if defined(USE_BPM_LED)
  uCtrl.dout->plug(USE_BPM_LED);
#endif

  //
  // AIN Module (no pots on this build, encoder is the changer)
  //
  uCtrl.initAin();

  //
  // MIDI Module
  //
  uCtrl.initMidi();
#if defined(USE_MIDI1)
  uCtrl.midi->plug(&MIDI1);
  #if defined(USE_SERIAL_MIDI_115200)
  Serial.begin(115200);
  #endif
#endif
#if defined(USE_MIDI3)
  uCtrl.midi->plug(&MIDI3);
#endif
  uCtrl.midi->setMidiInputCallback(Aciduino::midiInputHandler);
  uCtrl.setOn250usCallback(Aciduino::midiHandleSync);
  uCtrl.setOn1msCallback(Aciduino::midiHandle);

  //
  // Page Module for UI
  //
  uCtrl.initPage(5);
  // syst | seqr | gene | ptrn | midi
  system_page_init();
  step_sequencer_page_init();
  generative_page_init();
  pattern_page_init();
  midi_page_init();

  // Navigation mapping for the 6-input layout.
  // setNavComponentCtrl(SHIFT, UP, DOWN, PREVIOUS, NEXT, PAGE1, PAGE2,
  //                     GENERIC1, GENERIC2, [ENCODER_DEC, ENCODER_INC])
  uCtrl.page->setNavComponentCtrl(
      SHIFT_BUTTON,       // shift    -> encoder push
      PAGE_BUTTON_1,      // up       -> button 1
      PAGE_BUTTON_2,      // down     -> button 2
      GENERIC_BUTTON_1,   // previous -> button 3
      GENERIC_BUTTON_2,   // next     -> button 4
      NEXT_BUTTON,        // page1    -> button 5
      NEXT_BUTTON,        // page2    -> button 5 (SHIFT for page back)
      PAGE_BUTTON_1,      // generic1 -> shared (track switch via shift below)
      PAGE_BUTTON_2,      // generic2 -> shared
      ENCODER_DEC, ENCODER_INC);

  // SHIFT combos for the functions without a dedicated button:
  // previous / next track on the UP / DOWN buttons
  uCtrl.page->setShiftCtrlAction(PAGE_BUTTON_1, Aciduino::previousTrack);
  uCtrl.page->setShiftCtrlAction(PAGE_BUTTON_2, Aciduino::nextTrack);
  // transport: PLAY/STOP on NEXT button, REC on PREVIOUS button (shifted)
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_2, Aciduino::playStop);
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_1, Aciduino::recToggle);

  // bottom bar for f1 and f2 functions draw function
  uCtrl.page->setFunctionDrawCallback(functionDrawCallback);

  // init uCtrl modules and memory
  uCtrl.init();

  // get all leds off
  uCtrl.dout->writeAll(LOW);

  // default page to call at init
  uCtrl.page->setPage(0);

  // sequencer init (4x 303 + 1x 808 by default, see sequencer/setup.h)
  aciduino.init();

  //
  // On-board General MIDI synth (PCM5102 I2S DAC)
  //
  gmSynthInit();
}
