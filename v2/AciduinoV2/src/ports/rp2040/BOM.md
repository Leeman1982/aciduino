# Aciduino RP2040 - Bill of Materials (BOM)

## Core Components (Required)

| Qty | Component | Specification | Example Part # | Approx Cost (USD) |
|-----|-----------|---------------|----------------|-------------------|
| 1 | Raspberry Pi Pico | RP2040, headers pre-soldered | Raspberry Pi Pico | $4-6 |
| 1 | OLED Display | 1.3" or 2.4", SH1106/SSD1306, 128x64, I2C | 1.3" SH1106 | $5-8 |
| 9 | Push Buttons | 6mm×6mm tactile, momentary NO | MJTP1230 | $2-3 |
| 1 | Potentiometer | 10kΩ linear, 16mm rotary | B10K | $1-2 |
| 1 | MIDI DIN Connector | 5-pin female, PCB mount | CUI SD-50BV | $1-2 |
| 2 | Resistor 220Ω | 1/4W, through-hole | - | $0.10 |
| 1 | Breadboard or PCB | 830 tie-points breadboard OR custom PCB | - | $3-5 |
| 1 | USB Cable | Micro-USB data cable | - | $2-3 |

**Total Core Components**: ~$20-30

---

## Optional Components

### MIDI Input Circuit (Recommended)

| Qty | Component | Specification | Example Part # | Approx Cost (USD) |
|-----|-----------|---------------|----------------|-------------------|
| 1 | Optocoupler | 6N138 or similar | 6N138 | $0.50 |
| 1 | Resistor 220Ω | 1/4W | - | $0.05 |
| 1 | Resistor 10kΩ | 1/4W | - | $0.05 |
| 1 | MIDI DIN Connector | 5-pin female (input) | CUI SD-50BV | $1-2 |

**MIDI Input Total**: ~$2-3

---

### I2S Audio Output (High Quality)

| Qty | Component | Specification | Example Part # | Approx Cost (USD) |
|-----|-----------|---------------|----------------|-------------------|
| 1 | I2S DAC Module | PCM5102A breakout board | GY-PCM5102 | $3-5 |
| 1 | Audio Jack | 3.5mm stereo jack, PCB mount | PJ-320A | $0.50 |
| 2 | Capacitor | 100µF electrolytic, 16V | - | $0.20 |

**I2S Audio Total**: ~$4-6

---

### PWM Audio Output (Budget Option)

| Qty | Component | Specification | Example Part # | Approx Cost (USD) |
|-----|-----------|---------------|----------------|-------------------|
| 1 | Resistor 1kΩ | 1/4W | - | $0.05 |
| 1 | Capacitor | 10µF electrolytic, 16V | - | $0.10 |
| 1 | Audio Jack | 3.5mm stereo jack | PJ-320A | $0.50 |

**PWM Audio Total**: ~$0.65

---

### Rotary Encoder (Alternative to Pot)

| Qty | Component | Specification | Example Part # | Approx Cost (USD) |
|-----|-----------|---------------|----------------|-------------------|
| 1 | Rotary Encoder | Incremental, 20-30 PPR | EC11 | $1-2 |

---

### Additional MIDI CC Pots

| Qty | Component | Specification | Example Part # | Approx Cost (USD) |
|-----|-----------|---------------|----------------|-------------------|
| 3 | Potentiometer | 10kΩ linear, 16mm rotary | B10K | $3-6 |

---

### Enclosure Components

| Qty | Component | Specification | Approx Cost (USD) |
|-----|-----------|---------------|-------------------|
| 1 | Enclosure Box | ABS plastic, ~150×100×50mm | $5-10 |
| 4 | Rubber Feet | Self-adhesive | $1-2 |
| 10 | M3 Screws | 10mm length, for mounting | $1-2 |
| 10 | M3 Standoffs | 10mm length, for PCB | $2-3 |

**Enclosure Total**: ~$9-17

---

### Power Supply

| Qty | Component | Specification | Approx Cost (USD) |
|-----|-----------|---------------|-------------------|
| 1 | USB Power Adapter | 5V 1A, Micro-USB | $3-5 |

---

## Tools Required

