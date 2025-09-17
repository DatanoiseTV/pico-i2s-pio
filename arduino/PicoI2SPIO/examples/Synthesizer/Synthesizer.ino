/*
 * Synthesizer - A simple polyphonic synthesizer using callbacks
 *
 * This example creates a simple synthesizer with:
 * - Multiple oscillators
 * - ADSR envelope
 * - Low-pass filter
 * - Polyphonic voices
 *
 * Use Serial Monitor to trigger notes
 */

#include <PicoI2SPIO.h>

const float SAMPLE_RATE = 48000.0;
const int MAX_VOICES = 4;

// Oscillator types
enum OscType {
  OSC_SINE,
  OSC_SQUARE,
  OSC_SAW,
  OSC_TRIANGLE
};

// ADSR Envelope
struct Envelope {
  float attack = 0.01;   // seconds
  float decay = 0.1;     // seconds
  float sustain = 0.7;   // level (0-1)
  float release = 0.5;   // seconds

  float level = 0.0;
  float time = 0.0;
  bool gate = false;
  bool active = false;

  float process() {
    const float dt = 1.0 / SAMPLE_RATE;

    if (gate) {
      if (time < attack) {
        // Attack phase
        level = time / attack;
        time += dt;
      } else if (time < attack + decay) {
        // Decay phase
        float decayTime = time - attack;
        level = 1.0 - (1.0 - sustain) * (decayTime / decay);
        time += dt;
      } else {
        // Sustain phase
        level = sustain;
      }
      active = true;
    } else if (active) {
      // Release phase
      if (level > 0.001) {
        level *= exp(-dt / release);
      } else {
        level = 0.0;
        active = false;
        time = 0.0;
      }
    }

    return level;
  }

  void trigger() {
    gate = true;
    time = 0.0;
  }

  void release_key() {
    gate = false;
  }
};

// Voice structure
struct Voice {
  float frequency = 0.0;
  float phase = 0.0;
  Envelope env;
  bool active = false;
  OscType osc_type = OSC_SINE;

  float generate() {
    if (!active && !env.active) return 0.0;

    float sample = 0.0;

    // Generate oscillator
    switch (osc_type) {
      case OSC_SINE:
        sample = sin(phase);
        break;

      case OSC_SQUARE:
        sample = (phase < PI) ? 0.8 : -0.8;
        break;

      case OSC_SAW:
        sample = (phase / PI) - 1.0;
        break;

      case OSC_TRIANGLE:
        if (phase < PI) {
          sample = (phase / PI) * 2.0 - 1.0;
        } else {
          sample = 1.0 - ((phase - PI) / PI) * 2.0;
        }
        break;
    }

    // Apply envelope
    sample *= env.process();

    // Update phase
    phase += 2.0 * PI * frequency / SAMPLE_RATE;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;

    return sample;
  }

  void noteOn(float freq) {
    frequency = freq;
    active = true;
    env.trigger();
  }

  void noteOff() {
    env.release_key();
    active = false;
  }
};

// Global synthesizer state
Voice voices[MAX_VOICES];
float masterVolume = 0.3;
float filterCutoff = 0.8;

// Simple low-pass filter
float lowpass(float input, float cutoff) {
  static float output = 0.0;
  float alpha = cutoff;
  output = output + alpha * (input - output);
  return output;
}

// Audio callback
void synthCallback(float* left, float* right, size_t frames) {
  for (size_t i = 0; i < frames; i++) {
    float sample = 0.0;

    // Mix all voices
    for (int v = 0; v < MAX_VOICES; v++) {
      sample += voices[v].generate();
    }

    // Apply filter
    sample = lowpass(sample, filterCutoff);

    // Apply master volume
    sample *= masterVolume;

    // Soft clipping
    if (sample > 1.0) sample = tanh(sample);
    if (sample < -1.0) sample = tanh(sample);

    // Output to both channels
    left[i] = sample;
    right[i] = sample;
  }
}

// Note frequencies (C4 to B4)
float noteFreqs[] = {
  261.63,  // C4
  277.18,  // C#4
  293.66,  // D4
  311.13,  // D#4
  329.63,  // E4
  349.23,  // F4
  369.99,  // F#4
  392.00,  // G4
  415.30,  // G#4
  440.00,  // A4
  466.16,  // A#4
  493.88   // B4
};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("Simple Synthesizer");
  Serial.println("Commands:");
  Serial.println("  1-9, 0, -, = : Play notes C4 to B4");
  Serial.println("  q : All notes off");
  Serial.println("  s : Sine wave");
  Serial.println("  w : Square wave");
  Serial.println("  a : Saw wave");
  Serial.println("  t : Triangle wave");
  Serial.println("  [ ] : Adjust filter cutoff");
  Serial.println("  < > : Adjust volume");

  // Initialize I2S
  if (!I2S.begin(48000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  // Set up synthesizer callback
  I2S.setCallbackFloat(synthCallback);
  I2S.startCallback();

  // Initialize voices with different oscillator types
  for (int i = 0; i < MAX_VOICES; i++) {
    voices[i].osc_type = (OscType)(i % 4);
  }

  Serial.println("Synthesizer ready!");
}

int nextVoice = 0;

void loop() {
  // Process audio callback
  I2S.processCallback();

  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();

    // Note triggers
    if (cmd >= '1' && cmd <= '9') {
      int note = cmd - '1';
      voices[nextVoice].noteOn(noteFreqs[note]);
      nextVoice = (nextVoice + 1) % MAX_VOICES;
      Serial.print("Note: ");
      Serial.println(noteFreqs[note]);
    }
    else if (cmd == '0') {
      voices[nextVoice].noteOn(noteFreqs[9]);  // A4
      nextVoice = (nextVoice + 1) % MAX_VOICES;
    }
    else if (cmd == '-') {
      voices[nextVoice].noteOn(noteFreqs[10]); // A#4
      nextVoice = (nextVoice + 1) % MAX_VOICES;
    }
    else if (cmd == '=') {
      voices[nextVoice].noteOn(noteFreqs[11]); // B4
      nextVoice = (nextVoice + 1) % MAX_VOICES;
    }
    // All notes off
    else if (cmd == 'q') {
      for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].noteOff();
      }
      Serial.println("All notes off");
    }
    // Oscillator types
    else if (cmd == 's') {
      for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].osc_type = OSC_SINE;
      }
      Serial.println("Sine wave");
    }
    else if (cmd == 'w') {
      for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].osc_type = OSC_SQUARE;
      }
      Serial.println("Square wave");
    }
    else if (cmd == 'a') {
      for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].osc_type = OSC_SAW;
      }
      Serial.println("Saw wave");
    }
    else if (cmd == 't') {
      for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].osc_type = OSC_TRIANGLE;
      }
      Serial.println("Triangle wave");
    }
    // Filter control
    else if (cmd == '[') {
      filterCutoff = max(0.1, filterCutoff - 0.1);
      Serial.print("Filter: ");
      Serial.println(filterCutoff);
    }
    else if (cmd == ']') {
      filterCutoff = min(1.0, filterCutoff + 0.1);
      Serial.print("Filter: ");
      Serial.println(filterCutoff);
    }
    // Volume control
    else if (cmd == '<' || cmd == ',') {
      masterVolume = max(0.0, masterVolume - 0.05);
      Serial.print("Volume: ");
      Serial.println(masterVolume);
    }
    else if (cmd == '>' || cmd == '.') {
      masterVolume = min(1.0, masterVolume + 0.05);
      Serial.print("Volume: ");
      Serial.println(masterVolume);
    }
  }

  delay(1);
}