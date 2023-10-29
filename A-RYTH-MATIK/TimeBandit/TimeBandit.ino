#include "src/libmodulove/arythmatik.h"

using namespace modulove;
using namespace arythmatik;

struct Clock {
    modulove::DigitalOutput output;
    int division;
};

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Clock clocks[OUTPUT_COUNT];

bool updateDisplay = true;
byte counter;

void setup() {
    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    clocks[0] = {hw.outputs[0], 1};
    clocks[1] = {hw.outputs[1], 2};
    clocks[2] = {hw.outputs[2], 4};
    clocks[3] = {hw.outputs[3], 8};
    clocks[4] = {hw.outputs[4], 16};
    clocks[5] = {hw.outputs[5], 32};
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this loop.
    hw.ProcessInputs();

    // Advance the counter on CLK input
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        counter++;
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            // Bitwise logical check if division is high.
            (counter & clocks[i].division)
                ? clocks[i].output.High()
                : clocks[i].output.Low();
        }
    }

    // Display the current counter step on the OLED.
    if (updateDisplay) {
        updateDisplay = false;
        hw.display.clearDisplay();
        PageTitle("Time Bandit");

        double ymod = SCREEN_HEIGHT * 0.25;
        double xmod = SCREEN_WIDTH * 0.6;
        int ypos = 4;
        int xpos = 6;
        int count;

        for (int i = 0; i < 2; i++) {
            for (int i = 0; i < 3; i++) {
                ypos += ymod;
                hw.display.setCursor(xpos, ypos);
                hw.display.println("div: " + String(clocks[count].division));
                count++;
            }
            xpos += xmod;
            ypos = 4;
        }
        hw.display.display();
    }
}

void PageTitle(String title) {
    // Draw page title.
    hw.display.setTextSize(0);
    hw.display.setCursor(36, 0);
    hw.display.println(title);
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);
}