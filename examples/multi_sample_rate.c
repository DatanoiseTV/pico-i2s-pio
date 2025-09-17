// SPDX-License-Identifier: MIT

/**
 * @file multi_sample_rate.c
 * @brief Dynamic sample rate switching example
 *
 * This example demonstrates how to dynamically change sample rates
 * during runtime, useful for playing different audio sources.
 */

#include "pico/stdlib.h"
#include "i2s.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t sample_rate;
    const char* name;
} SampleRateConfig;

// Common sample rates
SampleRateConfig sample_rates[] = {
    {44100, "CD Quality"},
    {48000, "DVD/Digital Audio"},
    {88200, "High-Res 2x CD"},
    {96000, "High-Res 2x DVD"},
    {176400, "High-Res 4x CD"},
    {192000, "High-Res 4x DVD"}
};

// Generate test tone at current sample rate
void generate_test_tone(uint8_t* buffer, int size, uint32_t sample_rate) {
    int16_t* samples = (int16_t*)buffer;
    static float phase = 0.0f;
    float freq = 1000.0f; // 1kHz test tone

    for (int i = 0; i < size / 4; i++) {
        float sample = sinf(phase) * 0x4000;
        samples[i * 2] = (int16_t)sample;
        samples[i * 2 + 1] = (int16_t)sample;

        phase += 2.0f * M_PI * freq / sample_rate;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Multi Sample Rate Example\n");

    // Configure I2S pins
    i2s_mclk_set_pin(18, 20, 22);

    // Configure I2S
    i2s_mclk_set_config(
        pio0,
        0,
        0,
        false,
        CLOCK_MODE_DEFAULT,
        MODE_I2S
    );

    // Start with 48kHz
    uint32_t current_rate_idx = 1;
    i2s_mclk_init(sample_rates[current_rate_idx].sample_rate);

    printf("Starting with %s (%luHz)\n",
           sample_rates[current_rate_idx].name,
           sample_rates[current_rate_idx].sample_rate);

    // Set volume
    i2s_volume_change(0, 0);

    uint8_t audio_buffer[2048];
    uint32_t last_switch = 0;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Switch sample rate every 5 seconds
        if (now - last_switch >= 5000) {
            current_rate_idx = (current_rate_idx + 1) % 6;
            uint32_t new_rate = sample_rates[current_rate_idx].sample_rate;

            printf("Switching to %s (%luHz)\n",
                   sample_rates[current_rate_idx].name,
                   new_rate);

            // Change sample rate
            i2s_mclk_change_clock(new_rate);

            last_switch = now;
        }

        // Generate and play test tone
        if (i2s_get_buf_length() < I2S_TARGET_LEVEL) {
            generate_test_tone(audio_buffer, sizeof(audio_buffer),
                             sample_rates[current_rate_idx].sample_rate);
            i2s_enqueue(audio_buffer, sizeof(audio_buffer), 16);
        }

        sleep_ms(1);
    }

    return 0;
}