# Aciduino V2 - RP2040 Port (Raspberry Pi Pico)

**Roland TB-303 and TR-808 Step Sequencer Clone for RP2040**

This port brings Aciduino to the powerful dual-core RP2040 microcontroller (Raspberry Pi Pico) with OLED display support, MIDI RX/TX, and optional audio output via Arduino Audio Tools.

## Features

- ✅ **Dual-Core Processing**: Core 0 handles UI, Core 1 handles timing-critical sequencer tasks
- ✅ **OLED Display**: 1.3" or 2.4" SH1106 128x64 I2C display
- ✅ **MIDI RX/TX**: Full MIDI input and output via USB and hardware UART
- ✅ **9 Navigation Buttons**: Full sequencer control
- ✅ **Rotary Encoder or Pot**: Parameter navigation
- ✅ **Optional Audio Output**: I2S DAC or PWM audio via Arduino Audio Tools
- ✅ **MIDI CC Mapping**: Map performance parameters to MIDI CC
- ✅ **Pattern Storage**: EEPROM pattern saving
- ✅ **Generative Sequencing**: Euclidean algorithms and harmonizer

## Hardware Requirements

### Core Components

1. **Raspberry Pi Pico** (or compatible RP2040 board)
2. **1.3" or 2.4" OLED Display** (SH1106 or SSD1306, 128x64, I2C)
3. **9 Push Buttons** (momentary, NO)
4. **1 Rotary Potentiometer** (10kΩ linear) OR 1 Rotary Encoder
5. **MIDI DIN Connector** (5-pin female) for hardware MIDI
6. **Resistors**: 220Ω (x2), 10kΩ pull-up resistors for I2C if needed

### Optional Components

- **I2S DAC Module** (PCM5102A or similar) for high-quality audio
- **4 Additional Potentiometers** (10kΩ) for MIDI CC control
- **LED** for BPM indicator (built-in LED on GPIO 25)

## Pin Connections

### OLED Display (I2C)

| OLED Pin | Pico Pin | Description |
|----------|----------|-------------|
| VCC      | 3.3V     | Power       |
| GND      | GND      | Ground      |
| SDA      | GP4      | I2C Data    |
| SCL      | GP5      | I2C Clock   |

### Navigation Buttons

Connect one side of each button to the specified GPIO pin, and the other side to **GND**. Internal pull-up resistors are enabled in software.

| Button Function | Pico Pin | Description |
|----------------|----------|-------------|
| SHIFT          | GP22     | Shift modifier |
| FUNCTION 1 (F1)| GP18     | Page function 1 |
| FUNCTION 2 (F2)| GP19     | Page function 2 |
| DECREMENT (-)  | GP16     | Decrease value |
| INCREMENT (+)  | GP17     | Increase value |
| RIGHT (→)      | GP15     | Next parameter |
| UP (↑)         | GP14     | Navigate up |
| DOWN (↓)       | GP13     | Navigate down |
| LEFT (←)       | GP12     | Previous parameter |
| TRANSPORT (optional) | GP11 | Play/Stop |

### Changer Potentiometer (Alternative to Encoder)

| Pot Pin | Pico Pin | Description |
|---------|----------|-------------|
| Pin 1   | GND      | Ground      |
| Pin 2 (Wiper) | GP26 (ADC0) | Analog input |
| Pin 3   | 3.3V     | Power       |

### Rotary Encoder (Alternative to Pot)

| Encoder Pin | Pico Pin | Description |
|-------------|----------|-------------|
| CLK         | GP20     | Encoder DEC |
| DT          | GP21     | Encoder INC |
| SW          | -        | Not used    |
| GND         | GND      | Ground      |
| VCC         | 3.3V     | Power       |

**Note**: To use encoder instead of pot, uncomment `#define USE_CHANGER_ENCODER` in `pico_oled.h`

### MIDI Hardware Interface (UART)

**MIDI OUT Circuit:**

```
Pico GP0 (TX) ---[220Ω]---> MIDI DIN Pin 5
                      |
                      +--[220Ω]---> MIDI DIN Pin 4
                      |
Pico 3.3V ------------+

MIDI DIN Pin 2 ----> GND (Shield)
```

**MIDI IN Circuit with Optocoupler (recommended):**

```
MIDI DIN Pin 5 ---[220Ω]---> 6N138 Pin 2 (Anode)
MIDI DIN Pin 4 -------------> 6N138 Pin 3 (Cathode)

6N138 Pin 6 (Collector) ---> Pico GP1 (RX)
6N138 Pin 5 (Emitter) -----> GND
6N138 Pin 8 (VCC) ---------> Pico 3.3V
6N138 Pin 7 ---------------> [10kΩ] --> 3.3V
```

**MIDI Pinout:**
- Pin 2: Shield/Ground
- Pin 4: MIDI Current Source (220Ω to +5V)
- Pin 5: MIDI Signal (220Ω resistor)

### Optional: I2S Audio Output

