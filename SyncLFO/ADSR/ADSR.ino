/**
 * @file ADSR.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief ADSR Envelope Generator firmware for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.1
 * @date 2023-05-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <avr/io.h>

// GPIO Pin mapping.
#define P1 0  // Attack
#define P2 1  // Decay
#define P3 3  // Sustain
#define P4 5  // Release

#define GATE_IN 3  // Gate in / Re-trig
#define CV_OUT 10  // Envelope Output

#define DEBUG

bool gate = 1;  // External gate input detect: 0=gate off, 1=gate on
bool old_gate = 0;

const byte top = 255;  // Envelope state max value
byte val = 0;          // Envelope state value
int time = 0;          // Envelope delay time between incremental change
int sustain = 0;       // Sustain value;

// DEBUG
unsigned long last_print;
unsigned long period = 200;

enum Stage {
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
};
Stage stage = ATTACK;

void setup() {
    Serial.begin(9600);

    pinMode(GATE_IN, INPUT);  // Gate in
    pinMode(CV_OUT, OUTPUT);  // Envelope cv out

    // Register setting for high frequency PWM.
    TCCR1A = 0b00100001;
    TCCR1B = 0b00100001;

    delay(100);
}

void loop() {
    // Check gate input.
    old_gate = gate;
    gate = digitalRead(GATE_IN);

    // Detect if gate has just opened and begin envelope attack.
    if (old_gate == 0 && gate == 1) {
        stage = ATTACK;
        val = 0;
    }

    // Detect if gate has just closed and begin the release envelope.
    if (old_gate == 1 && gate == 0) {
        stage = RELEASE;
    }

    // Advance the cv value for the current envelope stage according to the related stage knob position.
    switch (stage) {
        case ATTACK:
            // At minimum attack levels, traverse the curve at a faster rate than default.
            val += analogRead(P1) == 0 ? min(top - val, 10) : 1;

            if (val >= top) {
                stage = DECAY;
            }
            break;

        case DECAY:
            // Decrease the envelope value if it's still falling.
            if (val > 0) {
                // At minimum release levels, traverse the curve at a faster rate than default.
                val -= analogRead(P2) == 0 ? min(val, 10) : 1;
            }

            // Check if the falling decay envelope has reached sustain level.
            sustain = min(map(analogRead(P3), 0, 1023, 0, top), val);
            if (val <= sustain) {
                stage = SUSTAIN;
            }
            break;

        case SUSTAIN:
            val = map(analogRead(P3), 0, 1023, 0, top);
            break;

        case RELEASE:
            // Decrease the envelope value if it's still falling.
            if (val > 0) {
                // At minimum release levels, traverse the curve at a faster rate than default.
                val -= analogRead(P4) == 0 ? min(val, 10) : 1;
            }
            break;
    }

    // Attack / Decay / Release incremental delay time.
    switch (stage) {
        case ATTACK:
            time = analogRead(P1);
            break;
        case DECAY:
            time = analogRead(P2);
            break;
        case SUSTAIN:
            time = 1000;  // Default time for Sustain.
            break;
        case RELEASE:
            time = analogRead(P4);
            break;
    }

    // Short sleep duration before advancing to the next step in the curve table.
    delayMicroseconds(time * 10);

    // Write envelope CV output
    analogWrite(CV_OUT, val);

    debug();
}

void debug() {
#ifdef DEBUG
    // Only print debug message once per elapsed period duration.
    if (millis() - last_print > period) {
        last_print = millis();
        Serial.println(
            "val:   " + String(val)             // Print all env state vars
            + "\tA: " + String(analogRead(P1))  //
            + "\tD: " + String(analogRead(P2))  //
            + "\tS: " + String(analogRead(P3))  //
            + "\tR: " + String(analogRead(P4))  //
            + "\tstage: " + String(stage)       //
        );
    }
#endif
}