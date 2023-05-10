/**
 * @file Baby4.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief 4 step cv sequencer firmware for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.1
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <avr/io.h>

// GPIO Pin mapping.
#define P1 0  // Step 1
#define P2 1  // Step 2
#define P3 3  // Step 3
#define P4 5  // Step 4

#define TRIG_IN 3  // Trigger Input to advance step
#define CV_OUT 10  // CV Output for current step

bool trig = 1;  // External trigger input detect
bool old_trig = 0;

const byte P[] = {P1, P2, P3, P4};  // Array of knob GPIO pin identifiers.

const byte max_val = 255;  // CV step max value
const byte max_step = 4;   // Max steps in the CV sequence (corresponding to number of knobs)

byte val = 0;   // Current CV value
byte step = 0;  // Current CV step (0,1,2,3)
int read = 0;   // Raw cv read from pot for current step

void setup() {
    pinMode(TRIG_IN, INPUT);  // Trigger in
    pinMode(CV_OUT, OUTPUT);  // Current cv step out

    // Register setting for high frequency PWM.
    TCCR1A = 0b00100001;
    TCCR1B = 0b00100001;

    delay(100);
}

void loop() {
    old_trig = trig;
    trig = digitalRead(TRIG_IN);

    // Detect if new trigger received and advance step.
    if (old_trig == 0 && trig == 1) {
        step = (step + 1) % max_step;
    }

    // Write current step CV output.
    val = map(analogRead(P[step]), 0, 1023, 0, max_val);
    analogWrite(CV_OUT, val);
}
