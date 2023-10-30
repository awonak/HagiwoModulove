/**
 * @file StutterEngine.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Clocked burst generator.
 * @version 0.1
 * @date 2023-10-29
 *
 * @copyright Copyright (c) 2023
 *
 * Configurable clock multiplication burst generator.
 *
 * ENCODER:
 *      short press: Toggle between editing the step length and selecting a seed.
 *      long press: Enter seed edit mode to manually provide a seed value.
 *
 * CLK: Provide a gate or trigger for each output to repeat with decreasing
 *      probability in each output.
 *
 * RST: Trigger this input to reseed the psudo random sequence.
 *
 */
#include "stutter.h"
#include "src/libmodulove/arythmatik.h"

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Stutter clocks[OUTPUT_COUNT];

bool updateDisplay = true;

void setup() {
    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    clocks[0].Init(hw.outputs[0], 1, 4);
    clocks[1].Init(hw.outputs[1], 2, 4);
    clocks[2].Init(hw.outputs[2], 3, 4);
    clocks[3].Init(hw.outputs[3], 2, 8);
    clocks[4].Init(hw.outputs[4], 4, 8);
    clocks[5].Init(hw.outputs[5], 8, 8);
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this loop.
    hw.ProcessInputs();

    // Grab variables needed in the for loops.
    long now = millis();

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        clocks[i].Tick(now);
    }

    // Advance the counter on CLK input
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            clocks[i].ProcessCLK(now);
        }
        updateDisplay = true;
    }

    // Start the burst generator on RST input
    if (hw.rst.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            clocks[i].ProcessRST(now);
        }
        updateDisplay = true;
    }

    // Manually adjust tempo
    Encoder::Direction dir = hw.encoder.Rotate();
    if (dir == Encoder::DIRECTION_INCREMENT) {
        updateDisplay = true;
    } else if (dir == Encoder::DIRECTION_DECREMENT) {
        updateDisplay = true;
    }

    // Display the current counter step on the OLED.
    if (updateDisplay) {
        updateDisplay = false;
        hw.display.clearDisplay();
        hw.display.setCursor(SCREEN_HEIGHT / 4, SCREEN_HEIGHT / 2);
        hw.display.println("Clock Tempo: stuff");
        hw.display.display();
    }
}