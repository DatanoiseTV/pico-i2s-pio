// SPDX-License-Identifier: MIT

/**
 * @file PicoI2SPIO.cpp
 * @brief Arduino wrapper implementation for pico-i2s-pio library
 * @version 0.4.0
 */

#include "PicoI2SPIO.h"
#include <cstring>

// Global singleton instance
PicoI2SPIO I2S;

// Constructor
PicoI2SPIO::PicoI2SPIO()
    : pio_(pio0), sm_(0), dma_ch_(0), initialized_(false),
      sample_rate_(48000), bit_depth_(16),
      callback_16_(nullptr), callback_32_(nullptr), callback_float_(nullptr),
      callback_active_(false), callback_buffer_16_(nullptr),
      callback_buffer_32_(nullptr), callback_float_left_(nullptr),
      callback_float_right_(nullptr), callback_buffer_size_(0) {
}

// Initialize with default pins
bool PicoI2SPIO::begin(uint32_t sample_rate, uint8_t bit_depth) {
    return begin(18, 20, 22, sample_rate, bit_depth);
}

// Initialize with custom pins
bool PicoI2SPIO::begin(uint data_pin, uint clock_pin_base, uint mclk_pin,
                       uint32_t sample_rate, uint8_t bit_depth) {
    return beginAdvanced(data_pin, clock_pin_base, mclk_pin,
                        pio0, 0, 0, false,
                        CLOCK_MODE_DEFAULT, MODE_I2S,
                        sample_rate, bit_depth);
}

// Advanced initialization
bool PicoI2SPIO::beginAdvanced(uint data_pin, uint clock_pin_base, uint mclk_pin,
                               PIO pio, uint sm, int dma_ch, bool use_core1,
                               CLOCK_MODE clock_mode, I2S_MODE mode,
                               uint32_t sample_rate, uint8_t bit_depth) {
    if (initialized_) {
        end();
    }

    // Validate parameters
    if (bit_depth != 16 && bit_depth != 24 && bit_depth != 32) {
        return false;
    }

    if (sample_rate < 8000 || sample_rate > 384000) {
        return false;
    }

    // Store settings
    pio_ = pio;
    sm_ = sm;
    dma_ch_ = dma_ch;
    sample_rate_ = sample_rate;
    bit_depth_ = bit_depth;

    // Configure I2S
    i2s_mclk_set_pin(data_pin, clock_pin_base, mclk_pin);
    i2s_mclk_set_config(pio, sm, dma_ch, use_core1, clock_mode, mode);
    i2s_mclk_init(sample_rate);

    initialized_ = true;
    return true;
}

// Stop I2S output
void PicoI2SPIO::end() {
    if (!initialized_) {
        return;
    }

    // Stop the PIO state machine
    pio_sm_set_enabled(pio_, sm_, false);
    pio_sm_set_enabled(pio_, sm_ + 1, false);  // MCLK state machine

    // Clean up callback buffers
    if (callback_buffer_16_) {
        delete[] callback_buffer_16_;
        callback_buffer_16_ = nullptr;
    }
    if (callback_buffer_32_) {
        delete[] callback_buffer_32_;
        callback_buffer_32_ = nullptr;
    }
    if (callback_float_left_) {
        delete[] callback_float_left_;
        callback_float_left_ = nullptr;
    }
    if (callback_float_right_) {
        delete[] callback_float_right_;
        callback_float_right_ = nullptr;
    }

    initialized_ = false;
    callback_active_ = false;
}

// Set audio generation callbacks
void PicoI2SPIO::setCallback(AudioCallback callback) {
    callback_16_ = callback;
    callback_32_ = nullptr;
    callback_float_ = nullptr;

    // Allocate buffer if needed
    if (!callback_buffer_16_ && callback) {
        callback_buffer_size_ = 256;  // 128 stereo samples
        callback_buffer_16_ = new int16_t[callback_buffer_size_];
    }
}

void PicoI2SPIO::setCallback32(AudioCallback32 callback) {
    callback_16_ = nullptr;
    callback_32_ = callback;
    callback_float_ = nullptr;

    // Allocate buffer if needed
    if (!callback_buffer_32_ && callback) {
        callback_buffer_size_ = 256;  // 128 stereo samples
        callback_buffer_32_ = new int32_t[callback_buffer_size_];
    }
}

void PicoI2SPIO::setCallbackFloat(AudioCallbackFloat callback) {
    callback_16_ = nullptr;
    callback_32_ = nullptr;
    callback_float_ = callback;

    // Allocate buffers if needed
    if (!callback_float_left_ && callback) {
        callback_buffer_size_ = 128;  // 128 samples per channel
        callback_float_left_ = new float[callback_buffer_size_];
        callback_float_right_ = new float[callback_buffer_size_];
    }
}

