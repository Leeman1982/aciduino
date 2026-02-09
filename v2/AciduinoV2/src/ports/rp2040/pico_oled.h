#include "../../uCtrl/uCtrl.h"

#include "../../aciduino.hpp"

// RP2040 Raspberry Pi Pico - OLED Version
// Dual-core sequencer with MIDI RX/TX and Audio output

//============================================
// Aciduino Features Setup
//============================================

// BPM LED on built-in LED
#define LED_BUILTIN       25
#define USE_BPM_LED       LED_BUILTIN

// Main navigation - use potentiometer or encoder
#define USE_CHANGER_POT
//#define USE_CHANGER_ENCODER

//#define USE_TRANSPORT_BUTTON

// Optional: 4 additional pots for MIDI CC mapping
//#define USE_POT_MICRO

// Display orientation
#define FLIP_DISPLAY
//#define INVERT_POT_READ

// MIDI Configuration
#define USE_MIDI1 // USB MIDI (native USB on RP2040)
#define USE_MIDI2 // Hardware UART MIDI RX/TX (Serial1)
//#define USE_MIDI3 // Optional second hardware UART (Serial2)

// RP2040 Dual-Core Support
#define USE_DUAL_CORE_RP2040

// Audio output via Arduino Audio Tools (optional)
//#define USE_AUDIO_TOOLS
//#define USE_AUDIO_I2S  // I2S DAC output
//#define USE_AUDIO_PWM  // PWM audio output

// SPI modules (optional extensions)
//#define USE_PUSH_8      // uses 165 shiftregister (buttons)
//#define USE_PUSH_24     // uses 3x 165 shiftregister
//#define USE_PUSH_32     // uses 4x 165 shiftregister
//#define USE_LED_8       // uses 595 shiftregister
//#define USE_LED_24      // uses 3x 595 shiftregister
//#define USE_POT_8       // uses 4051 multiplexer
//#define USE_POT_16      // uses 2x 4051 multiplexer

//============================================
// PINOUT Setup - Raspberry Pi Pico
//============================================

// I2C for OLED Display (default I2C0)
// SDA: GPIO 4
// SCL: GPIO 5

// MIDI UART (Serial1 - UART0)
#define MIDI_SERIAL_RX_PIN        1  // UART0 RX
#define MIDI_SERIAL_TX_PIN        0  // UART0 TX

// Optional second MIDI port (Serial2 - UART1)
//#define MIDI2_SERIAL_RX_PIN       9  // UART1 RX
//#define MIDI2_SERIAL_TX_PIN       8  // UART1 TX

// Navigation Encoder (optional - comment out if using pot)
//#define NAV_ENCODER_DEC_PIN       20
//#define NAV_ENCODER_INC_PIN       21

// Navigation Buttons
#define NAV_SHIFT_PIN             22
#define NAV_FUNCTION1_PIN         18
#define NAV_FUNCTION2_PIN         19
#define NAV_GENERAL1_PIN          16  // Decrement
#define NAV_GENERAL2_PIN          17  // Increment
#define NAV_RIGHT_PIN             15
#define NAV_UP_PIN                14
#define NAV_DOWN_PIN              13
#define NAV_LEFT_PIN              12

// Transport button (play/stop)
//#define TRANSPORT_BUTTON_1_PIN    11

// Changer Potentiometer (ADC0)
#define CHANGER_POT_PIN           26  // GPIO 26 - ADC0

// Optional: 4 additional pots for MIDI CC mapping
//#define POT_MICRO_1_PIN           27  // GPIO 27 - ADC1
//#define POT_MICRO_2_PIN           28  // GPIO 28 - ADC2
//#define POT_MICRO_3_PIN           29  // GPIO 29 - ADC3

// I2S Audio Output (optional)
#ifdef USE_AUDIO_I2S
  #define I2S_BCLK_PIN            6   // Bit Clock
  #define I2S_LRCLK_PIN           7   // Left/Right Clock
  #define I2S_DOUT_PIN            8   // Data Out
#endif

// PWM Audio Output (optional)
#ifdef USE_AUDIO_PWM
  #define PWM_AUDIO_PIN           9   // PWM output
#endif

//============================================
// Managed Devices Setup
//============================================

// Display device - SH1106 128x64 OLED
// Compatible with both 1.3" and 2.4" OLED displays
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// MIDI device configuration
#if defined(USE_MIDI1) // USB MIDI
#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI1);
#endif

#if defined(USE_MIDI2) // Hardware UART MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI2);
#endif

#if defined(USE_MIDI3) // Optional second hardware UART
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI3);
#endif

// Audio Tools setup (optional)
#ifdef USE_AUDIO_TOOLS
#include "AudioTools.h"

#ifdef USE_AUDIO_I2S
  I2SStream i2s;
  VolumeStream volume(i2s);
  SineWaveGenerator<int16_t> sineWave;
  GeneratedSoundStream<int16_t> sound(sineWave);
  StreamCopy copier(volume, sound);
