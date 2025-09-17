// SPDX-License-Identifier: MIT

/**
 * @file basic_i2s_output.c
 * @brief Basic I2S output example using pico-i2s-pio library
 *
 * This example demonstrates how to set up basic I2S output
 * with a PCM5102A DAC at 48kHz sample rate.
 */

#include "pico/stdlib.h"
#include "i2s.h"
#include <math.h>
#include <stdio.h>

// Generate a simple sine wave for testing
void generate_sine_wave(uint8_t* buffer, int samples, float frequency, int sample_rate) {
    int16_t* audio_buffer = (int16_t*)buffer;

    for (int i = 0; i < samples / 4; i++) {
        float sample = sinf(2.0f * M_PI * frequency * i / sample_rate) * 0x7FFF;
        audio_buffer[i * 2] = (int16_t)sample;     // Left channel
        audio_buffer[i * 2 + 1] = (int16_t)sample; // Right channel
    }
}

int main() {
    stdio_init_all();

    // Wait for USB serial connection (optional)
    sleep_ms(2000);
    printf("I2S Basic Output Example\n");

    // Configure I2S pins (using default pins)
    // DATA: GPIO18, LRCLK: GPIO20, BCLK: GPIO21, MCLK: GPIO22
    i2s_mclk_set_pin(18, 20, 22);

    // Configure I2S settings
    // Using PIO0, state machine 0, DMA channel 0
    // No core1 usage, default clock mode, standard I2S format
    i2s_mclk_set_config(pio0, 0, 0, false, CLOCK_MODE_DEFAULT, MODE_I2S);

    // Initialize I2S at 48kHz sample rate
    i2s_mclk_init(48000);

    // Set volume to 0dB (maximum) for both channels
    i2s_volume_change(0, 0);

    // Generate test audio - 440Hz sine wave (A4 note)
    uint8_t audio_buffer[1920]; // 48000 Hz * 0.01 sec * 2 channels * 2 bytes
    generate_sine_wave(audio_buffer, sizeof(audio_buffer), 440.0f, 48000);

    printf("Playing 440Hz sine wave...\n");

    // Main loop - continuously play the sine wave
    while (true) {
        // Check buffer space availability
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            // Enqueue audio data
            // Parameters: buffer, byte count, bit depth (16, 24, or 32)
            if (i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16)) {
                // Data successfully queued
            } else {
                // Buffer full, wait a bit
                sleep_ms(1);
            }
        } else {
            // Buffer has enough data, wait
            sleep_ms(1);
        }
    }

    return 0;
}