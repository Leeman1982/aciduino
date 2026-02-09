# OLED Display Setup Guide for RP2040
**A comprehensive guide to using I2C OLED displays with Raspberry Pi Pico**

This guide explains how the OLED display is configured in Aciduino and how you can apply this to your own projects.

---

## Table of Contents

1. [Hardware Setup](#hardware-setup)
2. [Software Setup](#software-setup)
3. [Basic Display Initialization](#basic-display-initialization)
4. [Drawing to the Display](#drawing-to-the-display)
5. [Advanced Features](#advanced-features)
6. [Complete Examples](#complete-examples)
7. [Troubleshooting](#troubleshooting)

---

## Hardware Setup

### Supported OLED Displays

The U8g2 library supports many I2C OLED displays:

| Controller | Resolution | Common Sizes | I2C Address |
|------------|------------|--------------|-------------|
| **SH1106** | 128×64 | 0.96", 1.3", 2.4" | 0x3C or 0x3D |
| **SSD1306** | 128×64 | 0.96", 1.3" | 0x3C or 0x3D |
| **SSD1306** | 128×32 | 0.91" | 0x3C or 0x3D |
| **SH1107** | 128×128 | 1.3" | 0x3C |
| **SSD1309** | 128×64 | 2.42" | 0x3C or 0x3D |

**Recommendation**: SH1106 1.3" has the best contrast and viewing angles for most projects.

### Wiring

#### Standard I2C Connection (I2C0 on RP2040)

```
OLED Display Pin → Raspberry Pi Pico Pin
───────────────────────────────────────────
VCC   →  3.3V (Pin 36)
GND   →  GND  (Pin 38 or any GND)
SDA   →  GP4  (Pin 6)  - I2C0 SDA
SCL   →  GP5  (Pin 7)  - I2C0 SCL
```

#### Alternative I2C Pins (I2C1 on RP2040)

The RP2040 has flexible GPIO mapping. You can use I2C1 on different pins:

```
SDA   →  GP6, GP10, GP14, GP18, or GP26
SCL   →  GP7, GP11, GP15, GP19, or GP27
```

**Note**: For I2C1, you'll need to modify the initialization code (see Advanced section).

#### Pull-up Resistors

Most OLED modules have built-in pull-up resistors (typically 10kΩ). If your display doesn't work:
- Add external 4.7kΩ pull-up resistors from SDA to 3.3V
- Add external 4.7kΩ pull-up resistors from SCL to 3.3V

#### Power Consumption

- **Typical current**: 15-30mA (all pixels on)
- **Low power mode**: 5-10mA
- Safe to power directly from Pico's 3.3V pin

---

## Software Setup

### Required Library

**U8g2** by oliver (olikraus)
- **Installation**: Arduino IDE → Sketch → Include Library → Manage Libraries → Search "U8g2"
- **GitHub**: https://github.com/olikraus/u8g2
- **Documentation**: https://github.com/olikraus/u8g2/wiki

### Include in Your Sketch

```cpp
#include <U8g2lib.h>
#include <Wire.h>  // I2C library (built-in)
```

---

## Basic Display Initialization

### Step 1: Create Display Object

The display object must be created as a **global variable** (outside functions):

```cpp
// For SH1106 displays (128×64, 1.3" or 2.4")
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// For SSD1306 displays (128×64, 0.96" or 1.3")
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// For SSD1306 displays (128×32, 0.91")
// U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
```

### Understanding the Constructor Parameters

```cpp
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(rotation, reset_pin);
                                        ↑         ↑
                                        |         └─ Reset pin (U8X8_PIN_NONE = no reset)
                                        └─ Display rotation
```

**Constructor breakdown**:
- `U8G2` = Graphics library (full buffer)
- `SH1106` = Display controller chip
- `128X64` = Resolution
- `NONAME` = Generic display (no specific manufacturer)
- `F` = Full buffer mode (uses more RAM but faster)
- `HW_I2C` = Hardware I2C (uses Wire library)

**Rotation options**:
```cpp
U8G2_R0   // 0°   - normal orientation
U8G2_R1   // 90°  - rotated clockwise
U8G2_R2   // 180° - upside down
U8G2_R3   // 270° - rotated counter-clockwise
```

**Buffer modes**:
- `_F_` = Full buffer (fastest, uses ~1KB RAM for 128×64)
- `_1_` = Page buffer (slower, uses ~128 bytes RAM)
- `_2_` = 2-page buffer (compromise)

### Step 2: Initialize Display in setup()

```cpp
void setup() {
  // Initialize U8g2 library
  u8g2.begin();

  // Optional: Set I2C clock speed (default 100kHz)
  // Wire.setClock(400000);  // 400kHz for faster updates

  // Clear the display
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}
```

### Step 3: Draw Something

```cpp
void loop() {
  u8g2.clearBuffer();                   // Clear the internal buffer
  u8g2.setFont(u8g2_font_ncenB08_tr);   // Choose a font
  u8g2.drawStr(0, 15, "Hello World!");  // Write text at (x, y)
  u8g2.sendBuffer();                    // Transfer buffer to display

  delay(1000);
}
```

---

## Drawing to the Display

### Understanding the Coordinate System

```
(0,0) ────────────────────────────→ X (127)
  │
  │     Your drawing area
  │     128 pixels wide
  │     64 pixels tall
  │
  ↓
  Y
 (63)
```

**Important**: Text Y coordinate is the **baseline**, not the top!

### Basic Drawing Functions

#### Text

```cpp
// Draw string at position
u8g2.drawStr(x, y, "Hello");

// Draw string with printf formatting
u8g2.setCursor(x, y);
u8g2.print("Value: ");
u8g2.print(123);

// Centered text
int width = u8g2.getStrWidth("Text");
int x = (128 - width) / 2;
u8g2.drawStr(x, 32, "Text");
```

#### Shapes

```cpp
// Pixel
u8g2.drawPixel(x, y);

// Line
u8g2.drawLine(x0, y0, x1, y1);

// Rectangle (outline)
u8g2.drawFrame(x, y, width, height);

// Rectangle (filled)
u8g2.drawBox(x, y, width, height);

// Circle (outline)
u8g2.drawCircle(x, y, radius);

// Circle (filled)
u8g2.drawDisc(x, y, radius);

// Triangle
u8g2.drawTriangle(x0, y0, x1, y1, x2, y2);
```

#### Bitmaps

```cpp
// XBM format (C array)
static const unsigned char logo_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

u8g2.drawXBM(x, y, width, height, logo_bits);
```

### Font Selection

```cpp
// Small fonts (6-8 pixels high)
u8g2.setFont(u8g2_font_6x10_tr);      // 6×10 pixels
u8g2.setFont(u8g2_font_ncenB08_tr);   // 8 pixels high

// Medium fonts (10-14 pixels high)
u8g2.setFont(u8g2_font_ncenB10_tr);   // 10 pixels
u8g2.setFont(u8g2_font_ncenB14_tr);   // 14 pixels

// Large fonts (18-24 pixels high)
u8g2.setFont(u8g2_font_ncenB18_tr);   // 18 pixels
u8g2.setFont(u8g2_font_ncenB24_tr);   // 24 pixels

// Monospace fonts (fixed width)
u8g2.setFont(u8g2_font_7x13_tr);
u8g2.setFont(u8g2_font_10x20_tr);
```

**Font naming convention**:
- `u8g2_font_` = prefix
- `ncenB` = font name (New Century Schoolbook Bold)
- `08` = height in pixels
- `_tr` = transparent background, reduced character set (ASCII)
- `_tf` = transparent background, full character set
- `_mr` = monochrome background, reduced
- `_mf` = monochrome background, full

**Browse all fonts**: https://github.com/olikraus/u8g2/wiki/fntlistall

### Drawing Workflow

**Full buffer mode** (what Aciduino uses):

```cpp
void loop() {
  // 1. Clear the buffer (in RAM)
  u8g2.clearBuffer();

  // 2. Draw everything to buffer
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 15, "Line 1");
  u8g2.drawStr(0, 30, "Line 2");
  u8g2.drawFrame(0, 40, 50, 20);

  // 3. Send buffer to display (single I2C transaction)
  u8g2.sendBuffer();

  delay(100);
}
```

**Page buffer mode** (lower RAM usage):

```cpp
void loop() {
  u8g2.firstPage();
  do {
    // Drawing code here (called multiple times for each page)
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Line 1");
  } while (u8g2.nextPage());

  delay(100);
}
```

---

## Advanced Features

### Flipping the Display

```cpp
void setup() {
  u8g2.begin();
  u8g2.setFlipMode(1);  // 1 = flip, 0 = normal
}
```

**Or** use rotation in constructor:
```cpp
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);  // 180° rotation
```

### Contrast Control

```cpp
u8g2.setContrast(255);  // 0-255, default is usually 127
```

### Power Save Mode

```cpp
u8g2.setPowerSave(1);  // 1 = sleep, 0 = wake up
```

### Using Different I2C Pins (Software I2C)

If you need to use non-standard I2C pins:

```cpp
// Software I2C constructor
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

// Example with custom pins
#define OLED_SCL  10
#define OLED_SDA  11

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);
```

**Note**: Software I2C is slower but more flexible.

### Custom I2C Address

Most displays use `0x3C`, but if yours uses `0x3D`:

```cpp
void setup() {
  u8g2.begin();
  u8g2.setI2CAddress(0x3D << 1);  // Shift left by 1 for 8-bit address
}
```

### Drawing Speed Optimization

```cpp
// Increase I2C speed (default 100kHz)
Wire.setClock(400000);  // 400kHz - works with most displays

// Use partial updates (only for page buffer mode)
u8g2.setPartialWindow(x, y, width, height);
```

---

## Complete Examples

### Example 1: Simple Text Display

```cpp
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  u8g2.begin();
  Wire.setClock(400000);  // Fast I2C
}

void loop() {
  u8g2.clearBuffer();

  // Title
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(25, 20, "RP2040");

  // Message
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 40, "Hello World!");

  // Counter
  static int count = 0;
  u8g2.setCursor(10, 60);
  u8g2.print("Count: ");
  u8g2.print(count++);

  u8g2.sendBuffer();
  delay(100);
}
```

### Example 2: Analog Sensor Display

```cpp
#include <U8g2lib.h>
#include <Wire.h>

#define POT_PIN 26  // ADC0

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  u8g2.begin();
  Wire.setClock(400000);
}

void loop() {
  int raw = analogRead(POT_PIN);
  int percent = map(raw, 0, 1023, 0, 100);

  u8g2.clearBuffer();

  // Title
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "Potentiometer");

  // Value
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(0, 30);
  u8g2.print(percent);
  u8g2.print("%");

  // Progress bar
  u8g2.drawFrame(0, 40, 128, 15);
  int barWidth = map(percent, 0, 100, 0, 126);
  u8g2.drawBox(1, 41, barWidth, 13);

  u8g2.sendBuffer();
  delay(50);
}
```

### Example 3: Animated Graphics

```cpp
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int ballX = 64;
int ballY = 32;
int velX = 2;
int velY = 1;

void setup() {
  u8g2.begin();
  Wire.setClock(400000);
}

void loop() {
  // Update ball position
  ballX += velX;
  ballY += velY;

  // Bounce off edges
  if (ballX <= 5 || ballX >= 123) velX = -velX;
  if (ballY <= 5 || ballY >= 59) velY = -velY;

  // Draw
  u8g2.clearBuffer();
  u8g2.drawCircle(ballX, ballY, 5);
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.sendBuffer();

  delay(20);  // ~50 FPS
}
```

### Example 4: Multi-Page Menu

```cpp
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

const char* menuItems[] = {
  "System Info",
  "Settings",
  "About",
  "Exit"
};
int selectedItem = 0;
int numItems = 4;

void setup() {
  u8g2.begin();
}

void loop() {
  u8g2.clearBuffer();

  // Title
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(30, 10, "Main Menu");
  u8g2.drawLine(0, 12, 128, 12);

  // Menu items
  for (int i = 0; i < numItems; i++) {
    int y = 25 + (i * 12);

    // Highlight selected item
    if (i == selectedItem) {
      u8g2.drawBox(0, y - 9, 128, 11);
      u8g2.setDrawColor(0);  // Black text on white
    }

    u8g2.drawStr(5, y, menuItems[i]);
    u8g2.setDrawColor(1);  // Reset to white text
  }

  u8g2.sendBuffer();

  // Simulate navigation (you'd use buttons)
  delay(1000);
  selectedItem = (selectedItem + 1) % numItems;
}
```

### Example 5: Real-Time Clock Display

```cpp
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  u8g2.begin();
}

void loop() {
  unsigned long seconds = millis() / 1000;
  int hours = (seconds / 3600) % 24;
  int minutes = (seconds / 60) % 60;
  int secs = seconds % 60;

  u8g2.clearBuffer();

  // Large clock display
  u8g2.setFont(u8g2_font_ncenB24_tr);
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, secs);

  int width = u8g2.getStrWidth(timeStr);
  u8g2.drawStr((128 - width) / 2, 40, timeStr);

  u8g2.sendBuffer();
  delay(100);
}
```

---

## How Aciduino Uses the OLED

### 1. Object Creation (Global)

From `pico_oled.h`:
```cpp
// Line 103
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
```

### 2. Initialization in initPort()

From `pico_oled.h` line 266:
```cpp
void initPort() {
  // OLED setup
  uCtrl.initOled(&u8g2);

  #if defined(FLIP_DISPLAY)
    uCtrl.oled->flipDisplay(1);
  #endif

  // ... rest of initialization
}
```

### 3. Drawing Through uCtrl Abstraction

Aciduino uses the **uCtrl library** as an abstraction layer. This allows the same UI code to work across different platforms. The uCtrl library internally uses the u8g2 object.

**Page rendering** happens in the interface components:
- **Topbar**: `src/interface/midilab/components/topbar.hpp`
- **Step grid**: `src/interface/midilab/components/sequencer_step_grid.hpp`
- **Pattern grid**: `src/interface/midilab/components/pattern_grid.hpp`

Each component draws to the u8g2 buffer, then the main loop calls `sendBuffer()`.

### 4. Display Update Loop

The display is updated continuously in the main loop:

```cpp
void loop() {
  aciduino.run();  // This internally handles UI refresh
}
```

Inside `aciduino.run()`, the UI components are redrawn at a controlled rate.

---

## Troubleshooting

### Display Not Working

**Problem**: Display stays blank

**Solutions**:
1. Check wiring (especially SDA/SCL swap)
2. Try different I2C address:
   ```cpp
   u8g2.setI2CAddress(0x3D << 1);
   ```
3. Test with I2C scanner:
   ```cpp
   #include <Wire.h>

   void setup() {
     Serial.begin(115200);
     Wire.begin();

     Serial.println("Scanning I2C...");
     for (byte addr = 1; addr < 127; addr++) {
       Wire.beginTransmission(addr);
       if (Wire.endTransmission() == 0) {
         Serial.print("Found device at 0x");
         Serial.println(addr, HEX);
       }
     }
   }
   ```

### Garbled Display

**Problem**: Random pixels, corrupted text

**Solutions**:
1. Add pull-up resistors (4.7kΩ) on SDA and SCL
2. Reduce I2C speed:
   ```cpp
   Wire.setClock(100000);  // Default 100kHz
   ```
3. Shorten wires (I2C max ~1 meter)
4. Try wrong controller type:
   ```cpp
   // If you have SH1106 but tried SSD1306, or vice versa
   U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
   ```

### Slow Updates

**Problem**: Display flickers or updates slowly

**Solutions**:
1. Increase I2C speed:
   ```cpp
   Wire.setClock(400000);
   ```
2. Use full buffer mode (`_F_` constructor)
3. Reduce drawing complexity
4. Call `sendBuffer()` less frequently

### Display Upside Down

**Problem**: Display orientation wrong

**Solutions**:
1. Use rotation in constructor:
   ```cpp
   U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);
   ```
2. Or use flip mode:
   ```cpp
   u8g2.setFlipMode(1);
   ```

### Text Cutoff

**Problem**: Text appears cut off at top or bottom

**Solutions**:
- Y coordinate is the baseline, not top left
- Add font height to Y position:
  ```cpp
  // Wrong
  u8g2.drawStr(0, 0, "Text");  // Cutoff!

  // Right
  u8g2.drawStr(0, 10, "Text");  // Visible
  ```

### Out of Memory

**Problem**: Sketch won't compile, "not enough RAM"

**Solutions**:
1. Use page buffer mode instead of full buffer:
   ```cpp
   // Change _F_ to _1_
   U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
   ```
2. Reduce number of fonts included
3. Use smaller bitmaps

---

## Memory Usage

### RAM Comparison (128×64 display)

| Buffer Mode | RAM Usage | Speed | Use Case |
|-------------|-----------|-------|----------|
| Full (`_F_`) | ~1024 bytes | Fast | RP2040, ESP32 (lots of RAM) |
| Page (`_1_`) | ~128 bytes | Slow | Arduino Uno, Nano (limited RAM) |
| 2-Page (`_2_`) | ~256 bytes | Medium | Compromise |

**RP2040 has 264KB RAM** → Full buffer is recommended for best performance.

---

## Additional Resources

- **U8g2 Wiki**: https://github.com/olikraus/u8g2/wiki
- **Font List**: https://github.com/olikraus/u8g2/wiki/fntlistall
- **API Reference**: https://github.com/olikraus/u8g2/wiki/u8g2reference
- **Examples**: Arduino IDE → File → Examples → U8g2
- **Datasheet SH1106**: https://www.velleman.eu/downloads/29/infosheets/sh1106_datasheet.pdf
- **Datasheet SSD1306**: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

---

## Summary: Key Takeaways

1. **Include library**: `#include <U8g2lib.h>`
2. **Create object globally**: `U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(...)`
3. **Initialize in setup()**: `u8g2.begin()`
4. **Drawing pattern**:
   ```cpp
   u8g2.clearBuffer();    // Clear
   // ... draw stuff ...    // Draw
   u8g2.sendBuffer();      // Display
   ```
5. **Common pins**: SDA=GP4, SCL=GP5 (I2C0 on Pico)
6. **Speed up**: `Wire.setClock(400000)` for 400kHz I2C
7. **Troubleshoot**: Check wiring, try different controller type, verify I2C address

---

**Now you can use OLED displays in any RP2040 project!**

Happy coding! 📟✨
