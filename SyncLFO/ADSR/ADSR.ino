/**
 * @file ADSR.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief ADSR Envelope Generator firmware for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.2
 * @date 2023-05-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <synclfo.h>

// ALTERNATE HARDWARE CONFIGURATION
#define SYNCHRONIZER

// Debug flag
// #define DEBUG

using namespace modulove;
using namespace synclfo;

// Declare SyncLFO hardware variable.
SyncLFO hw;

int val = 0;      // Envelope state value
int time = 0;     // Envelope delay time between incremental change
int sustain = 0;  // Sustain value;

// DEBUG
unsigned long last_print;
unsigned long period = 200;

enum Stage {
    WAIT,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
};
Stage stage = WAIT;

void setup() {
#ifdef SYNCHRONIZER
    hw.config.Synchronizer = true;
#endif

    // Initialize the SyncLFO peripherials.
    hw.Init();

#ifdef DEBUG
    Serial.begin(115200);
#endif
}

void loop() {
    // Read cv inputs to determine state for this loop.
    hw.ProcessInputs();

    bool gate_rising = hw.gate.State() == DigitalInput::STATE_RISING;
    bool gate_falling = hw.gate.State() == DigitalInput::STATE_FALLING;
    bool gate_high = hw.gate.On();

    if (hw.config.Synchronizer) {
        gate_rising |= hw.b1.Change() == Button::CHANGE_PRESSED;
        gate_falling |= hw.b1.Change() == Button::CHANGE_RELEASED;
        gate_high |= hw.b1.On();
    }

    // Detect if gate has just opened and begin envelope attack.
    if (gate_rising) {
        stage = ATTACK;
        val = 0;
    }

    // Detect if gate has just closed and begin the release envelope.
    if (gate_falling) {
        stage = RELEASE;
        gate_high = false;
    }

    // Advance the cv value for the current envelope stage according to the related stage knob position.
    switch (stage) {
        case ATTACK:
            // At minimum attack levels, traverse the curve at a faster rate than default.
            val += hw.p1.Read() == 0 ? min(MAX_INPUT - val, 10) : 1;

            if (val >= MAX_INPUT) {
                stage = DECAY;
            }
            break;

        case DECAY:
            // Decrease the envelope value if it's still falling.
            if (val > 0) {
                // At minimum release levels, traverse the curve at a faster rate than default.
                val -= hw.p2.Read() == 0 ? min(val, 10) : 1;
            }

            // Check if the falling decay envelope has reached sustain level.
            sustain = min(hw.p3.Read(), val);
            if (val <= sustain && gate_high) {
                stage = SUSTAIN;
            } else if (!gate_high) {
                stage = RELEASE;
            }
            break;

        case SUSTAIN:
            val = hw.p3.Read();
            break;

        case RELEASE:
            // Decrease the envelope value if it's still falling.
            if (val > 0) {
                // At minimum release levels, traverse the curve at a faster rate than default.
                val -= hw.p4.Read() == 0 ? min(val, 10) : 1;
            } else {
                stage = WAIT;
            }
            break;
    }

    // Attack / Decay / Release incremental delay time.
    switch (stage) {
        case ATTACK:
            time = hw.p1.Read();
            break;
        case DECAY:
            time = hw.p2.Read();
            break;
        case SUSTAIN:
            time = 1000;  // Default time for Sustain.
            break;
        case RELEASE:
            time = hw.p4.Read();
            break;
    }

    // Short sleep duration before advancing to the next step in the curve table.
    delayMicroseconds(time * 10);

    // Write envelope CV output
    hw.output.Update(val);

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
