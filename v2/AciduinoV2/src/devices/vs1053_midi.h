/*!
 *  @file       vs1053_midi.h
 *  Project     Aciduino V2 - VS1053B GM MIDI output device
 *  @brief      Drives a VS1053B (cheap "red" MP3 shield clone) as a General MIDI
 *              synth over SPI/SDI, exposed to Aciduino/uCtrl as a standard MIDI
 *              port (FortySevenEffects MidiInterface custom Transport).
 *
 *  The chip is brought up in real-time MIDI mode by loading the small
 *  "VS1053b Realtime MIDI Start" patch over SCI (the hardware GPIO strap is
 *  unavailable on clone boards). Each MIDI byte is then streamed over the data
 *  interface (XDCS) padded with a leading 0x00.
 *
 *  Wire up (defaults below, override before including from the port file):
 *      SCK=18  MISO=19  MOSI=23  (VSPI)
 *      XCS=5  XDCS=33  DREQ=34(in)  XRESET=32
 *  VS1053 audio comes out of the board's own 3.5mm jack - no MCU audio needed.
 *
 *  @license    MIT
 */
#ifndef __ACIDUINO_VS1053_MIDI_H__
#define __ACIDUINO_VS1053_MIDI_H__

#include <Arduino.h>
#include <SPI.h>
// FortySevenEffects MIDI library (bundled inside uCtrl). Provides
// midi::MidiInterface<> and midi::MidiType used by the custom transport below.
#include "../uCtrl/module/midi/MIDI/MIDI.h"

//============================================
// Pinout (overridable from the port header)
//============================================
#ifndef VS1053_PIN_SCK
#define VS1053_PIN_SCK     18
#endif
#ifndef VS1053_PIN_MISO
#define VS1053_PIN_MISO    19
#endif
#ifndef VS1053_PIN_MOSI
#define VS1053_PIN_MOSI    23
#endif
#ifndef VS1053_PIN_XCS
#define VS1053_PIN_XCS     5    // SCI (control) chip select
#endif
#ifndef VS1053_PIN_XDCS
#define VS1053_PIN_XDCS    33   // SDI (data) chip select
#endif
#ifndef VS1053_PIN_DREQ
#define VS1053_PIN_DREQ    34   // data request (MCU input, input-only OK)
#endif
#ifndef VS1053_PIN_XRESET
#define VS1053_PIN_XRESET  32   // active-low reset
#endif

//============================================
// SCI registers / opcodes
//============================================
#define VS1053_OP_WRITE      0x02
#define VS1053_OP_READ       0x03
#define VS1053_SCI_MODE      0x00
#define VS1053_SCI_STATUS    0x01
#define VS1053_SCI_BASS      0x02
#define VS1053_SCI_CLOCKF    0x03
#define VS1053_SCI_AUDATA    0x05
#define VS1053_SCI_WRAM      0x06
#define VS1053_SCI_WRAMADDR  0x07
#define VS1053_SCI_AIADDR    0x0A
#define VS1053_SCI_VOL       0x0B

// SPI speeds: keep SCI slow until SCI_CLOCKF is set (max CLKI/7 ~1.75MHz at boot),
// stream SDI MIDI fast afterwards.
#define VS1053_SCI_SPI_HZ    1000000UL
#define VS1053_SDI_SPI_HZ    8000000UL

//============================================
// Real-time MIDI start plugin (28 words, self-starts via AIADDR write)
// Source: VLSI vs1053b-rtmidistart.zip (reproduced with permission).
//============================================
static const uint16_t vs1053_rtMidiStart[28] = {
  0x0007, 0x0001, 0x8050, 0x0006, 0x0014, 0x0030, 0x0715, 0xb080,
  0x3400, 0x0007, 0x9255, 0x3d00, 0x0024, 0x0030, 0x0295, 0x6890,
  0x3400, 0x0030, 0x0495, 0x3d00, 0x0024, 0x2908, 0x4d40, 0x0030,
  0x0200, 0x000a, 0x0001, 0x0050
};

//============================================
// Low level SCI / SDI helpers
//============================================
static inline void vs1053_waitDreq() {
  // bounded wait so a stuck chip never hard-locks the sequencer
  uint32_t guard = 0;
  while (!digitalRead(VS1053_PIN_DREQ)) {
    if (++guard > 1000000UL) break;
  }
}

