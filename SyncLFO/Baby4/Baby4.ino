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

#include "src/libmodulove/synclfo.h"

using namespace modulove;
using namespace synclfo;


// Declare SyncLFO hardware variable.
SyncLFO hw;

// Current CV step (0,1,2,3)
byte step = 0;

void setup() {
    // Initialize the SyncLFO peripherials.
    hw.Init();
}

void loop() {
    // Read cv inputs to determine state for this loop.
    hw.ProcessInputs();

    // Detect if new trigger received and advance step.
    if (hw.trig.State() == DigitalInput::STATE_RISING) {
        step = (step + 1) % synclfo::P_COUNT;
    }

    // Write current step CV output.
    hw.output.Value(hw.knobs[step].Read());
}
