// SPDX-License-Identifier: MIT

/**
 * @file volume_control.c
 * @brief Volume control and channel balance example
 *
 * This example demonstrates volume control features including
 * individual channel control and smooth volume fading.
 */

#include "pico/stdlib.h"
#include "i2s.h"
#include <stdio.h>
#include <math.h>

// Volume levels in dB (using library's dB table)
typedef struct {
    int16_t db_value;
    const char* description;
} VolumeLevel;

VolumeLevel volume_presets[] = {
    {0, "Maximum (0dB)"},
    {6 << 8, "Comfortable (-6dB)"},
    {12 << 8, "Moderate (-12dB)"},
    {20 << 8, "Quiet (-20dB)"},
    {40 << 8, "Very Quiet (-40dB)"},
    {100 << 8, "Minimum (-100dB)"}
};

// Generate stereo test signal with different frequencies for L/R
void generate_stereo_test(uint8_t* buffer, int size) {
    int16_t* samples = (int16_t*)buffer;
    static float phase_l = 0.0f;
    static float phase_r = 0.0f;

    for (int i = 0; i < size / 4; i++) {
        // 440Hz on left channel (A4)
        samples[i * 2] = (int16_t)(sinf(phase_l) * 0x6000);
        phase_l += 2.0f * M_PI * 440.0f / 48000.0f;

        // 880Hz on right channel (A5)
        samples[i * 2 + 1] = (int16_t)(sinf(phase_r) * 0x6000);
        phase_r += 2.0f * M_PI * 880.0f / 48000.0f;

        if (phase_l > 2.0f * M_PI) phase_l -= 2.0f * M_PI;
        if (phase_r > 2.0f * M_PI) phase_r -= 2.0f * M_PI;
    }
}

// Smooth volume fade
void fade_volume(int16_t from_db, int16_t to_db, int steps, int delay_ms) {
    int16_t step_size = (to_db - from_db) / steps;
    int16_t current = from_db;

    for (int i = 0; i <= steps; i++) {
        i2s_volume_change(current, 0);  // Both channels
        sleep_ms(delay_ms);
        current += step_size;
    }
    i2s_volume_change(to_db, 0);  // Ensure we reach exact target
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Volume Control Example\n");

    // Configure I2S
    i2s_mclk_set_pin(18, 20, 22);
    i2s_mclk_set_config(pio0, 0, 0, false, CLOCK_MODE_DEFAULT, MODE_I2S);
    i2s_mclk_init(48000);

    printf("Playing stereo test signal:\n");
    printf("- Left channel: 440Hz (A4)\n");
    printf("- Right channel: 880Hz (A5)\n\n");

    uint8_t audio_buffer[1920];

    // Part 1: Volume presets demonstration
    printf("=== Volume Presets Demo ===\n");
    for (int i = 0; i < 6; i++) {
        printf("Setting volume to %s\n", volume_presets[i].description);
        i2s_volume_change(volume_presets[i].db_value, 0);  // Both channels

        // Play for 2 seconds at each level
        for (int j = 0; j < 200; j++) {
            if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
                generate_stereo_test(audio_buffer, sizeof(audio_buffer));
                i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16);
            }
            sleep_ms(10);
        }
    }

    // Part 2: Channel balance demonstration
    printf("\n=== Channel Balance Demo ===\n");
    i2s_volume_change(6 << 8, 0);  // Set both to -6dB

    printf("Left channel only\n");
    i2s_volume_change(6 << 8, 1);   // Left -6dB
    i2s_volume_change(100 << 8, 2); // Right muted
    for (int i = 0; i < 200; i++) {
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            generate_stereo_test(audio_buffer, sizeof(audio_buffer));
            i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16);
        }
        sleep_ms(10);
    }

    printf("Right channel only\n");
    i2s_volume_change(100 << 8, 1); // Left muted
    i2s_volume_change(6 << 8, 2);   // Right -6dB
    for (int i = 0; i < 200; i++) {
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            generate_stereo_test(audio_buffer, sizeof(audio_buffer));
            i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16);
        }
        sleep_ms(10);
    }

    // Part 3: Smooth fade demonstration
    printf("\n=== Smooth Fade Demo ===\n");
    printf("Fading in from silence...\n");

    i2s_volume_change(100 << 8, 0);  // Start at minimum

    // Continuous playback during fade
    while (true) {
        printf("Fade in (3 seconds)\n");
        // Fade from -100dB to 0dB over 3 seconds
        for (int vol = 100; vol >= 0; vol--) {
            i2s_volume_change(vol << 8, 0);

            // Keep playing during fade
            for (int i = 0; i < 3; i++) {
                if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
                    generate_stereo_test(audio_buffer, sizeof(audio_buffer));
                    i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16);
                }
                sleep_ms(10);
            }
        }

        printf("Fade out (3 seconds)\n");
        // Fade from 0dB to -100dB over 3 seconds
        for (int vol = 0; vol <= 100; vol++) {
            i2s_volume_change(vol << 8, 0);

            // Keep playing during fade
            for (int i = 0; i < 3; i++) {
                if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
                    generate_stereo_test(audio_buffer, sizeof(audio_buffer));
                    i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16);
                }
                sleep_ms(10);
            }
        }

        sleep_ms(1000);  // Pause in silence
    }

    return 0;
}