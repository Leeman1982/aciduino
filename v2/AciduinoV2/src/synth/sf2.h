/*!
 *  @file       sf2.h
 *  Project     Aciduino V2 - GM Synth engine
 *  @brief      Minimal SoundFont 2 parser and region resolver.
 *
 *  This is a small, self contained reader for the subset of the SF2 spec
 *  needed to play a General MIDI soundfont on a microcontroller:
 *  preset -> instrument -> sample resolution with key/velocity ranges,
 *  loop points, root key, tuning, attenuation, pan and a volume envelope.
 *
 *  The soundfont bytes are expected to live in flash (.rodata). Sample PCM
 *  data is read directly from that memory map at playback time, so no large
 *  RAM copy of the wavetable is needed (important on the ESP32 WROOM, which
 *  has no PSRAM). Only the small preset/instrument/sample index tables are
 *  walked, and that happens once per note-on.
 *
 *  @license    MIT - (c) 2024 - Aciduino contributors
 */
#ifndef __ACIDUINO_SF2_H__
#define __ACIDUINO_SF2_H__

#include <stdint.h>
#include <stddef.h>

namespace sf2 {

// SF2 generator operators we care about (see SoundFont 2.04 spec, 8.1.2).
enum {
  GEN_startAddrsOffset         = 0,
  GEN_endAddrsOffset           = 1,
  GEN_startloopAddrsOffset     = 2,
  GEN_endloopAddrsOffset       = 3,
  GEN_startAddrsCoarseOffset   = 4,
  GEN_endAddrsCoarseOffset     = 12,
  GEN_pan                      = 17,
  GEN_delayVolEnv              = 33,
  GEN_attackVolEnv             = 34,
  GEN_holdVolEnv               = 35,
  GEN_decayVolEnv              = 36,
  GEN_sustainVolEnv            = 37,
  GEN_releaseVolEnv            = 38,
  GEN_instrument               = 41,
  GEN_keyRange                 = 43,
  GEN_velRange                 = 44,
  GEN_startloopAddrsCoarse     = 45,
  GEN_initialAttenuation       = 48,
  GEN_endloopAddrsCoarse       = 50,
  GEN_coarseTune               = 51,
  GEN_fineTune                 = 52,
  GEN_sampleID                 = 53,
  GEN_sampleModes              = 54,
  GEN_overridingRootKey        = 58,
};

// A fully resolved sample region ready to drive a voice. All address fields
// are absolute sample-frame indices into the 16-bit smpl data.
struct Region {
  uint32_t start;
  uint32_t end;
  uint32_t loopStart;
  uint32_t loopEnd;
  uint32_t sampleRate;
  int16_t  rootKey;        // MIDI key the sample was recorded at
  int16_t  tuneCents;      // coarseTune*100 + fineTune
  uint8_t  loopMode;       // 0 = no loop, 1 = loop, 3 = loop then release
  float    attenuation;    // linear gain multiplier from initialAttenuation (cB)
  float    pan;            // -1.0 (L) .. +1.0 (R)
  // volume envelope, seconds (already converted from timecents)
  float    attack;
  float    hold;
  float    decay;
  float    sustain;        // linear level 0..1
  float    release;
};

class SF2 {
  public:
    // Parse a soundfont image held in flash/RAM. Returns true on success.
    bool load(const uint8_t* data, uint32_t size);

    // Resolve the regions that should sound for a given program/bank/key/vel.
    // Fills up to maxRegions and returns how many were written. Multiple
    // regions implement SF2 layering. bank 128 selects percussion.
    uint8_t findRegions(uint8_t bank, uint8_t program, uint8_t key,
                        uint8_t velocity, Region* out, uint8_t maxRegions);

    // Base pointer to the 16-bit PCM sample pool (the smpl chunk).
    const int16_t* samples() const { return _smpl; }
    uint32_t sampleCount() const { return _smplCount; }
    bool valid() const { return _valid; }

  private:
    const uint8_t* _data = nullptr;
    uint32_t _size = 0;
    bool _valid = false;

    // sample pool
    const int16_t* _smpl = nullptr;
    uint32_t _smplCount = 0;

    // pdta sub-chunk pointers and counts (records, not bytes)
    const uint8_t* _phdr = nullptr; uint32_t _phdrCount = 0;
    const uint8_t* _pbag = nullptr; uint32_t _pbagCount = 0;
    const uint8_t* _pgen = nullptr; uint32_t _pgenCount = 0;
    const uint8_t* _inst = nullptr; uint32_t _instCount = 0;
    const uint8_t* _ibag = nullptr; uint32_t _ibagCount = 0;
    const uint8_t* _igen = nullptr; uint32_t _igenCount = 0;
    const uint8_t* _shdr = nullptr; uint32_t _shdrCount = 0;

    // Resolve one instrument zone matching key/vel into a Region, applying the
    // preset-level generator offsets passed in. Returns false if no match.
    bool resolveInstrument(uint16_t instIndex, uint8_t key, uint8_t velocity,
                          const int16_t* presetGen, Region* out);
};

} // namespace sf2

#endif // __ACIDUINO_SF2_H__