#endif

#ifdef USE_AUDIO_PWM
  PWMAudioStream pwm;
  VolumeStream volume(pwm);
  SineWaveGenerator<int16_t> sineWave;
  GeneratedSoundStream<int16_t> sound(sineWave);
  StreamCopy copier(volume, sound);
#endif
#endif

// SPI devices
//#define PUSH_SPI          SPI
//#define LED_SPI           SPI

// The main interface
#include "../../interface/midilab/main.h"

//============================================
// Dual-Core RP2040 Support
//============================================

#ifdef USE_DUAL_CORE_RP2040

// Core 1 runs the sequencer and timing-critical tasks
void setup1() {
  // Core 1 initialization happens here
  // This core will handle sequencer timing
}

void loop1() {
  // Core 1 main loop - sequencer processing
  // This is called from main .ino after initPort()
  aciduino.run();
}

#endif

//============================================
// Port Initialization
//============================================

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
  // DIN Module (Digital Input)
  //
#if defined(USE_PUSH_8) || defined(USE_PUSH_24) || defined(USE_PUSH_32)
  // SPI shift register setup
  uCtrl.initDin(&PUSH_SPI, PUSH_LATCH_PIN);
#else
  uCtrl.initDin();
#endif

#if defined(USE_CHANGER_ENCODER)
  // Encoder pins
  uCtrl.din->plug(NAV_ENCODER_DEC_PIN);
  uCtrl.din->plug(NAV_ENCODER_INC_PIN);
#endif

  // Shift button
  uCtrl.din->plug(NAV_SHIFT_PIN);

  // Main navigation buttons
#if !defined(USE_PUSH_32) && !defined(USE_TOUCH_32)
  uCtrl.din->plug(NAV_FUNCTION1_PIN);
  uCtrl.din->plug(NAV_FUNCTION2_PIN);
  uCtrl.din->plug(NAV_GENERAL1_PIN);
  uCtrl.din->plug(NAV_GENERAL2_PIN);
  uCtrl.din->plug(NAV_RIGHT_PIN);
  uCtrl.din->plug(NAV_UP_PIN);
  uCtrl.din->plug(NAV_DOWN_PIN);
  uCtrl.din->plug(NAV_LEFT_PIN);
#if defined(USE_TRANSPORT_BUTTON)
  uCtrl.din->plug(TRANSPORT_BUTTON_1_PIN);
#endif
#endif

// Shift register configuration
#if defined(USE_PUSH_8)
  uCtrl.din->plugSR(1);
#elif defined(USE_PUSH_24)
  uCtrl.din->plugSR(3);
#elif defined(USE_PUSH_32)
  uCtrl.din->plugSR(4);
#endif

#if defined(USE_CHANGER_ENCODER)
  // Encoder setup (in pairs)
  uCtrl.din->encoder(ENCODER_DEC, ENCODER_INC);
#endif

  //
  // DOUT Module (Digital Output)
  //
#if defined(USE_LED_8) || defined(USE_LED_24)
  uCtrl.initDout(&LED_SPI, LED_LATCH_PIN);
#else
  uCtrl.initDout();
#endif

#if defined(USE_BPM_LED)
  // Built-in LED for BPM indicator
  uCtrl.dout->plug(USE_BPM_LED);
#endif

#if defined(USE_LED_8)
  uCtrl.dout->plugSR(1);
#elif defined(USE_LED_24)
  uCtrl.dout->plugSR(3);
#endif

  //
  // AIN Module (Analog Input)
  //
#if defined(USE_POT_8) || defined(USE_POT_16)
  uCtrl.initAin(POT_CTRL_PIN1, POT_CTRL_PIN2, POT_CTRL_PIN3);

#if defined(USE_CHANGER_POT)
  uCtrl.ain->plug(CHANGER_POT_PIN);
#endif

  uCtrl.ain->plugMux(POT_MUX_COMM1);

#if defined(USE_POT_16)
  uCtrl.ain->plugMux(POT_MUX_COMM2);
#endif

  // MIDI controller callback
  uCtrl.ain->setCallback(midiControllerHandle);

#else
  // No multiplexer - direct pot connection
  uCtrl.initAin();

#if defined(USE_CHANGER_POT)
  uCtrl.ain->plug(CHANGER_POT_PIN);
#endif

#if defined(USE_POT_MICRO)

#if defined(POT_MICRO_1_PIN)
  uCtrl.ain->plug(POT_MICRO_1_PIN);
#endif
#if defined(POT_MICRO_2_PIN)
  uCtrl.ain->plug(POT_MICRO_2_PIN);
#endif
#if defined(POT_MICRO_3_PIN)
  uCtrl.ain->plug(POT_MICRO_3_PIN);
#endif

  // MIDI controller callback
  uCtrl.ain->setCallback(midiControllerHandle);
#endif

#if defined(INVERT_POT_READ)
  uCtrl.ain->invertRead(true);
