/**
 * @file Baby4.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief 4 step cv sequencer firmware for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.2
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */

// ln -s ~/github/libmodulove ~/Arduino/libraries
#include <synclfo.h>

// ALTERNATE HARDWARE CONFIGURATION
#define SYNCHRONIZER

using namespace modulove;
using namespace synclfo;

// Declare SyncLFO hardware variable.
SyncLFO hw;

void setup() {
#ifdef SYNCHRONIZER
    hw.config.Synchronizer = true;
#endif

    // Initialize the SyncLFO peripherials.
    hw.Init();
}

void loop() {
    // Current CV step (0,1,2,3)
    static byte step = 0;

    // Read cv inputs to determine state for this loop.
    hw.ProcessInputs();

    bool advance = hw.trig.State() == DigitalInput::STATE_RISING;
    if (hw.config.Synchronizer) {
        advance |= hw.b1.Change() == Button::CHANGE_PRESSED;
    }

    // Detect if new trigger received and advance step.
    if (advance) {
        step = (step + 1) % synclfo::P_COUNT;
    }

    // Write current step CV output.
    hw.output.Update(hw.knobs[step]->Read());
}
