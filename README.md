# pico-i2s-pio
A library for outputting MCLK-compatible I2S using PIO on Raspberry Pi Pico. It sets the RP2040/RP2350 system clock to an integer multiple of MCLK and features a low jitter mode that doesn't use PIO's fractional division. It also includes functionality to operate non-differential output DACs like PCM5102A or PT8211 in dual mono mode.

EXDF mode compatible with AK449X has been added experimentally. Operation on actual hardware has not been confirmed.

External clock mode using 45.1584/49.152MHz external clock has been added experimentally. Please input 45.1584MHz to GPIO20 and 49.152MHz to GPIO22. Operation has not been confirmed.

## Supported Formats
16,24,32bit 44.1kHz～384kHz
### i2s
BCLK: 64fs
MCLK: 22.5792/24.576MHz

|name|pin|
|----|---|
|DATA|data_pin|
|LRCLK|clock_pin_base|
|BCLK|clock_pin_base+1|
|MCLK|mclk_pin|

### PT8211
BCLK: 32fs
MCLK: no

|name|pin|
|----|---|
|DATA|data_pin|
|LRCLK|clock_pin_base|
|BCLK|clock_pin_base+1|

### AK449X EXDF
BCK: 32fs
MCLK: 32fs (BCK)

|name|pin|
|----|---|
|DOUTL|data_pin|
|DOUTR|data_pin+1|
|WCK|clock_pin_base|
|BCK|clock_pin_base+1|
|MCLK|clock_pin_base+2|

### i2s dual
BCLK: 64fs
MCLK: 22.5792/24.576MHz
L channel of each DAC becomes positive, R channel becomes negative.

|name|pin|
|----|---|
|DATAL|data_pin|
|DATAR|data_pin + 1|
|LRCLK|clock_pin_base|
|BCLK|clock_pin_base+1|
|MCLK|mclk_pin|

### PT8211 dual
BCLK: 32fs
MCLK: no
L channel of each DAC becomes positive, R channel becomes negative.

|name|pin|
|----|---|
|DATAL|data_pin|
|DATAR|data_pin + 1|
|LRCLK|clock_pin_base|
|BCLK|clock_pin_base+1|

## About MCLK
MCLK is fixed at 22.5792/24.576MHz.

In EXDF mode, the same clock as BCK is output.

## Default Settings
- Output format: i2s
- Low jitter mode: off

|name|pin|
|----|---|
|DATA|GPIO18|
|LRCLK|GPIO20|
|BCLK|GPIO21|
|MCLK|GPIO22|

## API Reference

### Configuration Functions

#### `i2s_mclk_set_pin()`
```c
void i2s_mclk_set_pin(uint data_pin, uint clock_pin_base, uint mclk_pin);
```
Set I2S output pins. Must be called before initialization.
- `data_pin`: Data output pin (DATA/DOUT)
- `clock_pin_base`: LRCLK output pin (BCLK will be clock_pin_base+1)
- `mclk_pin`: MCLK output pin (not used for PT8211 modes)

#### `i2s_mclk_set_config()`
```c
void i2s_mclk_set_config(PIO pio, uint sm, int dma_ch, bool use_core1,
                         CLOCK_MODE clock_mode, I2S_MODE mode);
```
Configure I2S settings. Call before uart/i2c/spi when using low jitter mode.
- `pio`: PIO instance (pio0 or pio1)
- `sm`: State machine (0-2, MCLK uses sm+1)
- `dma_ch`: DMA channel to use
- `use_core1`: Use core1 for data transfer
- `clock_mode`: Clock mode (see Clock Modes below)
- `mode`: Output format (see Output Modes below)

#### `i2s_mclk_init()`
```c
void i2s_mclk_init(uint32_t audio_clock);
```
Initialize I2S with specified sample rate. Starts output immediately.
- `audio_clock`: Sample rate in Hz (44100, 48000, 96000, etc.)

### Data Transfer Functions

#### `i2s_enqueue()`
```c
bool i2s_enqueue(uint8_t* in, int sample, uint8_t resolution);
```
Add audio data to I2S buffer.
- `in`: Audio data buffer
- `sample`: Number of bytes to transfer
- `resolution`: Bit depth (16, 24, or 32)
- Returns: true on success, false if buffer full

#### `i2s_dequeue()`
```c
bool i2s_dequeue(int32_t** buff, int* sample);
```
Retrieve data from buffer (used internally when use_core1 is true).
- `buff`: Pointer to receive buffer address
- `sample`: Pointer to receive sample count
- Returns: true on success, false if buffer empty

#### `i2s_get_buf_length()`
```c
int8_t i2s_get_buf_length(void);
```
Get current buffer fill level (0 to I2S_BUF_DEPTH).

### Control Functions

