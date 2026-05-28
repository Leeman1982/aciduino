#pragma once
// ── JUNO SYNTH ENGINE ─────────────────────────────────────────────────────────
// Extracted from Juno6_v4_BandLimited (github.com/Leeman1982/ultimatesynthrp2350)
// Adapted for ZOMBI SS v2: renamed globals, removed standalone MIDI I/O,
// exposed junoNoteOn / junoNoteOff for sequencer integration.
//
// BAND-LIMITED OSCILLATORS
// To use the real BL_Oscillator_ESP32.h from the ultimatesynthrp2350 repo:
//   1. Copy BL_Oscillator_ESP32.h into this sketch folder
//   2. Add #define BL_OSCILLATOR_AVAILABLE before including this file
// Without it, a simple (non-band-limited) oscillator is used — functional
// but may alias at very high frequencies.
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

// ── Oscillator: use BL_Oscillator_ESP32 if available, else simple fallback ──
#ifdef BL_OSCILLATOR_AVAILABLE
  #include "BL_Oscillator_ESP32.h"
#else
// Simple wavetable oscillator — drop-in API-compatible with BLOscillator.
// Replace with BL_Oscillator_ESP32.h for alias-free audio.
class BLOscillator {
    float _phase    = 0.0f;
    float _phaseInc = 0.0f;
    int   _wave     = 0;
public:
    void setWaveform(int w) { _wave = w & 3; }
    void setFrequency(float f) {
        _phaseInc = constrain(f, 10.0f, 20000.0f) / JUNO_SAMPLE_RATE;
    }
    float next() {
        float v = 0.0f;
        switch (_wave) {
            case 0: v = 1.0f - 2.0f * _phase; break;                                       // Sawtooth
            case 1: v = (_phase < 0.5f) ? 1.0f : -1.0f; break;                             // Square
            case 2: v = (_phase < 0.5f) ? (4.0f*_phase - 1.0f) : (3.0f - 4.0f*_phase); break; // Triangle
            case 3: v = sinf(TWO_PI * _phase); break;                                       // Sine
        }
        _phase += _phaseInc;
        if (_phase >= 1.0f) _phase -= 1.0f;
        return v;
    }
    void reset() { _phase = 0.0f; }
};
class BLWavetables { public: static void init(float) {} };
#endif

// ── Constants ─────────────────────────────────────────────────────────────────
static const float J_INV_SR    = 1.0f / JUNO_SAMPLE_RATE;
static const float J_MIN_CUTOFF = 80.0f;
static const float J_MAX_CUTOFF = 12000.0f;
static const float J_MIN_RESO   = 0.0f;
static const float J_MAX_RESO   = 0.95f;
static const float J_MIN_LFO    = 0.05f;
static const float J_MAX_LFO    = 20.0f;
static const float J_OSC_GAIN   = 0.30f;
static const float J_VOICE_GAIN = 0.85f;
static const float J_MASTER_GAIN= 0.75f;
static const uint16_t J_ENV_MIN = 1;
static const uint16_t J_ENV_MAX = 2000;

// ── Exposed patch settings (modifiable from INST mode) ───────────────────────
struct JunoOscSettings {
    uint8_t wave1 = 0, wave2 = 0;  // 0=Saw 1=Sqr 2=Tri 3=Sin
    int8_t  osc2Coarse = 0, osc2Detune = 7;
    float   mix = 0.5f, mixSmooth = 0.5f;
    float   subLevel = 0.2f, subLevelSmooth = 0.2f;
    bool    hardSync = false;
};
struct JunoFiltSettings {
    float cutoff = 3000.0f, cutoffSmooth = 3000.0f;
    float reso   = 0.2f,    resoSmooth   = 0.2f;
    float envAmount = 2000.0f;
    float kbdTrack  = 0.5f;
    float drive = 0.0f,     driveSmooth  = 0.0f;
};
struct JunoLFOSettings {
    float rate   = 2.5f;
    float amount = 0.0f, amountSmooth = 0.0f;
    uint8_t dest = 0;    // 0=pitch 1=filter 2=amp
};
struct JunoPerfSettings {
    float   masterVol = 0.8f, masterVolSmooth = 0.8f;
    float   velSens   = 0.5f;
    int16_t pitchBend = 0;
    float   pitchBendRange = 2.0f;
    bool    chorus = true;
};

