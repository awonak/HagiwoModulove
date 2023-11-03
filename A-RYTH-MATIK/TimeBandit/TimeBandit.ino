/**
 * @file TimeBandit.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Fixed binary clock divider and sub oscillator.
 * @version 0.1
 * @date 2023-10-29
 *
 * @copyright Copyright (c) 2023
 *
 * The 6 digital outputs will produce a 50% duty cycle square wave in fixed
 * binary divisions of the incoming CLK signal. This can be used as a typical
 * clock divider or provide sub octaves of the incoming audio rate signal.
 * Each output is one octave lower than the previous.
 *
 * ENCODER: Unused.
 *
 * CLK: Clock input used to derrive fixed binary divisions.
 *
 * RST: Trigger this input to reset the division counter.
 *
 */
#include "src/libmodulove/arythmatik.h"

using namespace modulove;
using namespace arythmatik;

struct ClockDivision {
    // Instance of an A-RYTH-MATIK digital output.
    modulove::DigitalOutput output;
    // Binary division of the incoming clock.
    int division;
};

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
ClockDivision clockDiv[OUTPUT_COUNT];
byte counter;

void setup() {
    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Define each of the fixed clock divisions.
    // NOTE: This is binary value, clock divisions are bit shifted left by one.
    clockDiv[0] = {hw.outputs[0], 1};
    clockDiv[1] = {hw.outputs[1], 2};
    clockDiv[2] = {hw.outputs[2], 4};
    clockDiv[3] = {hw.outputs[3], 8};
    clockDiv[4] = {hw.outputs[4], 16};
    clockDiv[5] = {hw.outputs[5], 32};

    // Display each clock division on the OLED.
    hw.display.clearDisplay();

    // Draw page title.
    hw.display.setTextSize(0);
    hw.display.setCursor(36, 0);
    hw.display.println("Time Bandit");
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

    // Draw each clock division.
    double ymod = SCREEN_HEIGHT * 0.25;
    double xmod = SCREEN_WIDTH * 0.6;
    int ypos = 4;
    int xpos = 6;
    int count;
    for (int i = 0; i < 2; i++) {      // columns
        for (int j = 0; j < 3; j++) {  // rows
            ypos += ymod;
            hw.display.setCursor(xpos, ypos);
            hw.display.println("CLK /" + String(clockDiv[count].division<<1));
            count++;
        }
        xpos += xmod;
        ypos = 4;
    }
    hw.display.display();
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this loop.
    hw.ProcessInputs();

    // Advance the counter on CLK input
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        counter++;
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            // Bitwise logical check if division is high.
            (counter & clockDiv[i].division)
                ? clockDiv[i].output.High()
                : clockDiv[i].output.Low();
        }
    }

    // Reset the clock division counter on RST input.
    if (hw.rst.State() == DigitalInput::STATE_RISING) {
        counter = 0;
    }
}