| I2S DAC Pin | Pico Pin | Description |
|-------------|----------|-------------|
| VCC         | 3.3V     | Power       |
| GND         | GND      | Ground      |
| BCK (BCLK)  | GP6      | Bit Clock   |
| LCK (LRCLK) | GP7      | Left/Right Clock |
| DIN (DOUT)  | GP8      | Audio Data  |
| SCK         | -        | Not used (optional) |

**To enable I2S audio**: Uncomment `#define USE_AUDIO_I2S` in `pico_oled.h`

### Optional: PWM Audio Output

| Component | Pico Pin | Description |
|-----------|----------|-------------|
| PWM Out   | GP9      | Audio signal |
| RC Filter | -        | 1kΩ + 10µF to GND for smoothing |

**To enable PWM audio**: Uncomment `#define USE_AUDIO_PWM` in `pico_oled.h`

### Optional: Additional MIDI CC Pots

| Pot # | Pico Pin | Description |
|-------|----------|-------------|
| Pot 1 | GP27 (ADC1) | MIDI CC 1 |
| Pot 2 | GP28 (ADC2) | MIDI CC 2 |
| Pot 3 | GP29 (ADC3) | MIDI CC 3 |

**To enable**: Uncomment `#define USE_POT_MICRO` in `pico_oled.h`

## Software Setup

### Arduino IDE Configuration

1. **Install RP2040 Board Support**:
   - Open Arduino IDE
   - Go to **File → Preferences**
   - Add to "Additional Board Manager URLs":
     ```
     https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
     ```
   - Go to **Tools → Board → Board Manager**
   - Search for "Pico" and install **"Raspberry Pi Pico/RP2040"** by Earle F. Philhower

2. **Install Required Libraries**:
   - **U8g2** (OLED display):
     - Go to **Sketch → Include Library → Manage Libraries**
     - Search "U8g2" and install
   - **MIDI Library**:
     - Search "MIDI Library" by Francois Best and install
   - **Adafruit TinyUSB** (for USB MIDI):
     - Search "Adafruit TinyUSB" and install
   - **Arduino Audio Tools** (optional, for audio output):
     - Search "Arduino Audio Tools" by Phil Schatzmann and install

3. **Clone Aciduino Repository**:
   ```bash
   git clone --recursive https://github.com/midilab/aciduino.git
   cd aciduino/v2/AciduinoV2
   ```

4. **Configure Board Settings**:
   - Go to **Tools → Board** → Select **"Raspberry Pi Pico"**
   - **Tools → USB Stack** → Select **"Adafruit TinyUSB"**
   - **Tools → CPU Speed** → Select **"133 MHz"** (overclock for better performance)
   - **Tools → Optimize** → Select **"Optimize Even More (-O3)"**
   - **Tools → Flash Size** → Select **"2MB (Sketch: 1MB, FS: 1MB)"**
   - **Tools → Port** → Select your Pico's COM port

5. **Open and Upload**:
   - Open `AciduinoV2.ino`
   - Verify the port is set to RP2040: `#include "src/ports/rp2040/pico_oled.h"`
   - Click **Upload** (hold BOOTSEL button on Pico if not recognized)

## Configuration Options

Edit `src/ports/rp2040/pico_oled.h` to customize:

### Navigation Method

**Potentiometer (default)**:
```cpp
#define USE_CHANGER_POT
//#define USE_CHANGER_ENCODER
```

**Rotary Encoder**:
```cpp
//#define USE_CHANGER_POT
#define USE_CHANGER_ENCODER
```

### MIDI Ports

```cpp
#define USE_MIDI1 // USB MIDI (native)
#define USE_MIDI2 // Hardware UART MIDI
//#define USE_MIDI3 // Optional second UART
```

### Display Orientation

```cpp
#define FLIP_DISPLAY  // Uncomment to flip display 180°
```

### Audio Output

**Enable I2S DAC**:
```cpp
#define USE_AUDIO_TOOLS
#define USE_AUDIO_I2S
```

**Enable PWM Audio**:
```cpp
#define USE_AUDIO_TOOLS
#define USE_AUDIO_PWM
```

### Additional MIDI CC Pots

```cpp
#define USE_POT_MICRO  // Enables 3 extra pots on ADC1-3
```

## Dual-Core Architecture

The RP2040 port uses both ARM Cortex-M0+ cores for optimal performance:

- **Core 0**: UI rendering, button handling, OLED display updates
- **Core 1**: Sequencer engine, MIDI timing, clock generation, note processing

