/*
 * HighQualityAudio - Demonstrate high-quality audio output
 *
 * This example shows how to use low jitter mode and high
 * sample rates for audiophile-grade audio output.
 *
 * Features:
 * - Low jitter mode for minimal clock jitter
 * - 96kHz sample rate
 * - 24-bit audio depth (if your DAC supports it)
 *
 * IMPORTANT: When using low jitter mode, initialize I2S
 * before any other peripherals (Serial, SPI, etc.)
 */

#include <PicoI2SPIO.h>

// For 24-bit audio, we need to pack data properly
void pack24bit(uint8_t* dest, int32_t sample) {
  dest[0] = (sample >> 8) & 0xFF;   // Low byte
  dest[1] = (sample >> 16) & 0xFF;  // Mid byte
  dest[2] = (sample >> 24) & 0xFF;  // High byte
}

void setup() {
  // Initialize I2S FIRST with low jitter mode
  // This must be done before Serial.begin()!

  // Advanced initialization with low jitter mode
  bool success = I2S.beginAdvanced(
    18,                      // data_pin
    20,                      // clock_pin_base (LRCLK)
    22,                      // mclk_pin
    pio0,                    // PIO instance
    0,                       // State machine
    0,                       // DMA channel
    false,                   // Don't use core1
    CLOCK_MODE_LOW_JITTER,   // Low jitter mode!
    MODE_I2S,                // Standard I2S
    96000,                   // 96kHz sample rate
    24                       // 24-bit depth
  );

  // NOW we can initialize Serial
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  if (!success) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  Serial.println("High Quality Audio Example");
  Serial.println("Mode: Low Jitter");
  Serial.println("Sample Rate: 96kHz");
  Serial.println("Bit Depth: 24-bit");
  Serial.println("System clock adjusted for integer MCLK division");

  // Set initial volume to -3dB for headroom
  I2S.setVolumeDB(-3);
}

void loop() {
  static float phase1 = 0.0;
  static float phase2 = 0.0;
  static float phase3 = 0.0;

  // Generate rich harmonic content with multiple sine waves
  // This demonstrates the improved clarity of high-quality mode
  uint8_t buffer[576];  // 96 samples * 2 channels * 3 bytes

  for (int i = 0; i < 96; i++) {
    // Create complex waveform with harmonics
    float fundamental = sin(phase1) * 0.4;
    float second = sin(phase2) * 0.2;
    float third = sin(phase3) * 0.1;

    float sample = fundamental + second + third;

    // Convert to 24-bit
    int32_t value24 = (int32_t)(sample * 8388607.0f) << 8;

    // Pack for left channel
    pack24bit(&buffer[i * 6], value24);
    // Pack for right channel (same signal)
    pack24bit(&buffer[i * 6 + 3], value24);

    // Update phases (440Hz, 554Hz, 659Hz - A major chord)
    phase1 += 2.0 * PI * 440.0 / 96000.0;
    phase2 += 2.0 * PI * 554.37 / 96000.0;
    phase3 += 2.0 * PI * 659.25 / 96000.0;

    if (phase1 > 2.0 * PI) phase1 -= 2.0 * PI;
    if (phase2 > 2.0 * PI) phase2 -= 2.0 * PI;
    if (phase3 > 2.0 * PI) phase3 -= 2.0 * PI;
  }

  // Write 24-bit samples
  if (!I2S.write(buffer, sizeof(buffer))) {
    delayMicroseconds(100);
  }

  // In low jitter mode, timing is critical
  // Avoid Serial prints in the main loop
}