// Envelope ADSR times (ms) — shared across all voices, adjustable from INST mode
struct JunoEnvTimes {
    uint16_t ampAttack  =  10;
    uint16_t ampDecay   = 100;
    float    ampSustain = 0.7f;
    uint16_t ampRelease = 200;
    uint16_t filtAttack =  10;
    uint16_t filtDecay  = 150;
    float    filtSustain= 0.4f;
    uint16_t filtRelease= 200;
};

JunoOscSettings  junoOsc;
JunoFiltSettings junoFilt;
JunoLFOSettings  junoLFO;
JunoPerfSettings junoPerf;
JunoEnvTimes     junoEnv;

// ── Moog TPT ladder filter ─────────────────────────────────────────────────
class MoogTPT {
public:
    float s1=0,s2=0,s3=0,s4=0,G=0,g=0,k=0;

    void setCutoff(float freq) {
        freq = constrain(freq, J_MIN_CUTOFF, J_MAX_CUTOFF);
        g = tanf(PI * freq / JUNO_SAMPLE_RATE);
        G = g / (1.0f + g);
    }
    void setResonance(float r) { k = 4.0f * constrain(r, J_MIN_RESO, J_MAX_RESO); }

    float process(float in) {
        float Gc = G*G*G*G;
        float rc = 1.0f / (1.0f + k * Gc);
        float u  = constrain((in - k * s4) * rc, -4.0f, 4.0f);
        float v1=(u -s1)*G; float y1=v1+s1; s1=constrain(v1+y1,-4,4);
        float v2=(y1-s2)*G; float y2=v2+s2; s2=constrain(v2+y2,-4,4);
        float v3=(y2-s3)*G; float y3=v3+s3; s3=constrain(v3+y3,-4,4);
        float v4=(y3-s4)*G; float y4=v4+s4; s4=constrain(v4+y4,-4,4);
        return y4;
    }
    void reset() { s1=s2=s3=s4=0; }
};

// ── ADSR envelope ─────────────────────────────────────────────────────────
class ADSREnv {
public:
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } state = IDLE;
    float level=0, ar=0, dr=0, sl=0.7f, rr=0;

    float toRate(uint16_t ms) {
        ms = constrain(ms, J_ENV_MIN, J_ENV_MAX);
        return 1.0f - expf(-5.0f / (ms * JUNO_SAMPLE_RATE * 0.001f));
    }
    void setAttack(uint16_t ms)  { ar = toRate(ms); }
    void setDecay(uint16_t ms)   { dr = toRate(ms); }
    void setSustain(float v)     { sl = constrain(v, 0.0f, 1.0f); }
    void setRelease(uint16_t ms) { rr = toRate(ms); }

    void noteOn()  { state = ATTACK; }
    void noteOff() { if (state != IDLE) state = RELEASE; }
    bool isActive(){ return state != IDLE; }
    void reset()   { state = IDLE; level = 0; }

    float process() {
        switch (state) {
            case ATTACK:
                level += (1.0f - level) * ar;
                if (level >= 0.999f) { level=1; state=DECAY; }
                break;
            case DECAY:
                level += (sl - level) * dr;
                if (fabsf(level-sl) < 0.001f) { level=sl; state=SUSTAIN; }
                break;
            case SUSTAIN: level = sl; break;
            case RELEASE:
                level *= (1.0f - rr);
                if (level < 0.001f) { level=0; state=IDLE; }
                break;
            case IDLE: level = 0; break;
        }
        return level;
    }
};

// ── LFO ───────────────────────────────────────────────────────────────────
class JunoLFOosc {
    float _phase=0, _freq=2.5f;
public:
    void setFreq(float f){ _freq=constrain(f, J_MIN_LFO, J_MAX_LFO); }
    float process() {
        float t = _phase / TWO_PI;
        float v = (t < 0.5f) ? (4*t-1) : (3-4*t);
        _phase += TWO_PI * _freq * J_INV_SR;
        if (_phase >= TWO_PI) _phase -= TWO_PI;
        return v;
    }
};

