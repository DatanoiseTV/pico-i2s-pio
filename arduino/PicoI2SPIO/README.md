# PicoI2SPIO Arduino Library

Arduino wrapper for the pico-i2s-pio library, providing high-quality I2S audio output for Raspberry Pi Pico (RP2040/RP2350).

## Features

- **Multiple DAC Support**: PCM5102A, PT8211, AK449X, and generic I2S
- **High Quality Audio**: Low jitter mode with system clock optimization
- **Flexible Configuration**: 16/24/32-bit depth, 44.1kHz to 384kHz sample rates
- **MCLK Generation**: Automatic master clock generation at 256fs
- **Volume Control**: Per-channel volume with dB or percentage control
- **Dual Mono Mode**: Support for balanced audio configurations
- **Simple API**: Easy-to-use Arduino-style interface

## Installation

### Arduino IDE
1. Download this repository as a ZIP file
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps =
    https://github.com/DatanoiseTV/pico-i2s-pio.git#arduino
```

## Quick Start

```cpp
#include <PicoI2SPIO.h>

void setup() {
  // Initialize I2S with default settings (48kHz, 16-bit)
  I2S.begin();

  // Set volume to 75%
  I2S.setVolume(75);
}

void loop() {
  // Generate a sine wave
  int16_t sample = sin(millis() * 0.001) * 32767;

  // Write stereo sample
  I2S.writeStereo(sample, sample);
}
```

## Pin Connections

### Default Pins
| Signal | GPIO | DAC Pin |
|--------|------|---------|
| DATA   | 18   | DIN/SD  |
| LRCLK  | 20   | WS/LRCK |
| BCLK   | 21   | BCK/SCK |
| MCLK   | 22   | MCLK    |

### Custom Pins
```cpp
// Initialize with custom pins
I2S.begin(data_pin, clock_base_pin, mclk_pin, sample_rate, bit_depth);
```

## API Reference

### Basic Methods

#### `begin(sample_rate, bit_depth)`
Initialize I2S with default pins.
- `sample_rate`: 8000 to 384000 Hz (default: 48000)
- `bit_depth`: 16, 24, or 32 bits (default: 16)
- Returns: true on success

#### `write(samples, count)`
Write audio samples to I2S buffer.
- `samples`: Array of int16_t or int32_t samples
- `count`: Number of samples (not bytes)
- Returns: true if written, false if buffer full

#### `setVolume(volume)`
Set output volume.
- `volume`: 0-100 (percentage) or use `setVolumeDB()` for dB

#### `availableForWrite()`
Get number of bytes available in buffer.

### Advanced Methods

#### `beginAdvanced(...)`
Initialize with full control over all parameters.
```cpp
I2S.beginAdvanced(
  data_pin, clock_pin_base, mclk_pin,  // Pins
  pio, sm, dma_ch,                      // Hardware
  use_core1,                            // Multicore
  clock_mode,                           // Clock mode
  i2s_mode,                             // Output format
  sample_rate, bit_depth                // Audio params
);
```

#### Clock Modes
- `CLOCK_MODE_DEFAULT`: Standard mode with fractional divider
- `CLOCK_MODE_LOW_JITTER`: Low jitter mode (adjusts system clock)
- `CLOCK_MODE_LOW_JITTER_OC`: Overclocked low jitter mode
- `CLOCK_MODE_EXTERNAL`: External clock input

#### I2S Modes
- `MODE_I2S`: Standard I2S (PCM5102A, etc.)
- `MODE_PT8211`: PT8211 format (no MCLK)
- `MODE_EXDF`: AK449X EXDF format
- `MODE_I2S_DUAL`: Dual mono I2S
- `MODE_PT8211_DUAL`: Dual mono PT8211

## Examples

### Simple Tone Generator
```cpp
float phase = 0;
const float FREQ = 440.0;  // A4 note

void loop() {
  int16_t buffer[128];

  for (int i = 0; i < 64; i++) {
    int16_t sample = sin(phase) * 32767 * 0.5;
    buffer[i*2] = sample;      // Left
    buffer[i*2+1] = sample;    // Right
    phase += 2 * PI * FREQ / 48000;
  }

  I2S.write(buffer, 128);
}
```

### High Quality Audio Mode
```cpp
void setup() {
  // MUST initialize I2S before other peripherals!
  I2S.beginAdvanced(
    18, 20, 22,               // Pins
    pio0, 0, 0,               // Hardware
    false,                    // Single core
    CLOCK_MODE_LOW_JITTER,    // Low jitter!
    MODE_I2S,                 // Standard I2S
    96000, 24                 // 96kHz, 24-bit
  );

  // NOW initialize Serial, etc.
  Serial.begin(115200);
}
```

### PT8211 DAC Setup
```cpp
// PT8211 doesn't need MCLK
I2S.beginAdvanced(
  18, 20, 0,                  // No MCLK pin
  pio0, 0, 0, false,
  CLOCK_MODE_DEFAULT,
  MODE_PT8211,                // PT8211 mode
  48000, 16
);
```

## Supported DACs

### PCM5102A
- Standard I2S format
- Requires MCLK or internal PLL
- Supports up to 32-bit, 384kHz

### PT8211
- Simplified I2S (no MCLK)
- 16-bit only
- Very affordable

### AK449X Series
- EXDF mode support
- High-end audio quality
- Requires specific pin configuration

## Tips

1. **Low Jitter Mode**: Initialize I2S before any other peripherals
2. **Buffer Management**: Check `availableForWrite()` to prevent glitches
3. **Volume**: Start at lower volumes to prevent speaker damage
4. **Sample Format**: Match bit depth to your DAC capabilities

## License

MIT License - see LICENSE file for details