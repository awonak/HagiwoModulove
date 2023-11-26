/**
 * @file PerlinNoise.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Smooth random voltage to chaotic noise with various logic input modes
 * @version 0.1
 * @date 2023-11-03
 *
 * @copyright Copyright (c) 2023
 *
 * Smooth random voltages to chaotic noise using the Perlin noise algorithm.
 * Unlike random() in which each new random value has no correlation to it's
 * previous value, Perlin noise has a more organic appearance because it
 * produces a naturally ordered “smooth” sequence of pseudo-random numbers.
 *
 * Additionally, the random values produced by the Perlin noise algorithm
 * adhere to a bell curve distribution, meaning the OUT cv will hover around
 * 5v and the BI cv output will hover around 0v.
 *
 *  Knob 1 - Offset: Increase or decrease the center point the voltage hovers around.
 *
 *  Knob 2 - Frequency: the rate at which the value changes.
 *
 *  Knob 3 - Bit Crush: Reduce the bit depth of the output.
 *
 *  Knob 4 - Input mode: Open, Sample & Hold, Track & Hold, Gate.
 *
 *
 * Input Trigger/Gate Modes:
 *
 * OPEN - The output will continually produce random voltages regardless of CLK input.
 *
 * SAMPLE_HOLD - Produce a new stable random voltage upon each trigger input.
 *
 * TRACK_HOLD - Track and Hold, output the random voltage and hold the value upon gate input.
 *
 * GATE - Also known as Hold and Track, only output voltage while the Gate input is high.
 *
 */

#include <FastLED.h>

// GPIO Pin mapping.
#define P1 0  // offset
#define P2 1  // frequency
#define P3 3  // bit depth / bitcrusher
#define P4 5  // input logic mode

#define GATE_IN 3  // Trigger/Gate input for logic modes
#define CV_OUT 10  // CV Output for random voltage

// Exponential curve factors for frequency and noise buffer.
const float noiseReadFactor = 0.0098;

bool gate = 1;  // External gate input detect: 0=gate off, 1=gate on
bool old_gate = 0;
int nx = 0;             // Perlin noise buffer x read value
int seed = random16();  // Start with a new random Perlin noise Y-axis each time.
byte depth = 0;         // Bitcrush depth.
byte val = 0;           // current value from the perlin noise algorithm
byte hold = 0;          // held output value for s&h / t&h

unsigned long frequencyInterval = 1 << 14;  // Frequency timer (increase for slower rate)
unsigned long previousTime = 0;             // Store last time Frequency was updated

enum InputMode {
    OPEN,
    SAMPLE_HOLD,
    TRACK_HOLD,
    GATE
};
InputMode mode = OPEN;

void setup() {
    pinMode(GATE_IN, INPUT);  // Gate/Trigger in
    pinMode(CV_OUT, OUTPUT);  // CV output

    // Register setting for high frequency PWM.
    TCCR1A = 0b00100001;
    TCCR1B = 0b00100001;
}

void loop() {
    // Detect current selected input mode.
    InputMode inputMode = readMode();

    // Read gate input.
    old_gate = gate;
    gate = digitalRead(GATE_IN);

    // Detect if gate has just opened.
    if (old_gate == 0 && gate == 1) {
        switch (inputMode) {
            case SAMPLE_HOLD:
            case TRACK_HOLD:
                hold = val;
        }
    }
    // Detect if gate has just closed.
    if (old_gate == 1 && gate == 0) {
        if (inputMode == GATE) {
            hold = 0;
        }
    }

    // Frequency or rate of change.
    long offset =  map(analogRead(P1), 0, 1024, -63, 63);
    unsigned long currentTime = micros();

    if (currentTime - previousTime > frequencyInterval) {
        // Calculate the output value.
        nx += pow(2, analogRead(P2) * noiseReadFactor);
        val = inoise8(nx, seed);
        if (offset > 0) {
            val = constrain(val + offset, 0, 255);
        } else {
            val = (val > abs(offset)) ? val + offset : 0;
        }

        // Reduce the bit depth of the output value.
        depth = analogRead(P3) >> 7;
        val = bitcrush(val, depth);

        // Set next frequency update deadline.
        previousTime = currentTime;
        // Determine the output value based on the current selected input mode.
        analogWrite(CV_OUT, output(inputMode));
    }
}

int bitcrush(int val, byte depth) {
    if (depth == 0) {
        return val;
    }
    return (val >> depth) * pow(2, depth);
}

InputMode readMode() {
    switch (analogRead(P4) >> 8) {
        case 0:
            return OPEN;
        case 1:
            return SAMPLE_HOLD;
        case 2:
            return TRACK_HOLD;
        case 3:
            return GATE;
    }
}

byte output(InputMode inputMode) {
    switch (inputMode) {
        case OPEN:
            return val;
        case SAMPLE_HOLD:
            return hold;
        case TRACK_HOLD:
            return (gate == 1) ? hold : val;
        case GATE:
            return (gate == 1) ? val : 0;
    }
}