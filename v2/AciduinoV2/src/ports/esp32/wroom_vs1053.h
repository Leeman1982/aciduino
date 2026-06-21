/*!
 *  @file       wroom_vs1053.h
 *  Project     Aciduino V2 - ESP32-WROOM driving a VS1053B GM MIDI synth
 *  @brief      Board port: SH1106 1.3" OLED + rotary encoder + 4x4 keypad (via
 *              CD74HC4067) controlling Aciduino, with a VS1053B General-MIDI
 *              module exposed as a selectable MIDI output port (its own audio
 *              jack), alongside USB serial MIDI and a 5-pin hardware MIDI DIN out.
 *
 *  See devices/vs1053_midi.h and devices/mux_keypad.h for the drivers.
 *  Pinout summary (ESP32-WROOM-32):
 *    OLED I2C        SDA=21 SCL=22
 *    VS1053 VSPI     SCK=18 MISO=19 MOSI=23  XCS=5 XDCS=33 DREQ=34 XRESET=32
 *    Encoder (DIN)   A=25 B=26  (SW=27 spare)
 *    Keypad 4067     S0=4 S1=15 S2=13 S3=12 SIG=35 (+10k pull-up on SIG)
 *    MIDI DIN out    Serial2 TX=17 (RX=16)
 *    USB serial MIDI Serial TX0=1 RX0=3
 *    BPM LED         GPIO2
 */
#include "../../uCtrl/uCtrl.h"
#include "../../aciduino.hpp"

//============================================
// Aciduino Features Setup
//============================================
#define LED_BUILTIN       2
#define USE_BPM_LED       LED_BUILTIN

// rotary encoder is the value changer (no nav pot on this build)
#define USE_CHANGER_ENCODER

#define FLIP_DISPLAY

// serial-to-midi bridge friendly (hairless / ttymidi) over USB
#define USE_SERIAL_MIDI_115200

#define USE_MIDI1   // USB MIDI (serial bridge)
#define USE_MIDI3   // hardware MIDI serial - 5-pin DIN OUT on Serial2

// drive the 4x4 keypad through a CD74HC4067 and scan it from loop()
#define USE_MUX_KEYPAD

//============================================
// PINOUT Setup
//============================================
// rotary encoder quadrature pins (uCtrl DIN ports 0 and 1)
#define NAV_ENCODER_DEC_PIN       25
#define NAV_ENCODER_INC_PIN       26
// (encoder push button SW=27 left as a spare for future use)

// VS1053B (see devices/vs1053_midi.h defaults; pinned explicitly here)
#define VS1053_PIN_SCK     18
#define VS1053_PIN_MISO    19
#define VS1053_PIN_MOSI    23
#define VS1053_PIN_XCS     5
#define VS1053_PIN_XDCS    33
#define VS1053_PIN_DREQ    34
#define VS1053_PIN_XRESET  32

// CD74HC4067 keypad mux (see devices/mux_keypad.h)
#define MUX_PIN_S0   4
#define MUX_PIN_S1   15
#define MUX_PIN_S2   13
#define MUX_PIN_S3   12
#define MUX_PIN_SIG  35

//============================================
// Managed Devices Setup
//============================================
// Display device - SH1106 1.3" 128x64 I2C OLED (default Wire pins 21/22 on WROOM)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// MIDI devices
#if defined(USE_MIDI1) // USB serial bridge
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI1);
#endif
#if defined(USE_MIDI3) // hardware MIDI DIN out
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI3);
#endif

// the main interface choice (defines pages, consts/button enum, Aciduino glue)
#include "../../interface/midilab/main.h"

// device drivers (need the interface's BUTTONS enum + Aciduino class above)
#include "../../devices/vs1053_midi.h"
#include "../../devices/mux_keypad.h"

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
  // VS1053B bring-up (real-time MIDI mode over SPI/SDI)
  //
  vs1053_begin();
  // sensible GM defaults on the VS1053 so routed tracks sound good out of the box:
  //  - 303 melodic tracks (ch 1-4) -> Synth Bass 1 (GM program 38)
  //  - 808 drum track (ch 10) -> GM drum kit (bank 0x78)
  for (uint8_t ch = 0; ch < 4; ch++) {
    vs1053_talkMIDI(0xB0 | ch, 0x00, 0x00);   // bank select MSB 0 (melodic)
    vs1053_talkMIDI(0xC0 | ch, 38, 0);        // program change -> Synth Bass 1
  }
  vs1053_talkMIDI(0xB9, 0x00, 0x78);          // ch10 bank select MSB -> drums
  vs1053_talkMIDI(0xC9, 0, 0);                // ch10 program 0 (standard kit)

  //
  // DIN Module - rotary encoder only (nav buttons come from the keypad mux)
  //
  uCtrl.initDin();
#if defined(USE_CHANGER_ENCODER)
  uCtrl.din->plug(NAV_ENCODER_DEC_PIN);   // DIN port 0 = ENCODER_DEC
  uCtrl.din->plug(NAV_ENCODER_INC_PIN);   // DIN port 1 = ENCODER_INC
  uCtrl.din->encoder(ENCODER_DEC, ENCODER_INC);
#endif

  //
  // DOUT Module
  //
  uCtrl.initDout();
#if defined(USE_BPM_LED)
  uCtrl.dout->plug(USE_BPM_LED);
#endif

  //
  // AIN Module - no analog pots on this build (encoder is the value changer)
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
  // the VS1053B as an on-board MIDI output port (its own audio jack)
  uCtrl.midi->plug(&MIDI_VS1053);

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

  // navigation: keypad-injected nav controls + encoder as secondary value changer
  uCtrl.page->setNavComponentCtrl(SHIFT_BUTTON, UP_BUTTON, DOWN_BUTTON, PREVIOUS_BUTTON, NEXT_BUTTON, PAGE_BUTTON_1, PAGE_BUTTON_2, GENERIC_BUTTON_1, GENERIC_BUTTON_2, ENCODER_DEC, ENCODER_INC);

  // shift + value buttons change the selected track
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_1, Aciduino::previousTrack);
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_2, Aciduino::nextTrack);

  // bottom bar f1/f2 labels
  uCtrl.page->setFunctionDrawCallback(functionDrawCallback);

  //
  // Keypad over CD74HC4067
  //
  muxKeypadInit();

  // init uCtrl modules and memory
  uCtrl.init();

  // all leds off
  uCtrl.dout->writeAll(LOW);

  // default page at boot
  uCtrl.page->setPage(0);

  // sequencer init
  aciduino.init();
}
