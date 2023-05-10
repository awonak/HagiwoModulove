/**
 * @file Polyrhythm.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Generate polyrhythms based on the four 16 step beat counter knobs for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.1
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <avr/io.h>

// GPIO Pin mapping.
#define P1 0  // Polyrhythm count 1
#define P2 1  // Polyrhythm count 2
#define P3 3  // Polyrhythm count 3
#define P4 5  // Polyrhythm count 4

#define TRIG_IN 3  // Trigger Input to advance step
#define CV_OUT 10  // CV Output for current step

// Uncomment to print state to serial monitoring output.
// #define DEBUG

bool trig = 1;  // External trigger input detect
bool old_trig = 0;

byte P[] = {P1, P2, P3, P4};  // Array of knob GPIO identifiers.
byte S[] = {0, 0, 0, 0};      // Polyrhythm step choice per knob.
byte C[] = {0, 0, 0, 0};      // Polyrhythm counter per knob.

byte read;  // Raw read from analog input.
bool hit;   // Indicator if this step is triggerd by any polyrhythm setting.

void setup() {
    Serial.begin(9600);
    pinMode(TRIG_IN, INPUT);  // Trigger in
    pinMode(CV_OUT, OUTPUT);  // Polyrhythm trigger cv output

    // Initialize rhythm and counter arrays.
    update_polyrhthms();

    // Register setting for high frequency PWM.
    TCCR1A = 0b00100001;
    TCCR1B = 0b00100001;

    delay(100);
}

void loop() {
    old_trig = trig;
    trig = digitalRead(TRIG_IN);

    // Detect if new trigger received and advance counter.
    if (old_trig == 0 && trig == 1) {
        hit = advance_counters();
        digitalWrite(CV_OUT, hit);
        debug();
    }

    // Detect if trigger has just ended and turn off the trigger cv output.
    if (old_trig == 1 && trig == 0) {
        digitalWrite(CV_OUT, LOW);
    }

    update_polyrhthms();
}

// Advance the counter for each polyrhythm and return true if any counter reaches its polyrhythm step.
bool advance_counters() {
    hit = false;
    for (byte i = 0; i < 4; i++) {
        if (S[i] == 0) {
            continue;
        }
        if (C[i] >= S[i]) {
            C[i] = 0;
            hit = true;
        }
        C[i]++;
    }
    return hit;
}

// Check each polyrhythm step knob for a change and update the step and counter values.
void update_polyrhthms() {
    for (byte i = 0; i < 4; i++) {
        read = map(analogRead(P[i]), 0, 1023, 0, 16);
        if (read != S[i]) {
            S[i] = read;
            C[i] = read;
        }
    }
}

void debug() {
#ifdef DEBUG
    Serial.println(
        "Hit:   " + String(hit)                       // Print all state vars
        + "\tS1: " + String(S[0]) + "|" + String(C[0])  //
        + "\tS2: " + String(S[1]) + "|" + String(C[1])  //
        + "\tS3: " + String(S[2]) + "|" + String(C[2])  //
        + "\tS4: " + String(S[3]) + "|" + String(C[3])  //
    );
#endif
}