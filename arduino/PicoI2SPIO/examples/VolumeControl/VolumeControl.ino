/*
 * VolumeControl - Demonstrate volume control and fading
 *
 * This example shows how to control volume and create
 * smooth volume fades with the PicoI2SPIO library.
 *
 * The example plays a tone that fades in and out continuously.
 */

#include <PicoI2SPIO.h>

const float FREQUENCY = 440.0;
int16_t buffer[256];
float phase = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("PicoI2SPIO Volume Control Example");

  // Initialize I2S
  if (!I2S.begin()) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  Serial.println("Starting volume fade demo...");
}

void generateTone() {
  for (int i = 0; i < 128; i++) {
    float sample = sin(phase) * 0.8;  // 80% amplitude
    int16_t value = I2S.floatToInt16(sample);

    buffer[i * 2] = value;
    buffer[i * 2 + 1] = value;

    phase += 2.0 * PI * FREQUENCY / 48000.0;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
  }
}

void loop() {
  static int fadeDirection = 1;
  static int volume = 0;
  static unsigned long lastFadeTime = 0;

  // Update volume every 30ms for smooth fade
  if (millis() - lastFadeTime > 30) {
    lastFadeTime = millis();

    volume += fadeDirection * 2;

    if (volume >= 100) {
      volume = 100;
      fadeDirection = -1;
      Serial.println("Max volume reached, fading out...");
    } else if (volume <= 0) {
      volume = 0;
      fadeDirection = 1;
      Serial.println("Muted, fading in...");
    }

    // Set volume (0-100%)
    I2S.setVolume(volume);

    // Alternative: Set volume in dB
    // int db = map(volume, 0, 100, -40, 0);  // -40dB to 0dB range
    // I2S.setVolumeDB(db);
  }

  // Generate and play tone
  generateTone();

  // Write with buffer check
  if (I2S.availableForWrite() >= sizeof(buffer)) {
    I2S.write(buffer, 256);
  } else {
    delay(1);
  }
}