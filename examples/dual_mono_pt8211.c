// SPDX-License-Identifier: MIT

/**
 * @file dual_mono_pt8211.c
 * @brief Dual mono configuration example using PT8211 DACs
 *
 * This example demonstrates how to use two PT8211 DACs in dual mono mode
 * where each DAC's L channel outputs positive and R channel outputs negative.
 * This configuration can be used for balanced audio output.
 */

#include "pico/stdlib.h"
#include "i2s.h"
#include <stdio.h>
#include <string.h>

// Generate differential test signal
void generate_differential_signal(uint8_t* buffer, int samples) {
    int16_t* audio_buffer = (int16_t*)buffer;
    static uint32_t phase = 0;

    for (int i = 0; i < samples / 4; i++) {
        // Create a square wave for clear differential testing
        int16_t value = (phase < 24000) ? 0x4000 : -0x4000;

        audio_buffer[i * 2] = value;      // Left channel (positive)
        audio_buffer[i * 2 + 1] = value;  // Right channel (will be inverted)

        phase++;
        if (phase >= 48000) phase = 0;
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("PT8211 Dual Mono Mode Example\n");

    // Configure I2S pins for dual output
    // DATAL: GPIO18, DATAR: GPIO19 (automatically data_pin + 1)
    // LRCLK: GPIO20, BCLK: GPIO21
    i2s_mclk_set_pin(18, 20, 0);  // No MCLK pin for PT8211

    // Configure for PT8211 dual mono mode
    i2s_mclk_set_config(
        pio0,                  // PIO instance
        0,                     // State machine
        0,                     // DMA channel
        false,                 // Don't use core1
        CLOCK_MODE_DEFAULT,    // Default clock mode
        MODE_PT8211_DUAL       // PT8211 dual mono mode
    );

    // Initialize at 48kHz
    i2s_mclk_init(48000);

    // Set individual channel volumes
    // Channel 1 (L): -3dB
    i2s_volume_change(3 << 8, 1);
    // Channel 2 (R): -3dB
    i2s_volume_change(3 << 8, 2);

    printf("PT8211 Dual Mono Configuration:\n");
    printf("- DATAL (GPIO18): Positive signal for DAC1\n");
    printf("- DATAR (GPIO19): Positive signal for DAC2\n");
    printf("- Each DAC L channel: Positive output\n");
    printf("- Each DAC R channel: Negative (inverted) output\n");
    printf("- BCLK: 32fs (1.536MHz at 48kHz)\n");

    uint8_t audio_buffer[960];
    generate_differential_signal(audio_buffer, sizeof(audio_buffer));

    while (true) {
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            if (!i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16)) {
                sleep_us(100);
            }
        } else {
            sleep_ms(1);
        }
    }

    return 0;
}