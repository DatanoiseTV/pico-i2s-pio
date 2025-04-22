// SPDX-License-Identifier: MIT

/**
 * @file i2s.h
 * @author BambooMaster (https://misskey.hakoniwa-project.com/@BambooMaster)
 * @brief pico-i2s-pio
 * @version 0.3
 * @date 2025-04-06
 * 
 */

#ifndef I2S_H
#define I2S_H
#include "hardware/pio.h"

#define I2S_BUF_DEPTH   16
#define I2S_START_LEVEL     I2S_BUF_DEPTH / 4
#define I2S_TARGET_LEVEL    I2S_BUF_DEPTH / 2
#define I2S_DATA_LEN    (384 + 1) * 2

/**
 * @brief 再生状態の切り替わりを通知する関数の型
 * 
 * @param state 再生状態 true:再生開始 false:再生停止
 */
typedef void (*ExternalFunction)(bool state);

/**
 * @brief core1のmain関数の型
 * 
 */
typedef void (*Core1MainFunction)(void);

/**
 * @brief i2sの出力ピンを設定する
 * 
 * @param data_pin data出力ピン
 * @param clock_pin_base LRCLK出力ピン
 * @note BCLK=clock_pin_base+1 MCLK=clock_pin_base+2
 * @note lowジッタモードを使用する場合はMCLKの出力ピンはGPIO21に固定される
 */
void i2s_mclk_set_pin(int data_pin, int clock_pin_base);

/**
 * @brief i2sの設定を行う
 * 
 * @param pio i2sに使用するpio pio0 or pio1
 * @param sm i2sに使用するsm 0~3
 * @param dma_ch i2sに使用するdmaチャンネル
 * @param use_core1 pioのFIFOへデータを送る処理をcore1で行うかどうか
 * @param low_jitter lowジッタモードを使用するかどうか
 * @param overclock オーバークロックを有効にするか (low_jitterモードのときのみ有効)
 * @param pt8211 pt8211を使用するか
 * @note lowジッタモードを使用する場合はuart,i2s,spi設定よりも先に呼び出す
 * @note PT8211はBCLK32fsのlsbj16,MCLKなし
 */
void i2s_mclk_set_config(PIO pio, uint sm, int dma_ch, bool use_core1, bool low_jitter, bool overclock, bool pt8211);

/**
 * @brief i2sの初期化を行う
 * 
 * @param audio_clock サンプリング周波数
 * @note 呼び出してすぐにi2sの出力が開始される
 */
void i2s_mclk_init(uint32_t audio_clock);

/**
 * @brief i2sの周波数を変更する
 * 
 * @param audio_clock サンプリング周波数
 * @note ToDo 実行前後でミュート処理関数を呼び出される
 */
void i2s_mclk_change_clock(uint32_t audio_clock);

/**
 * @brief i2sのバッファにUSBから送られてきたuint8_tデータを格納する
 * 
 * @param in 格納するデータ
 * @param sample 格納するバイト数
 * @param resolution サンプルのビット深度 (16, 24, 32)
 * @return true 成功
 * @return false 失敗(バッファが一杯)
 */
bool i2s_enqueue(uint8_t* in, int sample, uint8_t resolution);

/**
 * @brief i2sのバッファからデータを取り出す
 * 
 * @param buff 取り出したデータ
 * @param sample 取り出したバイト数
 * @return true 成功
 * @return false 失敗(バッファが空)
 * @note use_core1がtrueのときに呼び出される
 */
bool i2s_dequeue(int32_t** buff, int* sample);

/**
 * @brief i2sのバッファの長さを取得する
 * 
 * @return int8_t バッファの長さ
 */
int8_t i2s_get_buf_length(void);

/**
 * @brief i2sの音量を変更する
 * 
 * @param v 音量
 * @param ch チャンネル 0:L&R 1:L 2:R
 */
void i2s_volume_change(int16_t v, int8_t ch);

/**
 * @brief i2s再生状態の切り替わりを通知するハンドラを設定する
 * 
 * @param func ExternalFunction形式の関数ポインタ
 */
void set_playback_handler(ExternalFunction func);

/**
 * @brief core1のmain関数を設定する
 * 
 * @param func Core1MainFunction形式の関数ポインタ
 */
void set_core1_main_function(Core1MainFunction func);

#endif