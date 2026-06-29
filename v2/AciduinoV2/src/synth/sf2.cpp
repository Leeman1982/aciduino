#include "sf2.h"
#include <math.h>
#include <string.h>

namespace sf2 {

// SF2 record sizes (bytes), per the spec.
static const uint8_t PHDR_SZ = 38;
static const uint8_t PBAG_SZ = 4;
static const uint8_t PGEN_SZ = 4;
static const uint8_t INST_SZ = 22;
static const uint8_t IBAG_SZ = 4;
static const uint8_t IGEN_SZ = 4;
static const uint8_t SHDR_SZ = 46;

// --- little endian readers (byte assembled => safe on unaligned flash) ---
static inline uint16_t rd16(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline int16_t rds16(const uint8_t* p) {
  return (int16_t)rd16(p);
}
static inline uint32_t rd32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline bool fourcc(const uint8_t* p, const char* id) {
  return p[0] == id[0] && p[1] == id[1] && p[2] == id[2] && p[3] == id[3];
}

// timecents -> seconds
static inline float tc2sec(int32_t tc) {
  return powf(2.0f, (float)tc / 1200.0f);
}
// centibels of attenuation -> linear amplitude (1.0 = no attenuation)
static inline float cb2gain(float cb) {
  if (cb <= 0) return 1.0f;
  return powf(10.0f, -cb / 200.0f);
}

bool SF2::load(const uint8_t* data, uint32_t size) {
  _valid = false;
  _data = data;
  _size = size;
  if (size < 12 || !fourcc(data, "RIFF") || !fourcc(data + 8, "sfbk"))
    return false;

  // Walk the top level LIST chunks: INFO, sdta, pdta.
  uint32_t pos = 12;
  while (pos + 8 <= size) {
    const uint8_t* chunk = data + pos;
    uint32_t csize = rd32(chunk + 4);
    if (fourcc(chunk, "LIST")) {
      const uint8_t* body = chunk + 12;          // after LIST <size> <type>
      uint32_t bodyEnd = pos + 8 + csize;
      if (fourcc(chunk + 8, "sdta")) {
        // sub-chunks: smpl (16 bit), sm24 (low byte, ignored)
        uint32_t sp = pos + 12;
        while (sp + 8 <= bodyEnd) {
          const uint8_t* sc = data + sp;
          uint32_t ssize = rd32(sc + 4);
          if (fourcc(sc, "smpl")) {
            _smpl = (const int16_t*)(sc + 8);
            _smplCount = ssize / 2;
          }
          sp += 8 + ssize + (ssize & 1);
        }
      } else if (fourcc(chunk + 8, "pdta")) {
        uint32_t sp = pos + 12;
        while (sp + 8 <= bodyEnd) {
          const uint8_t* sc = data + sp;
          uint32_t ssize = rd32(sc + 4);
          const uint8_t* rec = sc + 8;
          if      (fourcc(sc, "phdr")) { _phdr = rec; _phdrCount = ssize / PHDR_SZ; }
          else if (fourcc(sc, "pbag")) { _pbag = rec; _pbagCount = ssize / PBAG_SZ; }
          else if (fourcc(sc, "pgen")) { _pgen = rec; _pgenCount = ssize / PGEN_SZ; }
          else if (fourcc(sc, "inst")) { _inst = rec; _instCount = ssize / INST_SZ; }
          else if (fourcc(sc, "ibag")) { _ibag = rec; _ibagCount = ssize / IBAG_SZ; }
          else if (fourcc(sc, "igen")) { _igen = rec; _igenCount = ssize / IGEN_SZ; }
          else if (fourcc(sc, "shdr")) { _shdr = rec; _shdrCount = ssize / SHDR_SZ; }
          sp += 8 + ssize + (ssize & 1);
        }
      }
      (void)body;
    }
    pos += 8 + csize + (csize & 1);
  }

  _valid = _smpl && _phdr && _pbag && _pgen && _inst && _ibag && _igen && _shdr;
  return _valid;
}

// Build a generator value table for a bag's zone [genStart, genEnd).
// Out arrays must be sized 64. 'has' marks which generators were present.
// Returns the terminal generator value (instrument id or sample id) via
// outTerminal if the terminal generator (terminalOp) is present.
static void readZoneGens(const uint8_t* gen, uint16_t genStart, uint16_t genEnd,
                        int32_t* vals, bool* has,
                        uint8_t* keyLo, uint8_t* keyHi,
                        uint8_t* velLo, uint8_t* velHi) {
  for (uint16_t g = genStart; g < genEnd; g++) {
    const uint8_t* rec = gen + (uint32_t)g * 4;
    uint16_t op = rd16(rec);
    if (op >= 64) continue;
    if (op == GEN_keyRange) {
      *keyLo = rec[2]; *keyHi = rec[3];
      has[op] = true;
    } else if (op == GEN_velRange) {
      *velLo = rec[2]; *velHi = rec[3];
      has[op] = true;
    } else {
      vals[op] = rds16(rec + 2);
      has[op] = true;
    }
  }
}

bool SF2::resolveInstrument(uint16_t instIndex, uint8_t key, uint8_t velocity,
                           const int16_t* presetAdd, Region* out) {
  if ((uint32_t)instIndex + 1 >= _instCount) return false;

  uint16_t bagStart = rd16(_inst + (uint32_t)instIndex * INST_SZ + 20);
  uint16_t bagEnd   = rd16(_inst + (uint32_t)(instIndex + 1) * INST_SZ + 20);

  // global zone defaults accumulate here
  int32_t gVals[64]; bool gHas[64];
  memset(gVals, 0, sizeof(gVals));
  memset(gHas, 0, sizeof(gHas));
  uint8_t gKeyLo = 0, gKeyHi = 127, gVelLo = 0, gVelHi = 127;

  for (uint16_t b = bagStart; b < bagEnd; b++) {
    uint16_t genStart = rd16(_ibag + (uint32_t)b * IBAG_SZ);
    uint16_t genEnd   = rd16(_ibag + (uint32_t)(b + 1) * IBAG_SZ);

    int32_t vals[64]; bool has[64];
    memcpy(vals, gVals, sizeof(vals));
    memcpy(has, gHas, sizeof(has));
    uint8_t keyLo = gKeyLo, keyHi = gKeyHi, velLo = gVelLo, velHi = gVelHi;
    readZoneGens(_igen, genStart, genEnd, vals, has, &keyLo, &keyHi, &velLo, &velHi);

    bool hasSample = has[GEN_sampleID];
    if (!hasSample) {
      // global instrument zone: carry forward as defaults
      memcpy(gVals, vals, sizeof(gVals));
      memcpy(gHas, has, sizeof(gHas));
      gKeyLo = keyLo; gKeyHi = keyHi; gVelLo = velLo; gVelHi = velHi;
      continue;
    }

    if (key < keyLo || key > keyHi) continue;
    if (velocity < velLo || velocity > velHi) continue;

    uint16_t sampleID = (uint16_t)vals[GEN_sampleID];
    if (sampleID >= _shdrCount) continue;

    const uint8_t* sh = _shdr + (uint32_t)sampleID * SHDR_SZ;
    uint32_t start     = rd32(sh + 20);
    uint32_t end       = rd32(sh + 24);
    uint32_t loopStart = rd32(sh + 28);
    uint32_t loopEnd   = rd32(sh + 32);
    uint32_t rate      = rd32(sh + 36);
    uint8_t  origPitch = sh[40];
    int8_t   pitchCorr = (int8_t)sh[41];

    // sample address fine/coarse offsets (generators)
    start     += vals[GEN_startAddrsOffset]     + 32768 * vals[GEN_startAddrsCoarseOffset];
    end       += vals[GEN_endAddrsOffset]       + 32768 * vals[GEN_endAddrsCoarseOffset];
    loopStart += vals[GEN_startloopAddrsOffset] + 32768 * vals[GEN_startloopAddrsCoarse];
    loopEnd   += vals[GEN_endloopAddrsOffset]   + 32768 * vals[GEN_endloopAddrsCoarse];

    out->start      = start;
    out->end        = end;
    out->loopStart  = loopStart;
    out->loopEnd    = loopEnd;
    out->sampleRate = rate ? rate : 22050;

    int16_t root = has[GEN_overridingRootKey] && vals[GEN_overridingRootKey] >= 0
                     ? (int16_t)vals[GEN_overridingRootKey] : (int16_t)origPitch;
    out->rootKey = root;

    int32_t coarse = vals[GEN_coarseTune] + presetAdd[GEN_coarseTune];
    int32_t fine   = vals[GEN_fineTune]   + presetAdd[GEN_fineTune];
    out->tuneCents = coarse * 100 + fine + pitchCorr;

    float atten = (float)(vals[GEN_initialAttenuation] + presetAdd[GEN_initialAttenuation]);
    out->attenuation = cb2gain(atten);

    int32_t pan = vals[GEN_pan] + presetAdd[GEN_pan];   // 0.1% units
    if (pan < -500) pan = -500;
    if (pan > 500) pan = 500;
    out->pan = (float)pan / 500.0f;

    out->loopMode = (uint8_t)(vals[GEN_sampleModes] & 0x03);

    // volume envelope (timecents -> seconds). preset gens add to instrument.
    int32_t atk = has[GEN_attackVolEnv]  ? vals[GEN_attackVolEnv]  : -12000;
    int32_t hld = has[GEN_holdVolEnv]    ? vals[GEN_holdVolEnv]    : -12000;
    int32_t dec = has[GEN_decayVolEnv]   ? vals[GEN_decayVolEnv]   : -12000;
    int32_t rel = has[GEN_releaseVolEnv] ? vals[GEN_releaseVolEnv] : -12000;
    atk += presetAdd[GEN_attackVolEnv];
    hld += presetAdd[GEN_holdVolEnv];
    dec += presetAdd[GEN_decayVolEnv];
    rel += presetAdd[GEN_releaseVolEnv];
    out->attack  = tc2sec(atk);
    out->hold    = tc2sec(hld);
    out->decay   = tc2sec(dec);
    out->release = tc2sec(rel);

    int32_t sus = vals[GEN_sustainVolEnv] + presetAdd[GEN_sustainVolEnv]; // cB
    if (sus < 0) sus = 0;
    out->sustain = cb2gain((float)sus);

    return true;
  }
  return false;
}

uint8_t SF2::findRegions(uint8_t bank, uint8_t program, uint8_t key,
                        uint8_t velocity, Region* out, uint8_t maxRegions) {
  if (!_valid || maxRegions == 0) return 0;

  // Locate the preset record. Prefer exact (bank, program); fall back to the
  // same program in bank 0; then to program 0 in the requested bank.
  int best = -1, fallback = -1, fallback2 = -1;
  // phdr has a terminal record at the end; iterate count-1 real presets.
  uint32_t presets = _phdrCount > 0 ? _phdrCount - 1 : 0;
  for (uint32_t i = 0; i < presets; i++) {
    const uint8_t* ph = _phdr + (uint32_t)i * PHDR_SZ;
    uint16_t wPreset = rd16(ph + 20);
    uint16_t wBank   = rd16(ph + 22);
    if (wBank == bank && wPreset == program) { best = i; break; }
    if (wBank == 0 && wPreset == program && fallback < 0) fallback = i;
    if (wBank == bank && fallback2 < 0) fallback2 = i;
  }
  if (best < 0) best = fallback;
  if (best < 0) best = fallback2;
  if (best < 0) return 0;

  const uint8_t* ph    = _phdr + (uint32_t)best * PHDR_SZ;
  uint16_t bagStart    = rd16(ph + 24);
  uint16_t bagEnd      = rd16(_phdr + (uint32_t)(best + 1) * PHDR_SZ + 24);

  // global preset zone additive generators
  int16_t gAdd[64]; bool gHas[64];
  memset(gAdd, 0, sizeof(gAdd));
  memset(gHas, 0, sizeof(gHas));
  uint8_t gKeyLo = 0, gKeyHi = 127, gVelLo = 0, gVelHi = 127;

  uint8_t count = 0;
  for (uint16_t b = bagStart; b < bagEnd && count < maxRegions; b++) {
    uint16_t genStart = rd16(_pbag + (uint32_t)b * PBAG_SZ);
    uint16_t genEnd   = rd16(_pbag + (uint32_t)(b + 1) * PBAG_SZ);

    int32_t vals[64]; bool has[64];
    memset(vals, 0, sizeof(vals));
    memset(has, 0, sizeof(has));
    // start from global additive values
    for (int g = 0; g < 64; g++) vals[g] = gAdd[g];
    memcpy(has, gHas, sizeof(has));
    uint8_t keyLo = gKeyLo, keyHi = gKeyHi, velLo = gVelLo, velHi = gVelHi;
    readZoneGens(_pgen, genStart, genEnd, vals, has, &keyLo, &keyHi, &velLo, &velHi);

    if (!has[GEN_instrument]) {
      // global preset zone: carry additive generators forward
      for (int g = 0; g < 64; g++) gAdd[g] = (int16_t)vals[g];
      memcpy(gHas, has, sizeof(gHas));
      gKeyLo = keyLo; gKeyHi = keyHi; gVelLo = velLo; gVelHi = velHi;
      continue;
    }

    if (key < keyLo || key > keyHi) continue;
    if (velocity < velLo || velocity > velHi) continue;

    int16_t presetAdd[64];
    for (int g = 0; g < 64; g++) presetAdd[g] = (int16_t)vals[g];

    uint16_t instId = (uint16_t)vals[GEN_instrument];
    if (resolveInstrument(instId, key, velocity, presetAdd, &out[count]))
      count++;
  }
  return count;
}

} // namespace sf2
