# pico-i2s-pio
Raspberry Pi Picoのpioを使ってMCLK対応のi2sを出力するライブラリです。RP2040/RP2350のシステムクロックをMCLKの整数倍に設定し、pioのフラクショナル分周を使わないlowジッタモードを搭載しています。

試験的にAK449Xで使用できるEXDFモードを追加しました。実機での動作は未確認です。

試験的に45.1584/49.152MHzの外部クロックを使用する外部クロックモードを追加しました。GPIO20に45.1584MHzを、GPIO22に49.152MHzを入力してください。動作確認していません。

## 対応フォーマット
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

## MCLKについて
MCLKは22.5792/24.576MHzで固定です。

EXDFモードの場合は、BCKと同じクロックが出力されます。

## デフォルト
- 出力フォーマット: i2s
- lowジッタモード: off

|name|pin|
|----|---|
|DATA|GPIO18|
|LRCLK|GPIO20|
|BCLK|GPIO21|
|MCLK|GPIO22|