// Process callback and fill buffer
bool PicoI2SPIO::processCallback() {
    if (!initialized_ || (!callback_16_ && !callback_32_ && !callback_float_)) {
        return false;
    }

    // Check if buffer has space
    if (i2s_get_buf_length() >= I2S_TARGET_LEVEL) {
        return false;
    }

    if (callback_16_ && bit_depth_ == 16) {
        // Call the callback to generate samples
        callback_16_(callback_buffer_16_, callback_buffer_size_ / 2);
        // Write to I2S
        return write(reinterpret_cast<uint8_t*>(callback_buffer_16_),
                    callback_buffer_size_ * sizeof(int16_t));
    }
    else if (callback_32_ && bit_depth_ == 32) {
        // Call the callback to generate samples
        callback_32_(callback_buffer_32_, callback_buffer_size_ / 2);
        // Write to I2S
        return write(reinterpret_cast<uint8_t*>(callback_buffer_32_),
                    callback_buffer_size_ * sizeof(int32_t));
    }
    else if (callback_float_) {
        // Call the callback to generate samples
        callback_float_(callback_float_left_, callback_float_right_, callback_buffer_size_);

        // Convert float to appropriate bit depth
        if (bit_depth_ == 16) {
            if (!callback_buffer_16_) {
                callback_buffer_16_ = new int16_t[callback_buffer_size_ * 2];
            }
            for (size_t i = 0; i < callback_buffer_size_; i++) {
                callback_buffer_16_[i * 2] = floatToInt16(callback_float_left_[i]);
                callback_buffer_16_[i * 2 + 1] = floatToInt16(callback_float_right_[i]);
            }
            return write(reinterpret_cast<uint8_t*>(callback_buffer_16_),
                        callback_buffer_size_ * 2 * sizeof(int16_t));
        }
        else if (bit_depth_ == 32) {
            if (!callback_buffer_32_) {
                callback_buffer_32_ = new int32_t[callback_buffer_size_ * 2];
            }
            for (size_t i = 0; i < callback_buffer_size_; i++) {
                callback_buffer_32_[i * 2] = floatToInt32(callback_float_left_[i]);
                callback_buffer_32_[i * 2 + 1] = floatToInt32(callback_float_right_[i]);
            }
            return write(reinterpret_cast<uint8_t*>(callback_buffer_32_),
                        callback_buffer_size_ * 2 * sizeof(int32_t));
        }
        else if (bit_depth_ == 24) {
            // 24-bit requires special handling
            uint8_t* buffer24 = new uint8_t[callback_buffer_size_ * 2 * 3];
            for (size_t i = 0; i < callback_buffer_size_; i++) {
                int32_t left24 = floatToInt24(callback_float_left_[i]);
                int32_t right24 = floatToInt24(callback_float_right_[i]);

                // Pack left channel
                buffer24[i * 6] = (left24 >> 8) & 0xFF;
                buffer24[i * 6 + 1] = (left24 >> 16) & 0xFF;
                buffer24[i * 6 + 2] = (left24 >> 24) & 0xFF;

                // Pack right channel
                buffer24[i * 6 + 3] = (right24 >> 8) & 0xFF;
                buffer24[i * 6 + 4] = (right24 >> 16) & 0xFF;
                buffer24[i * 6 + 5] = (right24 >> 24) & 0xFF;
            }
            bool result = write(buffer24, callback_buffer_size_ * 2 * 3);
            delete[] buffer24;
            return result;
        }
    }

    return false;
}

// Start automatic callback processing
void PicoI2SPIO::startCallback() {
    callback_active_ = true;
}

// Stop automatic callback processing
void PicoI2SPIO::stopCallback() {
    callback_active_ = false;
}

// Write audio samples
bool PicoI2SPIO::write(const uint8_t* data, size_t bytes) {
    if (!initialized_) {
        return false;
    }

    // Make a mutable copy for the C library
    uint8_t* mutable_data = const_cast<uint8_t*>(data);
    return i2s_enqueue(mutable_data, bytes, bit_depth_);
}

bool PicoI2SPIO::write(const int16_t* samples, size_t count) {
    if (!initialized_ || bit_depth_ != 16) {
        return false;
    }

    return write(reinterpret_cast<const uint8_t*>(samples), count * sizeof(int16_t));
}

bool PicoI2SPIO::write(const int32_t* samples, size_t count) {
    if (!initialized_ || bit_depth_ != 32) {
        return false;
    }

    return write(reinterpret_cast<const uint8_t*>(samples), count * sizeof(int32_t));
}

