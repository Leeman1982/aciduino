#include "synth_config.h"
#include "gm_synth.h"

#if defined(USE_GM_SYNTH) && (defined(ARDUINO_ARCH_ESP32) || defined(ESP32))

#include "sf2.h"
#include "soundfont_vintage_dreams.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"      // legacy I2S driver (broadest core compatibility)

#define I2S_PORT  I2S_NUM_0
#define AUDIO_BLOCK 64       // frames rendered per i2s_write

// ---------------------------------------------------------------------------
// internal state
// ---------------------------------------------------------------------------
static sf2::SF2 s_font;

enum EnvStage { ENV_IDLE, ENV_ATTACK, ENV_HOLD, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

struct Voice {
  bool      active = false;
  uint8_t   channel;
  uint8_t   note;
  sf2::Region region;

  double    pos;        // play position in samples, relative to region.start
  double    inc;        // phase increment per output sample
  uint32_t  playLen;    // region length in samples
  uint32_t  loopStart;  // relative to region.start
  uint32_t  loopEnd;
  bool      canLoop;

  float     gainL, gainR;   // static gain (attenuation * velocity * pan)

  EnvStage  stage;
  float     env;        // current envelope level 0..1
  float     attackInc, decayInc, releaseInc;
  uint32_t  holdSamples, holdCount;
  float     sustainLevel;
};

static Voice s_voices[GM_SYNTH_MAX_VOICES];

static uint8_t s_program[16];
static uint8_t s_bank[16];

// cross-core event queue
struct MidiEvent { uint8_t type; uint8_t channel; uint8_t d1; uint8_t d2; };
enum { EV_NOTE_ON = 1, EV_NOTE_OFF, EV_PROGRAM, EV_CC, EV_ALL_OFF };
static QueueHandle_t s_queue = nullptr;

static const int16_t* s_smpl = nullptr;
static uint32_t s_smplCount = 0;

// ---------------------------------------------------------------------------
// voice allocation / control (audio-task side)
// ---------------------------------------------------------------------------
static inline float secToInc(float seconds) {
  // per-sample increment to ramp 0..1 over 'seconds'
  float n = seconds * (float)GM_SYNTH_SAMPLE_RATE;
  if (n < 1.0f) n = 1.0f;
  return 1.0f / n;
}

static int findFreeVoice() {
  for (int i = 0; i < GM_SYNTH_MAX_VOICES; i++)
    if (!s_voices[i].active) return i;
  // steal the quietest voice
  int best = 0; float lowest = 2.0f;
  for (int i = 0; i < GM_SYNTH_MAX_VOICES; i++) {
    if (s_voices[i].env < lowest) { lowest = s_voices[i].env; best = i; }
  }
  return best;
}

static void startVoice(uint8_t channel, uint8_t note, uint8_t velocity) {
  uint8_t bank = (channel == GM_SYNTH_DRUM_CHANNEL) ? 128 : s_bank[channel];
  uint8_t prog = (channel == GM_SYNTH_DRUM_CHANNEL) ? 0 : s_program[channel];

  sf2::Region regs[4];
  uint8_t n = s_font.findRegions(bank, prog, note, velocity, regs, 4);

  for (uint8_t r = 0; r < n; r++) {
    sf2::Region& reg = regs[r];
    int vi = findFreeVoice();
    Voice& v = s_voices[vi];

    v.active   = true;
    v.channel  = channel;
    v.note     = note;
    v.region   = reg;
    v.pos      = 0.0;

    // pitch ratio from key/root and tuning
    float semis = (float)((int)note - (int)reg.rootKey) + (float)reg.tuneCents / 100.0f;
    float ratio = powf(2.0f, semis / 12.0f);
    v.inc = ((double)reg.sampleRate / (double)GM_SYNTH_SAMPLE_RATE) * (double)ratio;

    v.playLen   = (reg.end > reg.start) ? (reg.end - reg.start) : 0;
    v.loopStart = (reg.loopStart > reg.start) ? (reg.loopStart - reg.start) : 0;
    v.loopEnd   = (reg.loopEnd > reg.start) ? (reg.loopEnd - reg.start) : 0;
    v.canLoop   = (reg.loopMode != 0) && (v.loopEnd > v.loopStart) && (v.loopEnd <= v.playLen);

    float g = reg.attenuation * ((float)velocity / 127.0f);
    float pan = reg.pan; // -1..1
    v.gainL = g * (pan <= 0 ? 1.0f : 1.0f - pan);
    v.gainR = g * (pan >= 0 ? 1.0f : 1.0f + pan);

    // envelope
    v.stage = ENV_ATTACK;
    v.env = 0.0f;
    v.attackInc  = secToInc(reg.attack);
    v.decayInc   = secToInc(reg.decay);
    v.releaseInc = secToInc(reg.release < 0.02f ? 0.02f : reg.release);
    v.holdSamples = (uint32_t)(reg.hold * (float)GM_SYNTH_SAMPLE_RATE);
    v.holdCount = 0;
    v.sustainLevel = reg.sustain;
  }
}

static void releaseVoice(uint8_t channel, uint8_t note) {
  for (int i = 0; i < GM_SYNTH_MAX_VOICES; i++) {
    Voice& v = s_voices[i];
    if (v.active && v.channel == channel && v.note == note && v.stage != ENV_RELEASE) {
      v.stage = ENV_RELEASE;
    }
  }
}

static void applyEvent(const MidiEvent& e) {
  switch (e.type) {
    case EV_NOTE_ON:
      if (e.d2 == 0) releaseVoice(e.channel, e.d1);
      else startVoice(e.channel, e.d1, e.d2);
      break;
    case EV_NOTE_OFF:
      releaseVoice(e.channel, e.d1);
      break;
    case EV_PROGRAM:
      if (e.channel < 16) s_program[e.channel] = e.d1;
      break;
    case EV_CC:
      // CC 120/123 = all notes / sound off
      if (e.d1 == 120 || e.d1 == 123) {
        for (int i = 0; i < GM_SYNTH_MAX_VOICES; i++)
          if (s_voices[i].active && s_voices[i].channel == e.channel)
            s_voices[i].stage = ENV_RELEASE;
      }
      break;
    case EV_ALL_OFF:
      for (int i = 0; i < GM_SYNTH_MAX_VOICES; i++) s_voices[i].active = false;
      break;
  }
}

// ---------------------------------------------------------------------------
// envelope + sample render
// ---------------------------------------------------------------------------
static inline void advanceEnv(Voice& v) {
  switch (v.stage) {
    case ENV_ATTACK:
      v.env += v.attackInc;
      if (v.env >= 1.0f) { v.env = 1.0f; v.stage = ENV_HOLD; v.holdCount = 0; }
      break;
    case ENV_HOLD:
      if (v.holdCount++ >= v.holdSamples) v.stage = ENV_DECAY;
      break;
    case ENV_DECAY:
      v.env -= v.decayInc;
      if (v.env <= v.sustainLevel) { v.env = v.sustainLevel; v.stage = ENV_SUSTAIN; }
      break;
    case ENV_SUSTAIN:
      break;
    case ENV_RELEASE:
      v.env -= v.releaseInc;
      if (v.env <= 0.0f) { v.env = 0.0f; v.active = false; }
      break;
    default:
      v.active = false;
      break;
  }
}

static void renderBlock(int16_t* out, int frames) {
  for (int f = 0; f < frames; f++) {
    float left = 0.0f, right = 0.0f;

    for (int i = 0; i < GM_SYNTH_MAX_VOICES; i++) {
      Voice& v = s_voices[i];
      if (!v.active) continue;

      uint32_t idx = (uint32_t)v.pos;
      float frac = (float)(v.pos - (double)idx);

      // loopMode 1 loops through the release tail; loopMode 3 stops looping
      // once released and plays out to the sample end.
      bool looping = v.canLoop && (v.stage != ENV_RELEASE || v.region.loopMode == 1);

      // bounds / end handling
      if (!looping && idx >= v.playLen) { v.active = false; continue; }

      uint32_t a_idx = v.region.start + idx;
      uint32_t n_idx = idx + 1;
      if (looping && n_idx >= v.loopEnd) n_idx = v.loopStart;
      uint32_t b_idx = v.region.start + n_idx;

      float s0 = (a_idx < s_smplCount) ? (float)s_smpl[a_idx] : 0.0f;
      float s1 = (b_idx < s_smplCount) ? (float)s_smpl[b_idx] : 0.0f;
      float sample = (s0 + (s1 - s0) * frac) * v.env;

      left  += sample * v.gainL;
      right += sample * v.gainR;

      // advance position
      v.pos += v.inc;
      if (looping && v.pos >= (double)v.loopEnd)
        v.pos -= (double)(v.loopEnd - v.loopStart);

      advanceEnv(v);
    }

    // master gain low enough to leave headroom for stacked voices, then a
    // tanh soft-limiter so dense hits compress gracefully instead of clipping.
    const float MASTER = 0.30f;
    left  = tanhf(left  * MASTER * (1.0f / 32768.0f)) * 32767.0f;
    right = tanhf(right * MASTER * (1.0f / 32768.0f)) * 32767.0f;

    out[2 * f]     = (int16_t)left;
    out[2 * f + 1] = (int16_t)right;
  }
}

// ---------------------------------------------------------------------------
// audio task
// ---------------------------------------------------------------------------
static void audioTask(void* arg) {
  static int16_t buffer[AUDIO_BLOCK * 2];
  for (;;) {
    // drain pending MIDI events
    MidiEvent e;
    while (xQueueReceive(s_queue, &e, 0) == pdTRUE) applyEvent(e);

    renderBlock(buffer, AUDIO_BLOCK);

    size_t written = 0;
    i2s_write(I2S_PORT, buffer, sizeof(buffer), &written, portMAX_DELAY);
  }
}

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------
bool gmSynthInit() {
  for (int i = 0; i < 16; i++) { s_program[i] = 0; s_bank[i] = 0; }

  if (!s_font.load(sf2_vintage_dreams, sf2_vintage_dreams_len)) {
    return false;
  }
  s_smpl = s_font.samples();
  s_smplCount = s_font.sampleCount();

  s_queue = xQueueCreate(64, sizeof(MidiEvent));
  if (!s_queue) return false;

  // I2S setup for PCM5102 (16-bit stereo)
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = GM_SYNTH_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = AUDIO_BLOCK;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;

  if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) return false;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = GM_SYNTH_I2S_BCK_PIN;
  pins.ws_io_num  = GM_SYNTH_I2S_WS_PIN;
  pins.data_out_num = GM_SYNTH_I2S_DATA_PIN;
  pins.data_in_num  = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) return false;

  // render on core 0 (Arduino loop / uClock run on core 1)
  xTaskCreatePinnedToCore(audioTask, "gm_synth", 4096, NULL, 2, NULL, 0);
  return true;
}