// ── Stereo chorus ─────────────────────────────────────────────────────────
class JunoChorus {
    float buf[1024]; int pos=0; float p1=0, p2=0.5f;
public:
    JunoChorus() { memset(buf, 0, sizeof(buf)); }
    void process(float in, float* L, float* R) {
        buf[pos] = in;
        float l1 = sinf(p1), l2 = sinf(p2);
        float d1 = 350.0f + l1*150.0f, d2 = 400.0f + l2*150.0f;
        int i1=(int)d1; float f1=d1-i1;
        int ia=(pos-i1+1024)&1023, ib=(ia-1+1024)&1023;
        float w1 = buf[ia]+f1*(buf[ib]-buf[ia]);
        int i2=(int)d2; float f2=d2-i2;
        int ic=(pos-i2+1024)&1023, id=(ic-1+1024)&1023;
        float w2 = buf[ic]+f2*(buf[id]-buf[ic]);
        *L = in*0.6f + w1*0.4f;
        *R = in*0.6f + w2*0.4f;
        p1 += 0.5f*TWO_PI*J_INV_SR; p2 += 0.63f*TWO_PI*J_INV_SR;
        if (p1>=TWO_PI) p1-=TWO_PI; if (p2>=TWO_PI) p2-=TWO_PI;
        pos = (pos+1)&1023;
    }
};

// ── Voice ─────────────────────────────────────────────────────────────────
struct JunoVoice {
    BLOscillator osc1, osc2, sub;
    ADSREnv      ampEnv, filtEnv;
    MoogTPT      filter;
    uint8_t  note=255;
    uint8_t  velocity=0;
    bool     active=false;
    float    velFactor=1.0f;
    uint32_t noteOnTime=0;
    float    freq=440.0f;
    float    lastOsc1=0;

    void setFreqs(float base) {
        base = constrain(base, 20.0f, 20000.0f);
        osc1.setFrequency(base);
        float f2 = base;
        if (junoOsc.osc2Coarse) f2 *= powf(2.0f, junoOsc.osc2Coarse / 12.0f);
        if (junoOsc.osc2Detune) f2 *= powf(2.0f, junoOsc.osc2Detune / 1200.0f);
        osc2.setFrequency(constrain(f2, 20.0f, 20000.0f));
        sub.setFrequency(base * 0.5f);
    }
    void applyEnvTimes() {
        ampEnv.setAttack(junoEnv.ampAttack);   ampEnv.setDecay(junoEnv.ampDecay);
        ampEnv.setSustain(junoEnv.ampSustain); ampEnv.setRelease(junoEnv.ampRelease);
        filtEnv.setAttack(junoEnv.filtAttack); filtEnv.setDecay(junoEnv.filtDecay);
        filtEnv.setSustain(junoEnv.filtSustain);filtEnv.setRelease(junoEnv.filtRelease);
    }
};

// ── Globals ───────────────────────────────────────────────────────────────
static JunoVoice  _jVoices[JUNO_VOICES];
static JunoLFOosc _jLFO;
static JunoChorus _jChorus;

static float _jDcX=0, _jDcY=0;
static const float J_DC_R = 0.995f;

// Frequency lookup
static float _jFreqTable[128];

static inline float _jMtof(uint8_t n) { return _jFreqTable[n & 127]; }

static float _jDcBlock(float x) {
    float y = x - _jDcX + J_DC_R * _jDcY;
    _jDcX = x; _jDcY = y;
    return y;
}

static float _jFastTanh(float x) {
    if (x < -3.0f) return -1.0f;
    if (x >  3.0f) return  1.0f;
    float x2 = x*x;
    return x*(27.0f+x2)/(27.0f+9.0f*x2);
}

// ── Note handling (called from sequencer, Core 0) ────────────────────────
static uint8_t _jFindVoice() {
    for (int i=0;i<JUNO_VOICES;i++)
        if (!_jVoices[i].active && !_jVoices[i].ampEnv.isActive()) return i;
    uint32_t oldest=0xFFFFFFFF; int idx=0;
    for (int i=0;i<JUNO_VOICES;i++)
        if (_jVoices[i].noteOnTime < oldest){ oldest=_jVoices[i].noteOnTime; idx=i; }
    return idx;
}