// Write single stereo sample
bool PicoI2SPIO::writeStereo(int16_t left, int16_t right) {
    if (!initialized_ || bit_depth_ != 16) {
        return false;
    }

    int16_t stereo[2] = {left, right};
    return write(stereo, 2);
}

bool PicoI2SPIO::writeStereo(int32_t left, int32_t right) {
    if (!initialized_ || bit_depth_ != 32) {
        return false;
    }

    int32_t stereo[2] = {left, right};
    return write(stereo, 2);
}

// Buffer management
int PicoI2SPIO::availableForWrite() {
    if (!initialized_) {
        return 0;
    }

    int8_t used = i2s_get_buf_length();
    return (I2S_BUF_DEPTH - used) * I2S_DATA_LEN * (bit_depth_ / 8);
}

bool PicoI2SPIO::isFull() {
    if (!initialized_) {
        return true;
    }

    return i2s_get_buf_length() >= I2S_BUF_DEPTH;
}

void PicoI2SPIO::flush() {
    if (!initialized_) {
        return;
    }

    // Wait until buffer is empty
    while (i2s_get_buf_length() > 0) {
        delay(1);
    }
}

// Volume control (0-100%)
void PicoI2SPIO::setVolume(uint8_t volume) {
    if (volume > 100) volume = 100;

    // Convert percentage to dB
    // 100% = 0dB, 50% = -6dB, 25% = -12dB, etc.
    int8_t db;
    if (volume == 0) {
        db = 100;  // -100dB (mute)
    } else if (volume == 100) {
        db = 0;
    } else {
        // Logarithmic conversion
        float ratio = volume / 100.0f;
        db = (int8_t)(-20.0f * log10f(ratio));
        if (db > 100) db = 100;
    }

    setVolumeDB(-db);
}

void PicoI2SPIO::setVolume(uint8_t left_volume, uint8_t right_volume) {
    if (left_volume > 100) left_volume = 100;
    if (right_volume > 100) right_volume = 100;

    int8_t left_db, right_db;

    // Convert left channel
    if (left_volume == 0) {
        left_db = 100;
    } else if (left_volume == 100) {
        left_db = 0;
    } else {
        float ratio = left_volume / 100.0f;
        left_db = (int8_t)(-20.0f * log10f(ratio));
        if (left_db > 100) left_db = 100;
    }

    // Convert right channel
    if (right_volume == 0) {
        right_db = 100;
    } else if (right_volume == 100) {
        right_db = 0;
    } else {
        float ratio = right_volume / 100.0f;
        right_db = (int8_t)(-20.0f * log10f(ratio));
        if (right_db > 100) right_db = 100;
    }

    setVolumeDB(-left_db, -right_db);
}

// Volume in dB (-100 to 0)
void PicoI2SPIO::setVolumeDB(int8_t db) {
    if (db > 0) db = 0;
    if (db < -100) db = -100;

    i2s_volume_change((-db) << 8, 0);  // Both channels
}

void PicoI2SPIO::setVolumeDB(int8_t left_db, int8_t right_db) {
    if (left_db > 0) left_db = 0;
    if (left_db < -100) left_db = -100;
    if (right_db > 0) right_db = 0;
    if (right_db < -100) right_db = -100;

    i2s_volume_change((-left_db) << 8, 1);   // Left channel
    i2s_volume_change((-right_db) << 8, 2);  // Right channel
}

// Change sample rate
bool PicoI2SPIO::setSampleRate(uint32_t sample_rate) {
    if (!initialized_) {
        return false;
    }

    if (sample_rate < 8000 || sample_rate > 384000) {
        return false;
    }

    i2s_mclk_change_clock(sample_rate);
    sample_rate_ = sample_rate;
    return true;
}

// Static callback for playback state
void PicoI2SPIO::setPlaybackHandler(void (*handler)(bool)) {
    set_playback_handler((ExternalFunction)handler);
}

// Utility functions
int16_t PicoI2SPIO::floatToInt16(float sample) {
    // Clamp to [-1, 1]
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;

    return (int16_t)(sample * 32767.0f);
}

int32_t PicoI2SPIO::floatToInt32(float sample) {
    // Clamp to [-1, 1]
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;

    return (int32_t)(sample * 2147483647.0f);
}

int16_t PicoI2SPIO::floatToInt24(float sample) {
    // Clamp to [-1, 1]
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;

    // 24-bit is typically stored in int32_t
    return (int32_t)(sample * 8388607.0f) << 8;
}