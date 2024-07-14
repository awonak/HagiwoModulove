/**
 * @file BurstGenerator.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief
 * @version 0.2
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024
 *
 * Burst generator with rate, duration and slope settings.
 *
 * Repeat the incoming trigger for a given rate and duration.
 *
 * Heavily inspired by Luther's
 * https://github.com/PierreIsCoding/sdiy/tree/main/Spark
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

// The max burst gate count.
const int MAX_BURST_COUNT = 16;

// The duration of each burst spark in milliseconds.
const int MIN_BURST_RATE = 10;
const int MAX_BURST_RATE = 512;

// Script state.
int counter = 0;
int individual_cv = 0;
int output = 0;
float individual_progress = 0;
float full_progress = 0;
unsigned long burst_start = 0;
unsigned long last_burst = 0;
int burst_rate;
int burst_count;
int target_cv;

enum SequenceShape {
    SEQUENCE_FADE_IN,
    SEQUENCE_FLAT,
    SEQUENCE_FADE_OUT,
    SEQUENCE_TRIANGLE,
    SEQUENCE_RANDOM,
};
SequenceShape sequence_shape = SEQUENCE_FLAT;

enum TrigShape {
    TRIG_FADE_IN,
    TRIG_FLAT,
    TRIG_FADE_OUT,
};
TrigShape trig_shape = TRIG_FLAT;

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif
#ifdef SYNCHRONIZER
    hw.config.Synchronizer = true;
#endif

    // Initialize the SyncLFO peripherials.
    hw.Init();
}

void loop() {
    // Read cv inputs to determine state for this loop.
    hw.ProcessInputs();

    // Detect if a new trigger has been received. If so, read inputs for burst
    // settings.
    if (beginBurst()) {
        // Initialize burst state variables.
        burst_start = millis();
        counter = 0;

        // Read the current knob parameters.
        burst_rate = map(hw.p1.Read(), 0, MAX_INPUT, MIN_BURST_RATE, MAX_BURST_RATE);
        burst_count = map(hw.p2.Read(), 0, MAX_INPUT, 1, MAX_BURST_COUNT);
        sequence_shape = readSequenceShape(hw.p3.Read());
        trig_shape = readTrigShape(hw.p4.Read());
    }

    // Update burst cv if withing a bursting state.
    if (millis() < burst_start + ((2 * burst_rate) * burst_count)) {
        // Increment counter for current phase of duty cycle.
        if (millis() > burst_start + (counter * burst_rate)) {
            counter++;
            if (counter % 2) {
                last_burst = millis();
                // Calculate the progress percentage from current time to
                // burst_duration.
                full_progress = float(millis() - burst_start) /
                                float(((2 * burst_rate) * burst_count));
                // Max output or random step.
                target_cv = (sequence_shape == SEQUENCE_RANDOM) ? random(MAX_INPUT) : MAX_INPUT;
            }
        }

        // Calculate progress of current burst slope.
        if (millis() < (last_burst + (2 * burst_rate))) {
            individual_progress =
                float(millis() - last_burst) / float(2 * burst_rate);
        }
        switch (trig_shape) {
            case TRIG_FLAT:
                individual_cv = (counter % 2) ? target_cv : 0;
                break;
            case TRIG_FADE_IN:
                individual_cv = individual_progress * float(target_cv);
                break;
            case TRIG_FADE_OUT:
                individual_cv = (float(target_cv) - (individual_progress * float(target_cv)));
                break;
        }

        // Calculate overall burst progress.
        switch (sequence_shape) {
            case SEQUENCE_FLAT:
            case SEQUENCE_RANDOM:
                output = individual_cv;
                break;
            case SEQUENCE_FADE_IN:
                output = full_progress * individual_cv;
                break;
            case SEQUENCE_FADE_OUT:
                output = (individual_cv - (full_progress * individual_cv));
                break;
            case SEQUENCE_TRIANGLE:
                output = (counter <= burst_count)
                             ? (full_progress * individual_cv)
                             : (individual_cv - (full_progress * individual_cv));
        }

        hw.output.Update10bit(output);
    } else {
        hw.output.Update(0);
    }
}

bool beginBurst() {
    // Read all inputs.
    bool trigger_start = hw.trig.State() == DigitalInput::STATE_RISING;

#ifdef SYNCHRONIZER
    trigger_start |= hw.b1.Change() == Button::CHANGE_PRESSED;
#endif

    return trigger_start;
}

SequenceShape readSequenceShape(int val) {
    if (val <= 204) {
        return SEQUENCE_FADE_IN;
    } else if (val <= 410) {
        return SEQUENCE_FLAT;
    } else if (val <= 616) {
        return SEQUENCE_FADE_OUT;
    } else if (val <= 820) {
        return SEQUENCE_TRIANGLE;
    } else {
        return SEQUENCE_RANDOM;
    }
}

TrigShape readTrigShape(int val) {
    if (val <= 341) {
        return TRIG_FADE_IN;
    } else if (val <= 682) {
        return TRIG_FLAT;
    } else {
        return TRIG_FADE_OUT;
    }
}
