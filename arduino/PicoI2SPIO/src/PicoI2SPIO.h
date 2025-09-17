// SPDX-License-Identifier: MIT

/**
 * @file PicoI2SPIO.h
 * @brief Arduino wrapper for pico-i2s-pio library
 * @version 0.4.0
 */

#ifndef PICO_I2S_PIO_ARDUINO_H
#define PICO_I2S_PIO_ARDUINO_H

#include <Arduino.h>
#include "i2s.h"

class PicoI2SPIO {
private:
    PIO pio_;
    uint sm_;
    int dma_ch_;
    bool initialized_;
    uint32_t sample_rate_;
    uint8_t bit_depth_;

public:
    // Constructor
    PicoI2SPIO();

    // Initialize with default pins
    bool begin(uint32_t sample_rate = 48000, uint8_t bit_depth = 16);

    // Initialize with custom pins
    bool begin(uint data_pin, uint clock_pin_base, uint mclk_pin,
               uint32_t sample_rate = 48000, uint8_t bit_depth = 16);

    // Advanced initialization
    bool beginAdvanced(uint data_pin, uint clock_pin_base, uint mclk_pin,
                       PIO pio, uint sm, int dma_ch, bool use_core1,
                       CLOCK_MODE clock_mode, I2S_MODE mode,
                       uint32_t sample_rate, uint8_t bit_depth);

    // Stop I2S output
    void end();

    // Write audio samples
    bool write(const uint8_t* data, size_t bytes);
    bool write(const int16_t* samples, size_t count);
    bool write(const int32_t* samples, size_t count);

    // Write single stereo sample
    bool writeStereo(int16_t left, int16_t right);
    bool writeStereo(int32_t left, int32_t right);

    // Buffer management
    int availableForWrite();
    bool isFull();
    void flush();

    // Volume control (0-100%)
    void setVolume(uint8_t volume);
    void setVolume(uint8_t left_volume, uint8_t right_volume);

    // Volume in dB (-100 to 0)
    void setVolumeDB(int8_t db);
    void setVolumeDB(int8_t left_db, int8_t right_db);

    // Change sample rate
    bool setSampleRate(uint32_t sample_rate);

    // Get current settings
    uint32_t getSampleRate() const { return sample_rate_; }
    uint8_t getBitDepth() const { return bit_depth_; }
    bool isInitialized() const { return initialized_; }

    // Static callback for playback state
    static void setPlaybackHandler(void (*handler)(bool));

    // Utility functions
    static int16_t floatToInt16(float sample);
    static int32_t floatToInt32(float sample);
    static int16_t floatToInt24(float sample);
};

// Singleton instance for simple usage
extern PicoI2SPIO I2S;

#endif // PICO_I2S_PIO_ARDUINO_H