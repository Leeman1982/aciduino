/*!
 *  @file       synth_config.h
 *  Project     Aciduino V2 - GM Synth engine
 *  @brief      Global build switch for the on-board General MIDI synth.
 *
 *  The Arduino build compiles every .cpp in the sketch tree as its own
 *  translation unit, so a per-port #define in a ports/*.h header is NOT
 *  visible to core files like aciduino.cpp. This header is included by every
 *  translation unit that needs to know whether the internal synth is active,
 *  giving a single, build-wide switch.
 *
 *  Comment out USE_GM_SYNTH to build a port WITHOUT the internal synth
 *  (e.g. the AVR / Teensy / plain ESP32 MIDI-only ports). When enabled the
 *  synth only actually runs on ESP32 and only after the selected port calls
 *  gmSynthInit() from initPort().
 */
#ifndef __ACIDUINO_SYNTH_CONFIG_H__
#define __ACIDUINO_SYNTH_CONFIG_H__

// On-board General MIDI soundfont synthesizer (ESP32 + PCM5102 I2S DAC).
#define USE_GM_SYNTH

#endif // __ACIDUINO_SYNTH_CONFIG_H__
