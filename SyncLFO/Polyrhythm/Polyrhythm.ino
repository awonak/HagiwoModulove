/**
 * @file Polyrhythm.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Generate polyrhythms based on the four 16 step counter knobs for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.1
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 * Each knob acts as a subdivision of the incoming clock. When a rhythm knob
 * is fully CCW (0) no rhythm is set. Moving the rhythm knob CW, the first
 * subdivision is every 16 beats. When the knob is fully CW, that is a
 * subdivision of 1 and the polyrhythm will trigger every beat.
 *
 * When in default OR mode, any beat that has more than one rhythm trigger
 * will output an accented 5v, otherwise a single rhythm trigger will output
 * 3v. XOR mode will output 3v triggers when only one rhythm triggers on the
 * beat.
 *
 * There are some flags to change the behavior of the script.
 *
 * OXR - The default behavior for overlapping rhythms is to OR, meaning the
 *       polyrhythm will trigger if any of the rhythms hits on the current
 *       beat. Enabling XOR will
 *
 * RESET - Enablign this will cause the each rhythm counter to reset when any
 *         one of the rhythms changes. This ensures all rhythms will count from
 *         the same starting point.
 *
 */

#include <avr/io.h>

// GPIO Pin mapping.
#define P1 0  // Polyrhythm count 1
#define P2 1  // Polyrhythm count 2
#define P3 3  // Polyrhythm count 3
#define P4 5  // Polyrhythm count 4

#define TRIG_IN 3  // Trigger Input to advance the rhythm counter
#define CV_OUT 10  // CV Output for polyrhythm triggers

// Define constant voltage values for analog cv output.
#define CV_3V 77
#define CV_5V 128

//
// Flags for changing behavior of the script.

// Flag for overriding OR overlapping hit behavior with XOR.
const bool XOR = false;

// Flag for resetting all polyrhythm counters when any one changes.
const bool RESET = false;

// Flag for enabling debug print to serial monitoring output.
const bool DEBUG = false;

//
// Script state variables.
bool trig = 1;  // External trigger input detect
bool old_trig = 0;

byte P[] = {P1, P2, P3, P4};  // Array of knob GPIO identifiers.
byte R[] = {0, 0, 0, 0};      // Polyrhythm rhythm subdivision choice per knob.
byte C[] = {0, 0, 0, 0};      // Polyrhythm counter per knob.

byte cv, hits;

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
        hits = advance_counters();
        // XOR mode enabled will only trigger if one and only one rhythm hits on this beat.
        if (XOR) {
            cv = (hits == 1) ? CV_3V : 0;
        }
        // 3v for only one rhythm hit on this beat.
        else if (hits == 1) {
            cv = CV_3V;
        }
        // 5v accent for more than one rhythm hit on this beat.
        else if (hits > 1) {
            cv = CV_5V;
        } else {
            cv = 0;
        }
        analogWrite(CV_OUT, cv);

        debug();
    }

    // Detect if trigger has just ended and turn off the trigger cv output.
    if (old_trig == 1 && trig == 0) {
        analogWrite(CV_OUT, 0);
    }

    update_polyrhthms();
}

// Advance the counter for each polyrhythm and return the number rhythm triggers on this beat.
byte advance_counters() {
    byte hits = 0;
    for (byte i = 0; i < 4; i++) {
        if (R[i] == 0) {
            continue;
        }
        C[i]++;
        if (C[i] >= R[i]) {
            C[i] = 0;
            hits++;
        }
    }
    return hits;
}

// Check each rhythm knob for a change and update the rhythm and counter values.
void update_polyrhthms() {
    bool reset = false;
    int raw, rhythm;
    for (byte i = 0; i < 4; i++) {
        raw = map(analogRead(P[i]), 0, 1023, 0, 16);
        // When rhythm is 0 CCW, no rhythm is set, otherwise rhythm is
        // triggered every 16 beats to every 1 beat moving CW.
        rhythm = (raw > 0) ? 17 - raw : 0;
        if (rhythm != R[i]) {
            R[i] = rhythm;
            C[i] = 0;
            reset = true;
        }
    }
    if (reset and RESET) {
        for (byte i = 0; i < 4; i++) {
            if (rhythm != R[i]) {
                C[i] = 0;
            }
        }
    }
}

void debug() {
    if (DEBUG) {
        Serial.println(
            "Out CV:   " + String(cv)                       // Print all state vars
            + "\tS1: " + String(C[0]) + ">" + String(R[0])  //
            + "\tS2: " + String(C[1]) + ">" + String(R[1])  //
            + "\tS3: " + String(C[2]) + ">" + String(R[2])  //
            + "\tS4: " + String(C[3]) + ">" + String(R[3])  //
        );
    }
}