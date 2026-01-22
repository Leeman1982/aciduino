# Aciduino RP2040 - Quick Start Guide

**Get up and running in 30 minutes!**

This guide assumes you have a Raspberry Pi Pico and want to test Aciduino quickly before building the full hardware.

---

## Step 1: Minimal Hardware Setup (Testing)

For initial testing, you only need:

✅ **Raspberry Pi Pico**
✅ **1.3" OLED Display** (I2C)
✅ **4 Jumper Wires** (for OLED connection)
✅ **USB Cable** (Micro-USB data cable)

### Wiring (Minimal)

```
OLED Display → Raspberry Pi Pico
─────────────────────────────────
VCC  →  3.3V (Pin 36)
GND  →  GND  (Pin 38)
SDA  →  GP4  (Pin 6)
SCL  →  GP5  (Pin 7)
```

**That's it!** You can control navigation using USB Serial commands initially.

---

## Step 2: Install Arduino IDE

1. **Download Arduino IDE**:
   - Visit: https://www.arduino.cc/en/software
   - Download version 2.x or later

2. **Install RP2040 Board Support**:
   - Open Arduino IDE
   - Go to **File → Preferences**
   - In "Additional Board Manager URLs", add:
     ```
     https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
     ```
   - Click **OK**
   - Go to **Tools → Board → Board Manager**
   - Search for **"Pico"**
   - Install **"Raspberry Pi Pico/RP2040"** by Earle F. Philhower III

---

## Step 3: Install Libraries

Go to **Sketch → Include Library → Manage Libraries**, then search and install:

| Library Name | Author | Purpose |
|-------------|--------|---------|
| **U8g2** | oliver | OLED display driver |
| **MIDI Library** | Francois Best | MIDI communication |
| **Adafruit TinyUSB** | Adafruit | USB MIDI support |

**Optional** (for audio):
| Library Name | Author | Purpose |
|-------------|--------|---------|
| **Arduino Audio Tools** | Phil Schatzmann | Audio output (I2S/PWM) |

Click **Install** for each library.

---

## Step 4: Download Aciduino

### Option A: Using Git (Recommended)

```bash
git clone --recursive https://github.com/midilab/aciduino.git
cd aciduino/v2/AciduinoV2
```

### Option B: Manual Download

1. Visit: https://github.com/midilab/aciduino
2. Click **Code → Download ZIP**
3. Extract to your Arduino folder
4. **Important**: Download submodules separately:
   - uClock: https://github.com/midilab/uClock
   - uCtrl: https://github.com/midilab/uCtrl
   - Place in `AciduinoV2/src/` folders

---

## Step 5: Configure Arduino IDE

1. **Connect Pico**:
   - Hold **BOOTSEL** button while plugging in USB
   - Pico appears as USB drive
   - Release button

2. **Select Board**:
   - **Tools → Board** → **"Raspberry Pi Pico"**

3. **Configure Settings**:
   - **USB Stack**: **"Adafruit TinyUSB"** ⚠️ *Important!*
   - **CPU Speed**: **133 MHz** (overclock)
   - **Optimize**: **"Optimize Even More (-O3)"**
   - **Flash Size**: **"2MB (Sketch: 1MB, FS: 1MB)"**

4. **Select Port**:
   - **Tools → Port** → Select your Pico's COM port (e.g., COM3, /dev/ttyACM0)

---

## Step 6: Upload Firmware

1. **Open Project**:
   - Open `AciduinoV2.ino` in Arduino IDE

2. **Verify RP2040 Port Selected**:
   - Check that line 45 is uncommented:
     ```cpp
     #include "src/ports/rp2040/pico_oled.h"
     ```
   - Other ports should be commented out (lines 38-44)

3. **Upload**:
   - Click **Upload** button (→)
   - Wait for compilation (may take 2-3 minutes first time)
   - Upload progress bar shows transfer

4. **Success!**:
   - OLED should light up with Aciduino interface
   - Built-in LED (GP25) blinks with BPM

---

## Step 7: First Run

### What You'll See

The OLED displays the **System Page** with:
- Current BPM (default 120)
- Clock source (Internal)
- Track info
- Function buttons (F1/F2)

### Navigation Without Buttons

For testing, you can control via Serial Monitor:

1. Open **Tools → Serial Monitor**
2. Set baud rate: **115200**
3. Send commands:
   - `+` → Increment value
   - `-` → Decrement value
   - `n` → Next parameter
   - `p` → Previous parameter
   - `f1` → Page function 1
   - `f2` → Page function 2

*(Note: This requires adding serial command handling - or just add buttons!)*

---

## Step 8: Add Buttons (Recommended)

For full control, add at least 4 buttons:

| Button | Pin | Function |
|--------|-----|----------|
| F1 | GP18 | Page Up |
| F2 | GP19 | Page Down |
| - (Decrement) | GP16 | Decrease Value |
| + (Increment) | GP17 | Increase Value |

