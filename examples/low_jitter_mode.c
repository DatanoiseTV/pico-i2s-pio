// SPDX-License-Identifier: MIT

/**
 * @file low_jitter_mode.c
 * @brief Low jitter mode example for high-quality audio applications
 *
 * This example shows how to use low jitter mode for improved audio quality
 * by setting the system clock to an integer multiple of MCLK.
 */

#include "pico/stdlib.h"
#include "i2s.h"
#include <stdio.h>

// Custom playback state handler
void playback_state_changed(bool is_playing) {
    if (is_playing) {
        printf("Playback started\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
    } else {
        printf("Playback stopped\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
    }
}

int main() {
    // IMPORTANT: Configure I2S BEFORE initializing other peripherals
    // when using low jitter mode

    // Configure I2S pins
    i2s_mclk_set_pin(18, 20, 22);

    // Configure I2S with LOW JITTER mode
    // This sets system clock to 147.5MHz for 48kHz family
    // or 135.5MHz for 44.1kHz family
    i2s_mclk_set_config(
        pio0,                    // PIO instance
        0,                       // State machine
        0,                       // DMA channel
        false,                   // Don't use core1
        CLOCK_MODE_LOW_JITTER,   // Low jitter mode
        MODE_I2S                 // Standard I2S format
    );

    // NOW initialize other peripherals
    stdio_init_all();
    sleep_ms(2000);
    printf("I2S Low Jitter Mode Example\n");

    // Set custom playback state handler
    set_playback_handler(playback_state_changed);

    // Initialize I2S at 96kHz for high-quality audio
    i2s_mclk_init(96000);

    // Set volume to -6dB
    i2s_volume_change(6 << 8, 0);  // Volume is in 8.8 fixed point format

    // Generate test pattern - 24-bit audio samples
    uint8_t audio_buffer_24bit[576]; // 96000 Hz * 0.002 sec * 2 channels * 3 bytes

    // Fill with 24-bit test pattern
    for (int i = 0; i < sizeof(audio_buffer_24bit); i += 6) {
        // Left channel (24-bit)
        audio_buffer_24bit[i] = 0x00;     // Low byte
        audio_buffer_24bit[i + 1] = 0x00; // Mid byte
        audio_buffer_24bit[i + 2] = 0x40; // High byte

        // Right channel (24-bit)
        audio_buffer_24bit[i + 3] = 0x00; // Low byte
        audio_buffer_24bit[i + 4] = 0x00; // Mid byte
        audio_buffer_24bit[i + 5] = 0x40; // High byte
    }

    printf("Playing 96kHz 24-bit audio in low jitter mode...\n");

    while (true) {
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            // Send 24-bit audio data
            i2s_enqueue(audio_buffer_24bit, sizeof(audio_buffer_24bit), 24);
        }
        sleep_ms(1);
    }

    return 0;
}