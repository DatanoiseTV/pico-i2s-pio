/*
 * StereoTest - Test stereo separation and channel control
 *
 * This example alternates a tone between left and right channels
 * to verify stereo separation and channel routing.
 *
 * You should hear:
 * - 440Hz tone in left channel for 1 second
 * - 880Hz tone in right channel for 1 second
 * - Both tones together for 1 second
 * - Silence for 1 second
 */

#include <PicoI2SPIO.h>

float phaseL = 0.0;
float phaseR = 0.0;

enum TestMode {
  LEFT_ONLY,
  RIGHT_ONLY,
  BOTH_CHANNELS,
  SILENCE
};

TestMode currentMode = LEFT_ONLY;
unsigned long modeStartTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("PicoI2SPIO Stereo Test");

  // Initialize I2S with custom pins if needed
  // I2S.begin(data_pin, clock_base_pin, mclk_pin, sample_rate, bit_depth);
  if (!I2S.begin(48000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  Serial.println("Starting stereo test sequence...");
  Serial.println("Listen for alternating L/R channels");

  modeStartTime = millis();
}

void loop() {
  // Switch mode every second
  if (millis() - modeStartTime > 1000) {
    modeStartTime = millis();
    currentMode = (TestMode)((currentMode + 1) % 4);

    switch (currentMode) {
      case LEFT_ONLY:
        Serial.println("Left channel: 440Hz");
        I2S.setVolume(100, 0);  // Left 100%, Right 0%
        break;
      case RIGHT_ONLY:
        Serial.println("Right channel: 880Hz");
        I2S.setVolume(0, 100);  // Left 0%, Right 100%
        break;
      case BOTH_CHANNELS:
        Serial.println("Both channels: 440Hz + 880Hz");
        I2S.setVolume(100, 100);  // Both 100%
        break;
      case SILENCE:
        Serial.println("Silence");
        I2S.setVolume(0, 0);  // Mute both
        break;
    }
  }

  // Generate stereo samples
  int16_t samples[256];
  for (int i = 0; i < 128; i++) {
    // Left channel - 440Hz (A4)
    float sampleL = sin(phaseL) * 0.5;
    phaseL += 2.0 * PI * 440.0 / 48000.0;
    if (phaseL > 2.0 * PI) phaseL -= 2.0 * PI;

    // Right channel - 880Hz (A5)
    float sampleR = sin(phaseR) * 0.5;
    phaseR += 2.0 * PI * 880.0 / 48000.0;
    if (phaseR > 2.0 * PI) phaseR -= 2.0 * PI;

    samples[i * 2] = I2S.floatToInt16(sampleL);
    samples[i * 2 + 1] = I2S.floatToInt16(sampleR);
  }

  // Write samples
  while (!I2S.write(samples, 256)) {
    delay(1);
  }
}