void junoNoteOn(uint8_t note, uint8_t vel) {
    note = note & 127;
    vel  = constrain(vel, 1, 127);
    uint8_t vi = _jFindVoice();
    _jVoices[vi].note       = note;
    _jVoices[vi].velocity   = vel;
    _jVoices[vi].active     = true;
    _jVoices[vi].freq       = _jMtof(note);
    _jVoices[vi].noteOnTime = millis();
    _jVoices[vi].velFactor  = 1.0f - (junoPerf.velSens * (1.0f - vel/127.0f));
    _jVoices[vi].velFactor  = constrain(_jVoices[vi].velFactor, 0.3f, 1.0f);
    _jVoices[vi].setFreqs(_jVoices[vi].freq);
    _jVoices[vi].applyEnvTimes();
    _jVoices[vi].ampEnv.noteOn();
    _jVoices[vi].filtEnv.noteOn();
}

void junoNoteOff(uint8_t note) {
    note = note & 127;
    for (int i=0;i<JUNO_VOICES;i++) {
        if (_jVoices[i].note==note && _jVoices[i].active) {
            _jVoices[i].active = false;
            _jVoices[i].ampEnv.noteOff();
            _jVoices[i].filtEnv.noteOff();
        }
    }
}

void junoAllNotesOff() {
    for (int i=0;i<JUNO_VOICES;i++) {
        _jVoices[i].note   = 255;
        _jVoices[i].active = false;
        _jVoices[i].ampEnv.reset();
        _jVoices[i].filtEnv.reset();
    }
}

// ── Parameter smoothing (call from main loop / Control task) ─────────────
void smoothJunoParams() {
    junoOsc.mixSmooth       += (junoOsc.mix       - junoOsc.mixSmooth)       * 0.002f;
    junoOsc.subLevelSmooth  += (junoOsc.subLevel  - junoOsc.subLevelSmooth)  * 0.002f;
    junoFilt.cutoffSmooth   += (junoFilt.cutoff   - junoFilt.cutoffSmooth)   * 0.005f;
    junoFilt.resoSmooth     += (junoFilt.reso     - junoFilt.resoSmooth)     * 0.005f;
    junoFilt.driveSmooth    += (junoFilt.drive    - junoFilt.driveSmooth)    * 0.002f;
    junoPerf.masterVolSmooth+= (junoPerf.masterVol- junoPerf.masterVolSmooth)* 0.002f;
    junoLFO.amountSmooth    += (junoLFO.amount    - junoLFO.amountSmooth)    * 0.001f;
}

