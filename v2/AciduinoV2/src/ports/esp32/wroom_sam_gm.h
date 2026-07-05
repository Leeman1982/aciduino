#include "../../uCtrl/uCtrl.h"

#include "../../aciduino.hpp"

// =====================================================================
// Aciduino V2 - ESP32 WROOM port: MIDI controller for a SAM2695 GM module
// =====================================================================
//
// This build drops any internal synthesis. Aciduino is a pure sequencer /
// MIDI controller and drives an external General MIDI sound module (the
// Nulllab / SAM2695 "GM 2.0, 128 tones" board) over a hardware serial MIDI
// port. The module renders the audio to its own amplified speaker.
//
// Target hardware:
//   - ESP32 WROOM dev board (30 pin)
//   - 1.3" OLED (SH1106, 128x64, I2C)
//   - 1x rotary encoder with push button (value changer + SHIFT)
//   - 6x momentary push buttons (navigation + transport)
//   - SAM2695 GM module on a hardware UART (TTL MIDI, 31250 baud)
//
// SAM2695 module wiring (3-pin "MIDI" connector, TTL serial MIDI IN):
//   ESP32 Serial2 TX (GPIO17) --> module MIDI signal / RX
//   ESP32 GND                 --> module GND
//   power the module from 5V (its own micro-USB, or a 5V rail)
// The ESP32 TX is 3.3V which the SAM2695 UART accepts. No MIDI opto/DIN
// circuit is needed for a direct TTL connection to the module.
//
// GM tips:
//   - The SAM2695 boots to the GM map: channel 1 = piano, channel 10 = drums.
//   - Set the 808 track's output channel to 10 in the MIDI page to play it as
//     a drum kit (the note number then selects the drum sound).
//   - Set each 303 track's channel/patch from the MIDI page as desired.
//
// CONTROL SCHEME (encoder + 6 buttons)
// ------------------------------------
//   Encoder rotate ........ change the selected value
//   Encoder push (SHIFT) .. hold for the shifted function of a button
//   Button 1 ......... PAGE cycle     | SHIFT = page back
//   Button 2 ......... UP             | SHIFT = previous track
//   Button 3 ......... DOWN           | SHIFT = next track
//   Button 4 ......... PREVIOUS (<-)
//   Button 5 ......... NEXT (->)
//   Button 6 ......... PLAY / STOP    | SHIFT = REC on/off

//============================================
// Aciduino Features Setup
//============================================

#define LED_BUILTIN       2
#define USE_BPM_LED       LED_BUILTIN

// main navigation: encoder as the value changer
#define USE_CHANGER_ENCODER

#define FLIP_DISPLAY

// MIDI: Serial2 is the real MIDI out to the SAM2695 GM module.
// USB serial bridge stays available for a PC (hairless/ttymidi).
#define USE_SERIAL_MIDI_115200
#define USE_MIDI1 // USB serial-to-midi bridge (PC)
#define USE_MIDI3 // hardware serial MIDI -> SAM2695 GM module (Serial2)

//============================================
// PINOUT Setup
//============================================
//
// I2C OLED : SDA=21, SCL=22 (ESP32 hardware I2C defaults)
// MIDI out : Serial2 TX=17 -> SAM2695 MIDI in (RX=16 unused)

// rotary encoder (A/B). plugged first so they map to ENCODER_DEC/INC.
#define NAV_ENCODER_DEC_PIN       32
#define NAV_ENCODER_INC_PIN       33

// encoder push button -> SHIFT
#define NAV_SHIFT_PIN             13

// 6 momentary navigation buttons
#define NAV_BTN1_PIN              14   // PAGE
#define NAV_BTN2_PIN               4   // UP
#define NAV_BTN3_PIN               5   // DOWN
#define NAV_BTN4_PIN              18   // PREVIOUS
#define NAV_BTN5_PIN              19   // NEXT
#define NAV_BTN6_PIN              23   // PLAY/STOP

//============================================
// Managed Devices Setup
//============================================

// Display device: 1.3" SH1106 OLED over hardware I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Midi devices
#if defined(USE_MIDI1) // USB serial bridge (PC)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI1);
#endif
#if defined(USE_MIDI3) // hardware serial MIDI -> SAM2695 GM module
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
  uCtrl.din->plug(NAV_BTN1_PIN);    // PAGE_BUTTON_1
  uCtrl.din->plug(NAV_BTN2_PIN);    // PAGE_BUTTON_2
  uCtrl.din->plug(NAV_BTN3_PIN);    // GENERIC_BUTTON_1
  uCtrl.din->plug(NAV_BTN4_PIN);    // GENERIC_BUTTON_2
  uCtrl.din->plug(NAV_BTN5_PIN);    // NEXT_BUTTON
  uCtrl.din->plug(NAV_BTN6_PIN);    // UP_BUTTON (used as PLAY/STOP)

  // encoder pair (in pair order)
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
  uCtrl.midi->plug(&MIDI3); // -> SAM2695 GM module (31250 baud on Serial2)
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

  // Navigation mapping for the encoder + 6-button layout.
  // setNavComponentCtrl(SHIFT, UP, DOWN, PREVIOUS, NEXT, PAGE1, PAGE2,
  //                     GENERIC1, GENERIC2, [ENCODER_DEC, ENCODER_INC])
  uCtrl.page->setNavComponentCtrl(
      SHIFT_BUTTON,       // shift    -> encoder push
      PAGE_BUTTON_2,      // up       -> button 2
      GENERIC_BUTTON_1,   // down     -> button 3
      GENERIC_BUTTON_2,   // previous -> button 4
      NEXT_BUTTON,        // next     -> button 5
      PAGE_BUTTON_1,      // page1    -> button 1
      PAGE_BUTTON_1,      // page2    -> button 1 (SHIFT for page back)
      PAGE_BUTTON_2,      // generic1 -> button 2 (SHIFT = previous track)
      GENERIC_BUTTON_1,   // generic2 -> button 3 (SHIFT = next track)
      ENCODER_DEC, ENCODER_INC);

  // SHIFT combos: track switching on the UP / DOWN buttons
  uCtrl.page->setShiftCtrlAction(PAGE_BUTTON_2, Aciduino::previousTrack);
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_1, Aciduino::nextTrack);

  // dedicated transport button (button 6): PLAY/STOP, SHIFT = REC on/off
  uCtrl.page->setCtrlAction(UP_BUTTON, Aciduino::playStop);
  uCtrl.page->setShiftCtrlAction(UP_BUTTON, Aciduino::recToggle);

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
}
