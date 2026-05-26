#pragma once
#include <Arduino.h>

// All 128 GM instrument names (index 0 = GM program 1 "Acoustic Grand Piano")
static const char GM_NAMES[128][13] = {
    "Ac.Piano   ","Bright Pno ","E.Grand Pno","Honky-Tonk ",  // 1-4
    "E.Piano 1  ","E.Piano 2  ","Harpsichord","Clavinet   ",  // 5-8
    "Celesta    ","Glocknspiel","Music Box  ","Vibraphone ",  // 9-12
    "Marimba    ","Xylophone  ","Tubular Bel","Dulcimer   ",  // 13-16
    "Drawbar Org","Perc.Organ ","Rock Organ ","Church Org.",  // 17-20
    "Reed Organ ","Accordion  ","Harmonica  ","Tango Acc. ",  // 21-24
    "Nylon Gtr. ","Steel Gtr. ","Jazz Guitar","Clean Gtr. ",  // 25-28
    "Muted Gtr. ","Overdrive  ","Distortion ","Harmonics  ",  // 29-32
    "Ac.Bass    ","Finger Bass","Pick Bass  ","Fretless   ",  // 33-36
    "Slap Bass1 ","Slap Bass2 ","Synth Bass1","Synth Bass2",  // 37-40
    "Violin     ","Viola      ","Cello      ","Contrabass ",  // 41-44
    "Tremolo Str","Pizzicato  ","Orch.Harp  ","Timpani    ",  // 45-48
    "Strings 1  ","Strings 2  ","Syn.Str. 1 ","Syn.Str. 2 ",  // 49-52
    "Choir Aahs ","Voice Oohs ","Synth Voice","Orch.Hit   ",  // 53-56
    "Trumpet    ","Trombone   ","Tuba       ","Muted Tpt. ",  // 57-60
    "French Horn","Brass Sect.","SynBrass 1 ","SynBrass 2 ",  // 61-64
    "Soprano Sax","Alto Sax   ","Tenor Sax  ","Bari.Sax   ",  // 65-68
    "Oboe       ","Eng.Horn   ","Bassoon    ","Clarinet   ",  // 69-72
    "Piccolo    ","Flute      ","Recorder   ","Pan Flute  ",  // 73-76
    "BottleBlow ","Shakuhachi ","Whistle    ","Ocarina    ",  // 77-80
    "Square Lead","Sawtooth   ","Calliope   ","Chiff Lead ",  // 81-84
    "Charang    ","Voice Lead ","5ths Lead  ","Bass+Lead  ",  // 85-88
    "New Age Pad","Warm Pad   ","Polysynth  ","Choir Pad  ",  // 89-92
    "Bowed Glass","Metal Pad  ","Halo Pad   ","Sweep Pad  ",  // 93-96
    "Rain FX    ","Soundtrack ","Crystal    ","Atmosphere ",  // 97-100
    "Brightness ","Goblins    ","Echoes     ","Sci-Fi     ",  // 101-104
    "Sitar      ","Banjo      ","Shamisen   ","Koto       ",  // 105-108
    "Kalimba    ","Bagpipe    ","Fiddle     ","Shanai     ",  // 109-112
    "TinkleBell ","Agogo      ","Steel Drums","Woodblock  ",  // 113-116
    "Taiko Drum ","Melody Tom ","Synth Drum ","Rev.Cymbal ",  // 117-120
    "Fret Noise ","Breath Nse.","Seashore   ","Bird Tweet ",  // 121-124
    "Telephone  ","Helicopter ","Applause   ","Gunshot    "   // 125-128
};

// GM percussion notes 35-81 (index 0 = note 35)
static const char DRUM_NAMES[47][13] = {
    "Ac.BassDrum","Bass Drum 1","Side Stick ","Ac.Snare   ",  // 35-38
    "Hand Clap  ","El.Snare   ","Lo.FloorTom","ClosedHiHat",  // 39-42
    "Hi.FloorTom","Pedal HiHat","Low Tom    ","Open HiHat ",  // 43-46
    "LoMid Tom  ","HiMid Tom  ","Crash Cym.1","High Tom   ",  // 47-50
    "Ride Cym.1 ","Chinese Cym","Ride Bell  ","Tambourine ",  // 51-54
    "Splash Cym.","Cowbell    ","Crash Cym.2","Vibraslap  ",  // 55-58
    "Ride Cym.2 ","Hi Bongo   ","Low Bongo  ","MuteHiConga",  // 59-62
    "OpenHiConga","Low Conga  ","Hi Timbale ","Lo Timbale ",  // 63-66
    "Hi Agogo   ","Low Agogo  ","Cabasa     ","Maracas    ",  // 67-70
    "ShrtWhistle","LongWhistle","Short Guiro","Long Guiro ",  // 71-74
    "Claves     ","Hi WoodBlk.","Lo WoodBlk.","Mute Cuica ",  // 75-78
    "Open Cuica ","MuteTriangl","OpenTriangl"                  // 79-81
};

inline const char* gmName(uint8_t idx) {
    return GM_NAMES[idx & 0x7F];
}

inline const char* drumName(uint8_t note) {
    if (note < 35 || note > 81) return "Unknown    ";
    return DRUM_NAMES[note - 35];
}

// Returns note name string into buf (e.g. "C5", "A#3")
inline void noteName(uint8_t midi, char* buf, uint8_t bufLen) {
    static const char* N[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    snprintf(buf, bufLen, "%s%d", N[midi % 12], (midi / 12) - 1);
}