**Wiring**: Connect one side to pin, other side to **GND**.

Now you have basic navigation!

---

## Step 9: Test MIDI Output

### USB MIDI

1. Connect Pico to computer via USB
2. Open DAW (Ableton, FL Studio, etc.) or MIDI monitor
3. Select **"Aciduino RP2040"** as MIDI input device
4. Press **SHIFT (GP22) + F2** to start sequencer
5. You should see MIDI notes and clock!

### Test in Sequencer Page

1. Press **F2** to switch to Sequencer page
2. Use **LEFT/RIGHT arrows** to move between steps
3. Use **UP/DOWN** to change note pitch
4. Each step sends MIDI notes when playing

---

## Step 10: Explore Features

### Switch Pages

Press **F1** or **F2** repeatedly to cycle through:

1. **System Page**: BPM, clock source, track selection
2. **Sequencer Page**: Edit step patterns
3. **Generative Page**: Auto-generate patterns
4. **Pattern Page**: Save/load patterns
5. **MIDI Page**: MIDI CC mapping

### Create Your First Pattern

1. Go to **Sequencer Page** (press F2 until you see grid)
2. Use **LEFT/RIGHT** to navigate steps
3. Use **UP/DOWN** to set note pitch
4. Press **Play** (SHIFT + F2)
5. Hear/see your pattern play!

### Generate Random Pattern

1. Go to **Generative Page**
2. Adjust **Fill** (pattern density) with +/-
3. Press **F1** to generate
4. Go back to **Sequencer Page** to see result

---

## Troubleshooting

### OLED is blank

- ✅ Check I2C wiring (SDA, SCL, VCC, GND)
- ✅ Try different display address (edit `pico_oled.h`, line 103):
  ```cpp
  // Try this if display doesn't work:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
  ```

### Compilation errors

- ✅ Verify all libraries installed (Step 3)
- ✅ Check USB Stack set to **"Adafruit TinyUSB"**
- ✅ Update RP2040 board package (Tools → Board Manager)
- ✅ Clone with `--recursive` flag for submodules

### USB MIDI not showing up

- ✅ USB Stack must be **"Adafruit TinyUSB"** (not Pico SDK)
- ✅ Install Adafruit TinyUSB library
- ✅ Try different USB cable (must support data)
- ✅ Restart computer after first upload

### Display is upside down

- ✅ Edit `pico_oled.h`, uncomment line 23:
  ```cpp
  #define FLIP_DISPLAY
  ```

### Buttons don't work

- ✅ Buttons must connect GPIO to **GND** (not 3.3V)
- ✅ Internal pull-ups are enabled automatically
- ✅ Test with multimeter: should read 3.3V when open, 0V when pressed

---

## Next Steps

### Add Full Hardware

1. **Add all 9 buttons** (see README.md pinout)
2. **Add potentiometer** (GP26) for smooth parameter control
3. **Add MIDI DIN connectors** for hardware MIDI out
4. **Add I2S DAC** for audio synthesis (optional)

### Build Enclosure

- 3D print case (modify Hugo Escalpelo design)
- Laser-cut acrylic panel
- DIY cardboard prototype

### Advanced Features

- **Multi-track sequencing**: 4× TB-303 + 1× TR-808
- **Euclidean patterns**: Generative rhythms
- **MIDI CC mapping**: Control external synths
- **Pattern chaining**: Create songs
- **External MIDI clock**: Sync with other gear

---

## Learning Resources

### Understanding the Interface

- **System Page**: Global settings (BPM, clock)
- **Sequencer Page**: Step-by-step note editing
- **Generative Page**: Algorithmic pattern creation
- **Pattern Page**: Storage and recall
- **MIDI Page**: External control mapping

### TB-303 vs TR-808 Tracks

- **303 Tracks** (0-3): Bassline sequencer, 16 steps, note + accent/slide/tie
- **808 Track** (4): Drum sequencer, 64 steps, 11 voices (kick, snare, etc.)

### MIDI Mapping

Navigate to **MIDI Page** to assign pots/encoders to:
- Track parameters (filter, resonance, decay)
- Generative settings (fill, accent, slide)
- Global controls (BPM, shuffle)

---

## Community & Support

- **GitHub Issues**: https://github.com/midilab/aciduino/issues
- **Midilab Website**: https://midilab.co
- **RP2040 Forums**: https://forums.raspberrypi.com

---

## Tips for Success

1. **Start simple**: Get OLED + USB MIDI working first
2. **Test incrementally**: Add one feature at a time
3. **Use breadboard**: Prototype before soldering
4. **Check connections**: Most issues are wiring mistakes
5. **Read error messages**: Compilation errors usually tell you what's missing
6. **Use USB power**: 5V 1A is enough for everything except high-power audio amps

---

**You're ready to make some acid! 🎹🔥**

Happy sequencing!

---

*Last updated: January 2026*
*For detailed hardware info, see README.md*
*For component shopping list, see BOM.md*