// ── Audio render (called from audioTask on Core 1) ───────────────────────
static void _junoRenderAudio(int16_t* buf, int n) {
    for (int s=0; s<n; s++) {
        float lfoVal = _jLFO.process();
        float mix=0; uint8_t cnt=0;

        for (int vi=0; vi<JUNO_VOICES; vi++) {
            if (!_jVoices[vi].active && !_jVoices[vi].ampEnv.isActive()) continue;
            cnt++;
            float freq = _jVoices[vi].freq;
            if (junoPerf.pitchBend)
                freq *= powf(2.0f, (junoPerf.pitchBend/8192.0f) * junoPerf.pitchBendRange / 12.0f);
            if (junoLFO.dest==0 && junoLFO.amountSmooth > 0.01f)
                freq *= 1.0f + lfoVal * junoLFO.amountSmooth * 0.1f;

            _jVoices[vi].setFreqs(freq);

            float o1 = _jVoices[vi].osc1.next();
            float o2 = _jVoices[vi].osc2.next();
            float sb = _jVoices[vi].sub.next();

            if (junoOsc.hardSync) {
                if (_jVoices[vi].lastOsc1 < 0.0f && o1 >= 0.0f)
                    _jVoices[vi].osc2.reset();
                _jVoices[vi].lastOsc1 = o1;
            }

            float oMix = o1*(1.0f-junoOsc.mixSmooth) + o2*junoOsc.mixSmooth;
            oMix += sb * junoOsc.subLevelSmooth;
            oMix *= J_OSC_GAIN;

            float fc = junoFilt.cutoffSmooth;
            if (junoLFO.dest==1 && junoLFO.amountSmooth>0.01f)
                fc *= 1.0f + lfoVal * junoLFO.amountSmooth;
            float fe = _jVoices[vi].filtEnv.process();
            fc += fe * junoFilt.envAmount;
            if (junoFilt.kbdTrack > 0.05f && _jVoices[vi].note < 128)
                fc *= powf(2.0f, ((_jVoices[vi].note-60)*junoFilt.kbdTrack)/12.0f);
            fc = constrain(fc, J_MIN_CUTOFF, J_MAX_CUTOFF);
            _jVoices[vi].filter.setCutoff(fc);
            _jVoices[vi].filter.setResonance(junoFilt.resoSmooth);
            float filtered = _jVoices[vi].filter.process(oMix);

            if (junoFilt.driveSmooth > 0.01f)
                filtered = _jFastTanh(filtered * (1.0f + junoFilt.driveSmooth) * 0.7f);

            float ae = _jVoices[vi].ampEnv.process();
            float ampMod = 1.0f;
            if (junoLFO.dest==2 && junoLFO.amountSmooth>0.01f)
                ampMod = 1.0f + lfoVal * junoLFO.amountSmooth * 0.3f;

            mix += filtered * ae * _jVoices[vi].velFactor * ampMod * J_VOICE_GAIN;
        }

        if (cnt > 1) mix *= 1.0f / sqrtf((float)cnt);
        mix *= junoPerf.masterVolSmooth * J_MASTER_GAIN;
        mix = _jDcBlock(mix);

        float L, R;
        if (junoPerf.chorus) {
            _jChorus.process(mix, &L, &R);
        } else {
            L = R = mix;
        }
        L = constrain(L, -0.95f, 0.95f);
        R = constrain(R, -0.95f, 0.95f);
        buf[s*2]   = (int16_t)(L * 32000.0f);
        buf[s*2+1] = (int16_t)(R * 32000.0f);
    }
}

// ── FreeRTOS audio task (Core 1) ─────────────────────────────────────────
static void _junoAudioTask(void*) {
    int16_t buf[JUNO_BUF_SIZE * 2];
    size_t  written;
    while (true) {
        _junoRenderAudio(buf, JUNO_BUF_SIZE);
        i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, portMAX_DELAY);
    }
}

// ── I2S initialisation ───────────────────────────────────────────────────
static void _junoI2SInit() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = (uint32_t)JUNO_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = JUNO_BUF_SIZE,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCK_PIN,
        .ws_io_num    = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pins);
}

// ── Public init (call from setup, AFTER SPI.begin) ───────────────────────
void initJunoSynth() {
    BLWavetables::init(JUNO_SAMPLE_RATE);

    for (int i=0; i<128; i++)
        _jFreqTable[i] = 440.0f * powf(2.0f, (i-69)/12.0f);

    for (int i=0; i<JUNO_VOICES; i++) {
        _jVoices[i].osc1.setWaveform(0);
        _jVoices[i].osc2.setWaveform(0);
        _jVoices[i].sub.setWaveform(3);  // sine sub
        _jVoices[i].applyEnvTimes();
        _jVoices[i].filter.setCutoff(junoFilt.cutoff);
        _jVoices[i].filter.setResonance(junoFilt.reso);
    }
    _jLFO.setFreq(junoLFO.rate);
    _junoI2SInit();

    xTaskCreatePinnedToCore(_junoAudioTask, "JunoAudio",
                            4096, NULL, configMAX_PRIORITIES-1, NULL, 1);
    Serial.println(F("[juno] Synth ready"));
}

// ── Waveform name helper (for INST mode display) ─────────────────────────
inline const char* junoWaveName(uint8_t w) {
    static const char* N[] = {"Sawtooth","Square  ","Triangle","Sine    "};
    return N[w & 3];
}

// ── Apply waveform to all voices (call after changing junoOsc.wave1) ─────
void junoSetWaveform(uint8_t w) {
    junoOsc.wave1 = w & 3;
    for (int i=0; i<JUNO_VOICES; i++) _jVoices[i].osc1.setWaveform(junoOsc.wave1);
}

// ── Apply updated envelope times to all voices ───────────────────────────
void junoApplyEnvTimes() {
    for (int i=0; i<JUNO_VOICES; i++) _jVoices[i].applyEnvTimes();
}
