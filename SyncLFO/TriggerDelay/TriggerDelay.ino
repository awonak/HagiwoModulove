/**
 * @file TriggerDelay.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief 
 * @version 0.2
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024
 * 
 * Trigger delay with gate and slope settings.
 * 
 * Repeat the incoming trigger with a delay up to 2 seconds set by P1 for a
 * duration up to 2 seconds set by P2. Additionally, the output rising edge
 * can be slewed using P3 and the falling edge can be slewed using P4.
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

// The max trigger delay / gate duration.
const int MAX_DELAY_MS = 2000;

enum Stage {
    DELAY,
    ATTACK,
    GATE,
    RELEASE,
    WAIT,
};
Stage stage = WAIT;

// Script state.
unsigned long trig_start;
int trig_delay;
int trig_duration;
int attack_slope;
int release_slope;
int attack_count;
int release_count;
int counter;
int output;

void setup() {
#ifdef SYNCHRONIZER
    hw.config.Synchronizer = true;
#endif

    // Initialize the SyncLFO peripherials.
    hw.Init();
}

void loop() {
    static bool trigger_start;
    
    // Read cv inputs to determine state for this loop.
    hw.ProcessInputs();

    trigger_start = hw.trig.State() == DigitalInput::STATE_RISING;
    if (hw.config.Synchronizer) {
        trigger_start |= hw.b1.Change() == Button::CHANGE_PRESSED;
    }

    // Detect if a new trigger has been received.
    if (trigger_start) {
        // Read the trigger delay and duration.
        trig_delay = map(hw.p1.Read(), 0, MAX_INPUT, 0, MAX_DELAY_MS);
        trig_duration = map(hw.p2.Read(), 0, MAX_INPUT, 20, MAX_DELAY_MS);
        trig_start = millis();

        // Calculate the exponential slope value.
        attack_count = hw.p3.Read();
        release_count = hw.p4.Read();
        attack_slope = slope(attack_count);
        release_slope = slope(release_count);

        stage = DELAY;
        output = 0;
        counter = 1;
    }

    switch (stage) {
        case DELAY:
            if (millis() > trig_start + trig_delay) {
                stage = ATTACK;
            }
            break;
        case ATTACK:
            if (counter >= attack_count) {
                stage = GATE;
                trig_start = millis();
                counter = release_count;
            } else {
                output = min(pow(2, (float(counter) / float(attack_slope))), MAX_OUTPUT);
                counter = min(counter++, attack_count);
            }
            break;
        case GATE:
            output = MAX_OUTPUT;
            if (millis() > trig_start + trig_duration) {
                stage = RELEASE;
            }
            break;
        case RELEASE:
            if (counter == 0) {
                stage = WAIT;
                output = 0;
            } else {
                output = min(pow(2, float(counter) / float(release_slope)), MAX_OUTPUT);
                if (counter > 0) counter--;
            }
            break;
        case WAIT:
            break;
    }

    // Set output voltage.
    hw.output.Update(output);
}

// Calculate the linear to exponential slope value.
int slope (int input) {
    return (input * log10(2)) / (log10(MAX_OUTPUT));
}