This architecture ensures:
- **Tight MIDI timing** (no jitter from UI updates)
- **Responsive interface** (UI doesn't block sequencer)
- **Higher BPM capability** (up to 300 BPM stable)

## Performance Features Mapped to MIDI CC

The **MIDI Page** allows mapping performance parameters to MIDI CC:

1. Navigate to **MIDI Page** (press F2 repeatedly)
2. Select a parameter slot (0-15)
3. Adjust the parameter using the pot/encoder
4. The CC value is sent in real-time to the configured MIDI output

**Default mappable parameters**:
- Accent amount
- Slide amount
- Filter cutoff (if using external synth)
- Resonance
- Decay
- BPM (tempo control)
- Pattern fill amount
- Euclidean density

## Usage Guide

### Basic Operation

1. **Power on**: Pico boots and shows system page
2. **Page Navigation**: Press **F1** or **F2** to switch pages
   - Page 1: System (BPM, clock source)
   - Page 2: Sequencer (step editing)
   - Page 3: Generative (pattern generation)
   - Page 4: Pattern (save/load)
   - Page 5: MIDI (CC mapping)

3. **Track Selection**:
   - Hold **SHIFT + Increment (→)** for next track
   - Hold **SHIFT + Decrement (←)** for previous track

4. **Play/Stop**:
   - Press **SHIFT + F2** (or dedicated transport button if enabled)

5. **Record Mode**:
   - Press **SHIFT + SHIFT + F2** to enable/disable MIDI recording

### Step Sequencer Page

- **Navigate steps**: LEFT/RIGHT arrows
- **Change note**: UP/DOWN arrows
- **Toggle accent**: Press **SHIFT + UP**
- **Toggle slide**: Press **SHIFT + DOWN**
- **Toggle tie**: Press **INCREMENT**
- **Rest step**: Press **DECREMENT**

### Generative Page

- **Fill amount**: Controls pattern density (0-100%)
- **Accent probability**: Chance of accent per step (0-100%)
- **Slide probability**: Chance of slide per step (0-100%)
- **Number of tones**: Scale constraint (1-12 notes)
- **Octave range**: Pitch range (1-5 octaves)

Press **SHIFT + F1** to generate new pattern with current settings.

### Pattern Management

- **Save pattern**: Navigate to slot, press **SHIFT + INCREMENT**
- **Load pattern**: Navigate to slot, press **SHIFT + DECREMENT**
- **Copy pattern**: Select pattern, press **F1**
- **Paste pattern**: Select target, press **F2**

## MIDI Implementation

### MIDI Output

- **Note messages**: Sent on configured channel per track
- **Clock sync**: 24 PPQN MIDI clock output
- **Start/Stop**: MIDI transport messages
- **CC messages**: Performance parameters (page 5)

### MIDI Input

- **Note recording**: Records notes to current track
- **Clock sync**: External MIDI clock input (set clock source to External)
- **Transport**: Start/Stop messages from external device

## Troubleshooting

### Display not working

- Check I2C connections (SDA, SCL, VCC, GND)
- Verify display address (default 0x3C for SH1106)
- Try adding 4.7kΩ pull-up resistors on SDA/SCL lines

### Buttons not responding

- Verify all buttons connected to GND (not 3.3V)
- Check internal pull-ups are enabled in code
- Test individual GPIO pins with multimeter

### No MIDI output

- Check MIDI circuit (220Ω resistors required)
- Verify UART TX pin (GP0) is connected correctly
- Test with MIDI monitor software (USB MIDI)

### Audio issues

- I2S: Verify all connections (BCK, LRCLK, DOUT)
- PWM: Add RC filter (1kΩ + 10µF) for smoother output
- Check library installation (Arduino Audio Tools)

### USB MIDI not recognized

- Verify **USB Stack** set to **"Adafruit TinyUSB"**
- Install Adafruit TinyUSB library
- Check USB cable supports data (not charge-only)

## Performance Tips

1. **Overclock to 133 MHz**: Improves timing precision
2. **Use O3 optimization**: Faster code execution
3. **Enable dual-core**: Ensures sequencer priority
4. **Use USB MIDI for low latency**: Hardware UART has ~1ms latency
5. **Keep BPM ≤ 200**: For complex patterns with many tracks

## Enclosure Ideas

### 3D Printable Case

You can adapt the **Hugo Escalpelo ESP32 case designs** for RP2040:
- Located in: `v2/hardware/HugoEscalpelo-3DCase-*`
- Modify mounting holes for Raspberry Pi Pico dimensions
- Pico is smaller (21mm × 51mm) than ESP32 DevKit

### Desktop Panel

- Laser-cut acrylic or wood panel (150mm × 100mm)
- Mount OLED in center cutout
- 3×3 button grid layout
- Side-mounted potentiometer
- MIDI DIN and USB access cutouts

## Credits

- **Aciduino Project**: Romulo Silva (Midilab)
- **RP2040 Port**: Claude (Anthropic)
- **uClock Library**: Midilab
- **uCtrl Library**: Midilab
- **U8g2 Library**: Olikraus
- **MIDI Library**: Francois Best
- **Arduino-Pico**: Earle F. Philhower III

## License

MIT License - See main project LICENSE file

## Links

- **Aciduino GitHub**: https://github.com/midilab/aciduino
- **RP2040 Datasheet**: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
- **Arduino-Pico**: https://github.com/earlephilhower/arduino-pico
- **Midilab**: https://midilab.co

---

**Enjoy making acid! 🎹🎛️🔊**
