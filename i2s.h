// SPDX-License-Identifier: MIT

/**
 * @file i2s.h
 * @author BambooMaster (https://misskey.hakoniwa-project.com/@BambooMaster)
 * @brief pico-i2s-pio
 * @version 0.4
 * @date 2025-05-05
 * 
 */

#ifndef I2S_H
#define I2S_H
#include "hardware/pio.h"

#define I2S_BUF_DEPTH   8
#define I2S_START_LEVEL     (I2S_BUF_DEPTH / 4)
#define I2S_TARGET_LEVEL    (I2S_BUF_DEPTH / 2)
#define I2S_DATA_LEN    ((384 + 1) * 2 * 2)

typedef enum {
    MODE_I2S,
    MODE_PT8211,
    MODE_EXDF,
    MODE_I2S_DUAL,
    MODE_PT8211_DUAL
} I2S_MODE;

typedef enum {
    CLOCK_MODE_DEFAULT,
    CLOCK_MODE_LOW_JITTER_LOW,
    CLOCK_MODE_LOW_JITTER,
    CLOCK_MODE_LOW_JITTER_OC,
    CLOCK_MODE_EXTERNAL
} CLOCK_MODE;

/**
 * @brief Function type for notifying playback state changes
 *
 * @param state Playback state true:playback started false:playback stopped
 */
typedef void (*ExternalFunction)(bool state);

/**
 * @brief Function type for core1 main function
 *
 */
typedef void (*Core1MainFunction)(void);

/**
 * @brief Set i2s output pins
 *
 * @param data_pin data output pin
 * @param clock_pin_base LRCLK output pin
 * @param mclk_pin_pin MCLK output pin
 * @note BCLK=clock_pin_base+1
 * @note For MODE_EXDF, DOUTL = data_pin, DOUTR = data_pin + 1, WCK=clock_pin_base, BCK=clock_pin_base+1 MCLK=clock_pin_base+2
 */
void i2s_mclk_set_pin(uint data_pin, uint clock_pin_base, uint mclk_pin);

/**
 * @brief Configure i2s settings
 *
 * @param pio PIO to use for i2s: pio0 or pio1
 * @param sm State machine to use for i2s: sm0~2 (mclk uses sm+1)
 * @param dma_ch DMA channel to use for i2s
 * @param use_core1 Whether to use core1 for sending data to PIO FIFO
 * @param clock_mode Clock mode selection (CLOCK_MODE_DEFAULT, CLOCK_MODE_LOW_JITTER, CLOCK_MODE_LOW_JITTER_OC, CLOCK_MODE_EXTERNAL)
 * @param mode Output format selection (MODE_I2S, MODE_PT8211, MODE_EXDF, MODE_I2S_DUAL, MODE_PT8211_DUAL)
 * @note When using low jitter mode, call before uart, i2s, spi configuration
 * @note MODE_PT8211 is BCLK32fs lsbj16, no MCLK
 */
void i2s_mclk_set_config(PIO pio, uint sm, int dma_ch, bool use_core1, CLOCK_MODE clock_mode, I2S_MODE mode);

/**
 * @brief Initialize i2s
 *
 * @param audio_clock Sampling frequency
 * @note i2s output starts immediately after calling
 */
void i2s_mclk_init(uint32_t audio_clock);

/**
 * @brief Change i2s frequency
 *
 * @param audio_clock Sampling frequency
 * @note ToDo Mute processing function is called before and after execution
 */
void i2s_mclk_change_clock(uint32_t audio_clock);

/**
 * @brief Store uint8_t data sent from USB into i2s buffer
 *
 * @param in Data to store
 * @param sample Number of bytes to store
 * @param resolution Sample bit depth (16, 24, 32)
 * @return true Success
 * @return false Failed (buffer full)
 */
bool i2s_enqueue(uint8_t* in, int sample, uint8_t resolution);

/**
 * @brief Retrieve data from i2s buffer
 *
 * @param buff Retrieved data
 * @param sample Number of bytes retrieved
 * @return true Success
 * @return false Failed (buffer empty)
 * @note Called when use_core1 is true
 */
bool i2s_dequeue(int32_t** buff, int* sample);

/**
 * @brief Get i2s buffer length
 *
 * @return int8_t Buffer length
 */
int8_t i2s_get_buf_length(void);

/**
 * @brief Change i2s volume
 *
 * @param v Volume
 * @param ch Channel 0:L&R 1:L 2:R
 */
void i2s_volume_change(int16_t v, int8_t ch);

/**
 * @brief Set handler for notifying i2s playback state changes
 *
 * @param func Function pointer in ExternalFunction format
 */
void set_playback_handler(ExternalFunction func);

/**
 * @brief Set core1 main function
 *
 * @param func Function pointer in Core1MainFunction format
 */
void set_core1_main_function(Core1MainFunction func);

/**
 * @brief Fast function for alternately rearranging EXDF LR bits
 *
 * @param x Input
 */
__force_inline uint64_t part1by1_32(uint32_t x){
    uint64_t res = x;
    res = (res | (res << 16)) & 0x0000FFFF0000FFFFULL;
    res = (res | (res << 8))  & 0x00FF00FF00FF00FFULL;
    res = (res | (res << 4))  & 0x0F0F0F0F0F0F0F0FULL;
    res = (res | (res << 2))  & 0x3333333333333333ULL;
    res = (res | (res << 1))  & 0x5555555555555555ULL;
    return res;
}

#endif