static inline void vs1053_sciWrite(uint8_t reg, uint16_t val) {
  vs1053_waitDreq();
  SPI.beginTransaction(SPISettings(VS1053_SCI_SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(VS1053_PIN_XCS, LOW);
  SPI.transfer(VS1053_OP_WRITE);
  SPI.transfer(reg);
  SPI.transfer((uint8_t)(val >> 8));
  SPI.transfer((uint8_t)(val & 0xFF));
  vs1053_waitDreq();
  digitalWrite(VS1053_PIN_XCS, HIGH);
  SPI.endTransaction();
}

static inline uint16_t vs1053_sciRead(uint8_t reg) {
  vs1053_waitDreq();
  SPI.beginTransaction(SPISettings(VS1053_SCI_SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(VS1053_PIN_XCS, LOW);
  SPI.transfer(VS1053_OP_READ);
  SPI.transfer(reg);
  uint16_t v = (uint16_t)SPI.transfer(0x00) << 8;
  v |= SPI.transfer(0x00);
  digitalWrite(VS1053_PIN_XCS, HIGH);
  SPI.endTransaction();
  return v;
}

// Walk the compressed plugin: (addr, n) records; high bit of n == RLE run.
static inline void vs1053_applyPatch(const uint16_t *p, uint16_t len) {
  uint16_t i = 0;
  while (i < len) {
    uint16_t addr = p[i++];
    uint16_t n    = p[i++];
    if (n & 0x8000) {                 // RLE run
      n &= 0x7FFF;
      uint16_t val = p[i++];
      while (n--) vs1053_sciWrite(addr, val);
    } else {                          // copy run
      while (n--) vs1053_sciWrite(addr, p[i++]);
    }
  }
}

// Send a raw MIDI message straight over SDI (used for boot GM setup / panic).
static inline void vs1053_talkMIDI(uint8_t cmd, uint8_t d1, uint8_t d2) {
  vs1053_waitDreq();
  SPI.beginTransaction(SPISettings(VS1053_SDI_SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(VS1053_PIN_XDCS, LOW);
  SPI.transfer((uint8_t)0x00); SPI.transfer(cmd);
  SPI.transfer((uint8_t)0x00); SPI.transfer(d1);
  uint8_t status = cmd & 0xF0;
  if (status != 0xC0 && status != 0xD0) {   // 0xC0/0xD0 are 1-data-byte messages
    SPI.transfer((uint8_t)0x00); SPI.transfer(d2);
  }
  digitalWrite(VS1053_PIN_XDCS, HIGH);
  SPI.endTransaction();
}

// All-notes-off on every channel + reset controllers (panic).
static inline void vs1053_panic() {
  for (uint8_t ch = 0; ch < 16; ch++) {
    vs1053_talkMIDI(0xB0 | ch, 123, 0);   // CC123 all notes off
    vs1053_talkMIDI(0xB0 | ch, 121, 0);   // CC121 reset all controllers
  }
}

//============================================
// Bring-up: reset -> clock -> volume -> load rt-midi patch
// Returns true if real-time MIDI mode is confirmed live (AUDATA == 0xAC45).
//============================================
static inline bool vs1053_begin() {
  pinMode(VS1053_PIN_XCS, OUTPUT);
  pinMode(VS1053_PIN_XDCS, OUTPUT);
  pinMode(VS1053_PIN_DREQ, INPUT);
  pinMode(VS1053_PIN_XRESET, OUTPUT);
  digitalWrite(VS1053_PIN_XCS, HIGH);
  digitalWrite(VS1053_PIN_XDCS, HIGH);

  SPI.begin(VS1053_PIN_SCK, VS1053_PIN_MISO, VS1053_PIN_MOSI);

  // 1. hardware reset
  digitalWrite(VS1053_PIN_XRESET, LOW);
  delay(10);
  digitalWrite(VS1053_PIN_XRESET, HIGH);
  vs1053_waitDreq();
  delay(5);

  // 2. clock multiplier 3.0x (reverb-capable, plenty for GM MIDI) BEFORE the patch
  vs1053_sciWrite(VS1053_SCI_CLOCKF, 0x6000);
  delay(2);

  // 3. moderate master volume + clean (flat) output
  vs1053_sciWrite(VS1053_SCI_VOL, 0x2020);
  vs1053_sciWrite(VS1053_SCI_BASS, 0x0000);

  // 4. load real-time MIDI start patch (self-starts via AIADDR write)
  vs1053_applyPatch(vs1053_rtMidiStart, 28);
  delay(10);

  // 5. confirm: AUDATA reads 0xAC45 (44100Hz stereo) once RT MIDI is running
  return (vs1053_sciRead(VS1053_SCI_AUDATA) == 0xAC45);
}

//============================================
// uCtrl/FortySevenEffects MIDI Transport
//
// Streaming a MIDI message = XDCS low, write each byte padded with 0x00, XDCS high.
// Plugging the resulting MidiInterface into uCtrl makes the VS1053 appear as a
// selectable output "port" - no changes to the Aciduino sequencer core.
//============================================
class VS1053Transport {
public:
  static const bool thruActivated = true;

  void begin() { /* chip already initialised by vs1053_begin() */ }

  bool beginTransmission(midi::MidiType) {
    vs1053_waitDreq();
    SPI.beginTransaction(SPISettings(VS1053_SDI_SPI_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(VS1053_PIN_XDCS, LOW);
    return true;
  }

  void write(byte value) {
    SPI.transfer((uint8_t)0x00);   // VS1053 SDI pad byte (datasheet's 0xFF is an error)
    SPI.transfer(value);
  }

  void endTransmission() {
    digitalWrite(VS1053_PIN_XDCS, HIGH);
    SPI.endTransaction();
  }

  byte read() { return 0; }
  unsigned available() { return 0; }
};

// Instance + MidiInterface wrapper (mirrors the MIDI_CREATE_INSTANCE macro pattern).
static VS1053Transport vs1053Transport;
static midi::MidiInterface<VS1053Transport> MIDI_VS1053((VS1053Transport&)vs1053Transport);

#endif // __ACIDUINO_VS1053_MIDI_H__
