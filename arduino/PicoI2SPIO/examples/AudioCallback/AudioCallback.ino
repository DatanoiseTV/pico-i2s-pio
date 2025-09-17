/*
 * AudioCallback - Generate audio using callback functions
 *
 * This example demonstrates three different callback methods:
 * 1. Integer callback (int16_t)
 * 2. Float callback (separate L/R channels)
 * 3. Manual callback processing
 *
 * The callback is called automatically when the I2S buffer
 * needs more audio data, making it perfect for synthesizers,
 * audio effects, and real-time audio generation.
 */

#include <PicoI2SPIO.h>

// Audio parameters
const float SAMPLE_RATE = 48000.0;
float phase = 0.0;
float frequency = 440.0;

// Example 1: Integer callback (interleaved stereo)
void audioCallback16(int16_t* buffer, size_t frames) {
  for (size_t i = 0; i < frames; i++) {
    float sample = sin(phase) * 0.5;
    int16_t value = I2S.floatToInt16(sample);

    buffer[i * 2] = value;      // Left channel
    buffer[i * 2 + 1] = value;  // Right channel

    phase += 2.0 * PI * frequency / SAMPLE_RATE;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
  }
}

// Example 2: Float callback (separate channels)
void audioCallbackFloat(float* left, float* right, size_t frames) {
  static float phaseL = 0.0;
  static float phaseR = 0.0;

  for (size_t i = 0; i < frames; i++) {
    // Different frequencies for L/R
    left[i] = sin(phaseL) * 0.5;   // 440Hz
    right[i] = sin(phaseR) * 0.5;  // 554Hz (C#)

    phaseL += 2.0 * PI * 440.0 / SAMPLE_RATE;
    phaseR += 2.0 * PI * 554.37 / SAMPLE_RATE;

    if (phaseL > 2.0 * PI) phaseL -= 2.0 * PI;
    if (phaseR > 2.0 * PI) phaseR -= 2.0 * PI;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("Audio Callback Example");

  // Initialize I2S
  if (!I2S.begin(48000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  // Choose which callback style to use:
  // Uncomment ONE of the following:

  // Option 1: Integer callback (most common)
  I2S.setCallback(audioCallback16);

  // Option 2: Float callback (easier for DSP)
  // I2S.setCallbackFloat(audioCallbackFloat);

  // Start automatic callback processing
  I2S.startCallback();

  Serial.println("Callback started - generating audio");
  Serial.println("Press any key to switch callback modes");
}

void loop() {
  static int mode = 0;

  // Process the callback (fills buffer when needed)
  if (I2S.isCallbackActive()) {
    I2S.processCallback();
  }

  // Switch between callback modes on serial input
  if (Serial.available()) {
    Serial.read();  // Clear the input
    mode = (mode + 1) % 3;

    switch (mode) {
      case 0:
        Serial.println("Mode 0: Integer callback (mono)");
        I2S.setCallback(audioCallback16);
        frequency = 440.0;
        break;

      case 1:
        Serial.println("Mode 1: Float callback (stereo)");
        I2S.setCallbackFloat(audioCallbackFloat);
        break;

      case 2:
        Serial.println("Mode 2: Manual processing (sweep)");
        I2S.stopCallback();
        I2S.setCallback(nullptr);  // Clear callback
        break;
    }

    if (mode < 2) {
      I2S.startCallback();
    }
  }

  // Mode 2: Manual audio generation (frequency sweep)
  if (mode == 2 && !I2S.isCallbackActive()) {
    static float sweepPhase = 0.0;
    static float sweepFreq = 100.0;
    static unsigned long lastSweep = 0;

    // Sweep frequency
    if (millis() - lastSweep > 10) {
      lastSweep = millis();
      sweepFreq += 5.0;
      if (sweepFreq > 2000.0) sweepFreq = 100.0;
    }

    // Generate samples manually
    int16_t buffer[256];
    for (int i = 0; i < 128; i++) {
      float sample = sin(sweepPhase) * 0.3;
      int16_t value = I2S.floatToInt16(sample);

      buffer[i * 2] = value;
      buffer[i * 2 + 1] = value;

      sweepPhase += 2.0 * PI * sweepFreq / SAMPLE_RATE;
      if (sweepPhase > 2.0 * PI) sweepPhase -= 2.0 * PI;
    }

    I2S.write(buffer, 256);
  }

  // Small delay to prevent CPU hogging
  delay(1);
}