static inline void push(uint8_t type, uint8_t ch, uint8_t d1, uint8_t d2) {
  if (!s_queue) return;
  MidiEvent e = { type, ch, d1, d2 };
  xQueueSend(s_queue, &e, 0);   // non-blocking; drop if full
}

void gmSynthNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  push(EV_NOTE_ON, channel & 0x0F, note, velocity);
}
void gmSynthNoteOff(uint8_t channel, uint8_t note) {
  push(EV_NOTE_OFF, channel & 0x0F, note, 0);
}
void gmSynthProgramChange(uint8_t channel, uint8_t program) {
  push(EV_PROGRAM, channel & 0x0F, program, 0);
}
void gmSynthControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
  push(EV_CC, channel & 0x0F, cc, value);
}
void gmSynthAllNotesOff() {
  push(EV_ALL_OFF, 0, 0, 0);
}

#else // synth disabled or non-ESP32 -> no-op stubs so other ports still build

bool gmSynthInit() { return false; }
void gmSynthNoteOn(uint8_t, uint8_t, uint8_t) {}
void gmSynthNoteOff(uint8_t, uint8_t) {}
void gmSynthProgramChange(uint8_t, uint8_t) {}
void gmSynthControlChange(uint8_t, uint8_t, uint8_t) {}
void gmSynthAllNotesOff() {}

#endif
