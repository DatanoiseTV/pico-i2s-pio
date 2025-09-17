/*
 * PT8211DAC - Example for PT8211 16-bit DAC
 *
 * The PT8211 is an inexpensive 16-bit stereo DAC that uses
 * a simplified I2S protocol (no MCLK required).
 *
 * Connections:
 * - DATA  -> GPIO18 -> PT8211 DIN
 * - LRCLK -> GPIO20 -> PT8211 WS
 * - BCLK  -> GPIO21 -> PT8211 BCK
 * - No MCLK connection needed
 *
 * PT8211 Power:
 * - VDD -> 3.3V or 5V
 * - GND -> GND
 * - L/R outputs -> Audio amplifier or headphones
 */

#include <PicoI2SPIO.h>

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("PT8211 DAC Example");

  // Initialize for PT8211 (no MCLK needed)
  bool success = I2S.beginAdvanced(
    18,                    // data_pin
    20,                    // clock_pin_base
    0,                     // mclk_pin (0 = not used)
    pio0,                  // PIO
    0,                     // State machine
    0,                     // DMA channel
    false,                 // Don't use core1
    CLOCK_MODE_DEFAULT,    // Standard clock
    MODE_PT8211,           // PT8211 mode (32fs BCLK, no MCLK)
    48000,                 // 48kHz sample rate
    16                     // 16-bit (PT8211 is 16-bit only)
  );

  if (!success) {
    Serial.println("Failed to initialize I2S for PT8211!");
    while (1);
  }

  Serial.println("PT8211 initialized successfully");
  Serial.println("Format: LSBJ 16-bit");
  Serial.println("BCLK: 32 * fs (1.536MHz at 48kHz)");

  I2S.setVolume(90);  // Set to 90% volume
}

void loop() {
  static float phase = 0.0;
  static uint32_t lastFreqChange = 0;
  static float frequency = 440.0;

  // Change frequency every 2 seconds for variety
  if (millis() - lastFreqChange > 2000) {
    lastFreqChange = millis();
    // Cycle through musical notes
    static int note = 0;
    float notes[] = {440.0, 493.88, 523.25, 587.33, 659.25, 698.46, 783.99, 880.0};
    frequency = notes[note];
    note = (note + 1) % 8;
    Serial.print("Playing frequency: ");
    Serial.print(frequency);
    Serial.println("Hz");
  }

  // Generate samples
  int16_t buffer[256];
  for (int i = 0; i < 128; i++) {
    float sample = sin(phase) * 0.7;  // 70% amplitude
    int16_t value = I2S.floatToInt16(sample);

    buffer[i * 2] = value;
    buffer[i * 2 + 1] = value;

    phase += 2.0 * PI * frequency / 48000.0;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
  }

  // Write to PT8211
  if (!I2S.write(buffer, 256)) {
    delay(1);
  }
}