#endif

  // Better stability with more averaged reads
  uCtrl.ain->setAvgReads(8);

#endif

  //
  // MIDI Module
  //
  uCtrl.initMidi();

  // RP2040 TinyUSB setup
#if defined(USE_MIDI1)
  // Initialize USB MIDI
  usb_midi.setStringDescriptor("Aciduino RP2040");
  #if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
    // For Arduino Mbed OS RP2040 core
    TinyUSBDevice.setID(0x2E8A, 0x000A); // Raspberry Pi VID/PID
  #endif
  uCtrl.midi->plug(&MIDI1);
#endif

  // Hardware UART MIDI
#if defined(USE_MIDI2)
  Serial1.setRX(MIDI_SERIAL_RX_PIN);
  Serial1.setTX(MIDI_SERIAL_TX_PIN);
  uCtrl.midi->plug(&MIDI2);
#endif

#if defined(USE_MIDI3)
  Serial2.setRX(MIDI2_SERIAL_RX_PIN);
  Serial2.setTX(MIDI2_SERIAL_TX_PIN);
  uCtrl.midi->plug(&MIDI3);
#endif

  // Set MIDI input callback for RX handling
  uCtrl.midi->setMidiInputCallback(Aciduino::midiInputHandler);

  // Timing callbacks
  uCtrl.setOn250usCallback(Aciduino::midiHandleSync);
  uCtrl.setOn1msCallback(Aciduino::midiHandle);

  //
  // Audio Tools Setup (optional)
  //
#ifdef USE_AUDIO_TOOLS

#ifdef USE_AUDIO_I2S
  // I2S configuration
  auto i2s_config = i2s.defaultConfig(TX_MODE);
  i2s_config.pin_bck = I2S_BCLK_PIN;
  i2s_config.pin_ws = I2S_LRCLK_PIN;
  i2s_config.pin_data = I2S_DOUT_PIN;
  i2s_config.sample_rate = 44100;
  i2s_config.channels = 2;
  i2s_config.bits_per_sample = 16;
  i2s.begin(i2s_config);

  // Setup sine wave generator (example)
  sineWave.begin(i2s_config, N_B4); // B note
  volume.begin(i2s_config);
  volume.setVolume(0.5);
#endif

#ifdef USE_AUDIO_PWM
  // PWM audio configuration
  auto pwm_config = pwm.defaultConfig();
  pwm_config.pin = PWM_AUDIO_PIN;
  pwm_config.sample_rate = 32000;
  pwm_config.channels = 1;
  pwm_config.bits_per_sample = 16;
  pwm.begin(pwm_config);

  // Setup sine wave generator (example)
  sineWave.begin(pwm_config, N_B4);
  volume.begin(pwm_config);
  volume.setVolume(0.5);
#endif

#endif // USE_AUDIO_TOOLS

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

  // Component UI navigation control
#if defined(USE_CHANGER_ENCODER)
  uCtrl.page->setNavComponentCtrl(SHIFT_BUTTON, UP_BUTTON, DOWN_BUTTON, PREVIOUS_BUTTON, NEXT_BUTTON, PAGE_BUTTON_1, PAGE_BUTTON_2, GENERIC_BUTTON_1, GENERIC_BUTTON_2, ENCODER_DEC, ENCODER_INC);
#else
  uCtrl.page->setNavComponentCtrl(SHIFT_BUTTON, UP_BUTTON, DOWN_BUTTON, PREVIOUS_BUTTON, NEXT_BUTTON, PAGE_BUTTON_1, PAGE_BUTTON_2, GENERIC_BUTTON_1, GENERIC_BUTTON_2);
#endif

#if defined(USE_CHANGER_POT)
  uCtrl.page->setNavPot(0); // first registered ain pin
#endif

  // Hook button callbacks
  // Previous/Next track
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_1, Aciduino::previousTrack);
  uCtrl.page->setShiftCtrlAction(GENERIC_BUTTON_2, Aciduino::nextTrack);

  // Transport play/stop and record
#if defined(USE_TRANSPORT_BUTTON)
  uCtrl.page->setCtrlAction(TRANSPORT_BUTTON_1, Aciduino::playStop);
  uCtrl.page->setShiftCtrlAction(TRANSPORT_BUTTON_1, Aciduino::recToggle);
#elif defined(USE_PUSH_8) || defined(USE_PUSH_16) || defined(USE_PUSH_32)
  uCtrl.page->setCtrlAction(SELECTOR_BUTTON_8, Aciduino::playStop);
  uCtrl.page->setShiftCtrlAction(SELECTOR_BUTTON_8, Aciduino::recToggle);
#endif

  // Bottom bar function draw callback
  uCtrl.page->setFunctionDrawCallback(functionDrawCallback);

  // Initialize uCtrl modules and memory
  uCtrl.init();

  // Turn off all LEDs
  uCtrl.dout->writeAll(LOW);

  // Set default page
  uCtrl.page->setPage(0);

  // Initialize aciduino sequencer
  aciduino.init();
}
