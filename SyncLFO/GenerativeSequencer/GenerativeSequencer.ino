/**
 * @file GenerativeSequencer.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Generative Sequencer firmware for HAGIWO Sync Mod LFO (demo: https://youtu.be/okj47TcXJXg)
 * @version 0.1
 * @date 2023-03-26
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <synclfo.h>

// ALTERNATE HARDWARE CONFIGURATION
#define SYNCHRONIZER

// Uncomment to print state to serial monitoring output.
// #define DEBUG

using namespace modulove;
using namespace synclfo;

// Declare SyncLFO hardware variable.
SyncLFO hw;

// State variables.
int probability, steps, amplititude;

int output;
int step;
int refrain_counter;
int rand_val;

int refrain = 1;
int refrain_max = 4;
int pattern_size_max = 16;
int cv_max = MAX_INPUT;
int cv_pattern[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int pattern_options[8] = {1, 2, 3, 4, 6, 8, 12, 16};

void setup() {
#ifdef SYNCHRONIZER
    hw.config.Synchronizer = true;
#endif
#ifdef DEBUG
    Serial.begin(115200);
#endif

    // Initialize the SyncLFO peripherials.
    hw.Init();

    // Inititialize random values in sequence.
    for (int i = 0; i < pattern_size_max; i++) {
        cv_pattern[i] = random(cv_max);
    }
}

void loop() {
    // Read cv inputs to determine state for this loop.
    hw.ProcessInputs();

    bool advance = hw.trig.State() == DigitalInput::STATE_RISING;
    if (hw.config.Synchronizer) {
        advance |= hw.b1.Change() == Button::CHANGE_PRESSED;
    }

    // Check for new clock trigger.
    if (advance) {
        // Increment the current sequence step.
        // Right shift to scale input to a range of 8.
        steps = pattern_options[(hw.p2.Read() >> 7)];
        step = (step + 1) % steps;

        // Increment Refrain at the first step of the sequence.
        if (step == 0) {
            // Right shift to scale input to a range of 4.
            refrain = (hw.p4.Read() >> 8) + 1;
            refrain_counter = (refrain_counter + 1) % refrain;
        }

        // Check probability and refrain counter to see if the current step
        // should be updated.
        probability = hw.p1.Read();
        rand_val = random(cv_max);
        if (refrain_counter == 0 && probability > rand_val) {
            cv_pattern[step] = random(cv_max);
        }

        debug();
    }

    // Scale the max cv output range.
    amplititude = hw.p3.Read();

    // Update PWM CV output value.
    output = map(cv_pattern[step], 0, cv_max, 0, amplititude);
    hw.output.Update(output);
}

void debug() {
#ifdef DEBUG
    Serial.println(
        "Prob: " + String(probability) + " > " + String(rand_val)                 //
        + "\tValue: " + String(cv_pattern[step])                                  //
        + "\tRefrain: [" + String(refrain_counter) + "/" + String(refrain) + "]"  //
        + "\tSteps: [" + String(step) + "/" + String(steps) + "]"                 //
        + "\tAmp: " + String(amplititude));
#endif
}
