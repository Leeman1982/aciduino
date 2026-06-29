/*!
 *  @file       gm_synth.h
 *  Project     Aciduino V2 - GM Synth engine
 *  @brief      General MIDI soundfont synthesizer for the ESP32.
 *
 *  Renders a General MIDI soundfont (see soundfont_vintage_dreams.h) to 16-bit
 *  stereo audio over I2S, intended for an external PCM5102 DAC. The sequencer
 *  feeds note events through gmSynthNoteOn()/Off(); those are queued and applied
 *  by a dedicated audio task running on the second core, so they are safe to
 *  call from the sequencer/clock context on the main core.
 *
 *  MIDI channel 10 (zero-based index GM_SYNTH_DRUM_CHANNEL) is treated as the
 *  GM percussion channel (bank 128): the note number selects the drum sound.
 *
 *  @license    MIT - (c) 2024 - Aciduino contributors
 */
#ifndef __ACIDUINO_GM_SYNTH_H__
#define __ACIDUINO_GM_SYNTH_H__

#include <stdint.h>

// ---- configuration (override before including if desired) ----
#ifndef GM_SYNTH_SAMPLE_RATE
#define GM_SYNTH_SAMPLE_RATE   22050    // output rate; 22050 keeps CPU sane
#endif
#ifndef GM_SYNTH_MAX_VOICES
#define GM_SYNTH_MAX_VOICES    16       // polyphony cap
#endif
#ifndef GM_SYNTH_DRUM_CHANNEL
#define GM_SYNTH_DRUM_CHANNEL  9        // zero-based MIDI channel 10 = drums
#endif
#ifndef GM_SYNTH_I2S_BCK_PIN
#define GM_SYNTH_I2S_BCK_PIN   26
#endif
#ifndef GM_SYNTH_I2S_WS_PIN
#define GM_SYNTH_I2S_WS_PIN    25
#endif
#ifndef GM_SYNTH_I2S_DATA_PIN
#define GM_SYNTH_I2S_DATA_PIN  27   // DIN on PCM5102 (GPIO21/22 are the OLED I2C)
#endif

// Initialise the soundfont, I2S output and the audio render task.
// Returns true if the soundfont parsed and the synth started.
bool gmSynthInit();

// MIDI-style control surface (safe to call from the sequencer context).
void gmSynthNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void gmSynthNoteOff(uint8_t channel, uint8_t note);
void gmSynthProgramChange(uint8_t channel, uint8_t program);
void gmSynthControlChange(uint8_t channel, uint8_t cc, uint8_t value);
void gmSynthAllNotesOff();

#endif // __ACIDUINO_GM_SYNTH_H__
