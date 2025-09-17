/*
 * SimpleTone - Generate a simple 440Hz tone using I2S
 *
 * This example shows basic usage of the PicoI2SPIO library
 * to generate a sine wave tone on an I2S DAC.
 *
 * Connections:
 * - DATA  -> GPIO18 -> DAC DIN
 * - LRCLK -> GPIO20 -> DAC LRCK/WS
 * - BCLK  -> GPIO21 -> DAC BCK/SCK
 * - MCLK  -> GPIO22 -> DAC MCLK (if required)
 *
 * Tested with PCM5102A and PT8211 DACs
 */

#include <PicoI2SPIO.h>

const float FREQUENCY = 440.0;  // A4 note
const float AMPLITUDE = 0.5;    // 50% volume

float phase = 0.0;
int16_t buffer[256];  // Stereo buffer (128 samples per channel)

void setup() {
  Serial.begin(115200);

  // Wait for serial connection
  while (!Serial && millis() < 3000);

  Serial.println("PicoI2SPIO Simple Tone Example");

  // Initialize I2S with default pins at 48kHz, 16-bit
  if (!I2S.begin(48000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  Serial.println("I2S initialized successfully");
  Serial.println("Playing 440Hz tone...");

  // Set volume to 75%
  I2S.setVolume(75);
}

void loop() {
  // Generate sine wave samples
  for (int i = 0; i < 128; i++) {
    float sample = sin(phase) * AMPLITUDE;
    int16_t value = I2S.floatToInt16(sample);

    buffer[i * 2] = value;      // Left channel
    buffer[i * 2 + 1] = value;  // Right channel

    phase += 2.0 * PI * FREQUENCY / 48000.0;
    if (phase > 2.0 * PI) {
      phase -= 2.0 * PI;
    }
  }

  // Write samples to I2S
  if (!I2S.write(buffer, 128 * 2)) {
    // Buffer full, wait a bit
    delay(1);
  }
}