#### `i2s_volume_change()`
```c
void i2s_volume_change(int16_t v, int8_t ch);
```
Set volume in dB (fixed point 8.8 format).
- `v`: Volume level (0 = 0dB, 6<<8 = -6dB, etc.)
- `ch`: Channel (0=both, 1=left, 2=right)

#### `i2s_mclk_change_clock()`
```c
void i2s_mclk_change_clock(uint32_t audio_clock);
```
Change sample rate during playback.
- `audio_clock`: New sample rate in Hz

### Callback Functions

#### `set_playback_handler()`
```c
void set_playback_handler(ExternalFunction func);
```
Set callback for playback state changes.
- `func`: Function called with bool parameter (true=playing, false=stopped)

#### `set_core1_main_function()`
```c
void set_core1_main_function(Core1MainFunction func);
```
Set custom core1 main function (when use_core1 is true).

### Enumerations

#### Clock Modes
```c
typedef enum {
    CLOCK_MODE_DEFAULT,         // Standard mode with fractional divider
    CLOCK_MODE_LOW_JITTER_LOW,  // Low jitter mode (135.5/147.5MHz)
    CLOCK_MODE_LOW_JITTER,      // Same as LOW_JITTER_LOW
    CLOCK_MODE_LOW_JITTER_OC,   // Overclocked low jitter (271/295MHz)
    CLOCK_MODE_EXTERNAL         // External clock input
} CLOCK_MODE;
```

#### Output Modes
```c
typedef enum {
    MODE_I2S,          // Standard I2S format
    MODE_PT8211,       // PT8211 format (32fs BCLK, no MCLK)
    MODE_EXDF,         // AK449X EXDF format
    MODE_I2S_DUAL,     // Dual mono I2S
    MODE_PT8211_DUAL   // Dual mono PT8211
} I2S_MODE;
```

## Usage Examples

### Basic I2S Output
```c
#include "i2s.h"

int main() {
    // Configure pins
    i2s_mclk_set_pin(18, 20, 22);

    // Configure I2S
    i2s_mclk_set_config(pio0, 0, 0, false, CLOCK_MODE_DEFAULT, MODE_I2S);

    // Initialize at 48kHz
    i2s_mclk_init(48000);

    // Set volume to 0dB
    i2s_volume_change(0, 0);

    // Send audio data
    uint8_t audio_data[1920];  // Your audio data
    while (true) {
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            i2s_enqueue(audio_data, sizeof(audio_data), 16);
        }
        sleep_ms(1);
    }
}
```

### Low Jitter Mode for High-Quality Audio
```c
// IMPORTANT: Configure I2S before other peripherals
i2s_mclk_set_config(pio0, 0, 0, false, CLOCK_MODE_LOW_JITTER, MODE_I2S);

// Now initialize other peripherals
stdio_init_all();

// Initialize at 96kHz
i2s_mclk_init(96000);
```

### Dual Mono Configuration
```c
// Configure for dual mono PT8211 DACs
i2s_mclk_set_pin(18, 20, 0);  // No MCLK for PT8211
i2s_mclk_set_config(pio0, 0, 0, false, CLOCK_MODE_DEFAULT, MODE_PT8211_DUAL);
i2s_mclk_init(48000);

// Each DAC receives:
// - L channel: positive signal
// - R channel: inverted signal
```

### Dynamic Sample Rate Switching
```c
// Start at 48kHz
i2s_mclk_init(48000);

// Later, switch to 96kHz
i2s_mclk_change_clock(96000);
```

### Volume Control
```c
// Set both channels to -6dB
i2s_volume_change(6 << 8, 0);

// Mute left channel, set right to -12dB
i2s_volume_change(100 << 8, 1);  // Left muted
i2s_volume_change(12 << 8, 2);   // Right -12dB
```

### Playback State Monitoring
```c
void on_playback_change(bool playing) {
    if (playing) {
        printf("Playback started\n");
    } else {
        printf("Playback stopped (buffer empty)\n");
    }
}

// Set the handler
set_playback_handler(on_playback_change);
```

## Complete Examples

See the `examples/` directory for complete working examples:
- `basic_i2s_output.c` - Simple I2S output with sine wave
- `low_jitter_mode.c` - High-quality audio with low jitter mode
- `dual_mono_pt8211.c` - Dual mono configuration for balanced output
- `multi_sample_rate.c` - Dynamic sample rate switching
- `volume_control.c` - Volume control and channel balance

## Buffer Management

The library uses a circular buffer system:
- Buffer depth: `I2S_BUF_DEPTH` (default 8)
- Target level: `I2S_TARGET_LEVEL` (default 4)
- Start level: `I2S_START_LEVEL` (default 2)

Monitor buffer level with `i2s_get_buf_length()` to prevent underruns.