- Soldering iron (25W-60W)
- Solder (lead-free or 60/40)
- Wire strippers
- Small screwdriver set
- Multimeter (for testing)
- Flush cutters
- Helping hands/PCB holder

---

## Wire & Connectors

| Qty | Component | Specification | Approx Cost (USD) |
|-----|-----------|---------------|-------------------|
| 1 | Hookup Wire Kit | 22-24 AWG, multiple colors | $5-8 |
| 10 | Female Header Pins | 2.54mm pitch, for Pico | $1-2 |
| 1 | Heat Shrink Tubing | Assorted sizes | $2-3 |

---

## Total Project Cost

| Configuration | Estimated Cost |
|---------------|----------------|
| **Minimal** (no audio, breadboard) | $22-33 |
| **Standard** (MIDI RX, PWM audio, breadboard) | $25-37 |
| **Premium** (MIDI RX, I2S audio, custom PCB, enclosure) | $45-70 |
| **Deluxe** (all options, 3D printed case) | $55-90 |

---

## Purchasing Tips

### Where to Buy

- **Electronics**: Mouser, DigiKey, LCSC, AliExpress
- **Raspberry Pi Pico**: Official distributors, Adafruit, SparkFun
- **OLED Displays**: Amazon, eBay, AliExpress (cheaper)
- **Enclosures**: Hammond Manufacturing, Bud Industries, or 3D print

### Cost Saving Options

1. **Use breadboard initially**: Test before committing to PCB
2. **Skip audio output**: Focus on MIDI sequencing first
3. **Bulk purchase**: Buy resistors/caps in packs
4. **AliExpress**: Cheaper for non-critical components (OLED, buttons)
5. **Generic MIDI connectors**: Don't need brand name for DIN jacks

### Quality Recommendations

- **Don't cheap out on**: Raspberry Pi Pico (get genuine), buttons (tactile feel)
- **Can save on**: Resistors, wire, connectors
- **OLED quality varies**: SH1106 generally better than SSD1306 for this project

---

## Alternative Components

### Display Options

| Type | Resolution | Size | Notes | Cost |
|------|------------|------|-------|------|
| SH1106 | 128×64 | 1.3" | Best contrast, good viewing angle | $5-8 |
| SSD1306 | 128×64 | 0.96" | Smaller, cheaper | $3-5 |
| SSD1306 | 128×64 | 2.4" | Larger, easier to read | $8-12 |

**Recommendation**: 1.3" SH1106 (best price/performance)

### Button Options

| Type | Size | Notes | Cost (ea) |
|------|------|-------|-----------|
| Tactile PCB | 6×6mm | Compact, breadboard-friendly | $0.20 |
| Arcade | 24mm | Retro feel, requires panel mount | $1-2 |
| Low-profile tactile | 12×12mm | Better tactile feedback | $0.30 |

**Recommendation**: 6×6mm tactile (standard, cheap, reliable)

### Pico Alternatives (RP2040 Compatible)

| Board | Features | Cost |
|-------|----------|------|
| Raspberry Pi Pico | Standard, 26 GPIO | $4 |
| Pico W | WiFi/Bluetooth | $6 |
| Adafruit Feather RP2040 | USB-C, more pins | $10 |
| Seeed XIAO RP2040 | Ultra-compact | $7 |

**Recommendation**: Standard Pico (unless you need WiFi)

---

## Compatibility Notes

- **I2C OLED**: Both SH1106 and SSD1306 work (U8g2 library supports both)
- **Button types**: Any momentary normally-open (NO) switch works
- **Potentiometers**: Linear (B) taper recommended, logarithmic (A) works too
- **MIDI connectors**: 5-pin DIN (standard), can use TRS MIDI with adapter
- **USB cables**: Must support data (charge-only cables won't work)

---

## PCB Design Files

If you'd like to create a custom PCB instead of breadboard:

- KiCad design files (coming soon)
- Gerber files for ordering from JLCPCB/PCBWay
- SMD version available for compact design

---

## 3D Printing Files

Enclosure STL files compatible with this build:

- Modified Hugo Escalpelo design for RP2040
- Panel mount version (desktop style)
- Portable version with battery compartment (requires LiPo + charger)

---

*Prices and availability subject to change. Check current prices from your preferred supplier.*

**Last updated**: January 2026
