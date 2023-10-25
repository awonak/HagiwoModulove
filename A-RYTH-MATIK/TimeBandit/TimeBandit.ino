#include "clock.h"
#include "src/libmodulove/arythmatik.h"

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Clock clocks[OUTPUT_COUNT];

bool updateDisplay = true;

void setup() {
    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    clocks[0].Init(hw.outputs[0], Clock::MOD_MULTIPLY, 1);
    clocks[1].Init(hw.outputs[1], Clock::MOD_MULTIPLY, 2);
    clocks[2].Init(hw.outputs[2], Clock::MOD_MULTIPLY, 3);
    clocks[3].Init(hw.outputs[3], Clock::MOD_DIVISION, 2);
    clocks[4].Init(hw.outputs[4], Clock::MOD_DIVISION, 4);
    clocks[5].Init(hw.outputs[5], Clock::MOD_DIVISION, 8);
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this loop.
    hw.ProcessInputs();

    // Grab variables needed in the for loops.
    long now = millis();
    int bpm = clocks[0].BPM();

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        clocks[i].Tick(now);
    }

    // Advance the counter on CLK input
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            clocks[i].Process(now);
        }
        updateDisplay = true;
    }

    // Manually adjust tempo
    Encoder::Direction dir = hw.encoder.Rotate();
    if (dir == Encoder::DIRECTION_INCREMENT) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            clocks[i].SetBPM(bpm + 1, now);
        }
        updateDisplay = true;
    } else if (dir == Encoder::DIRECTION_DECREMENT) {
        int bpm = clocks[0].BPM();
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            clocks[i].SetBPM(bpm - 1, now);
        }
        updateDisplay = true;
    }

    // Display the current counter step on the OLED.
    if (updateDisplay) {
        updateDisplay = false;
        hw.display.clearDisplay();
        hw.display.setCursor(SCREEN_HEIGHT / 4, SCREEN_HEIGHT / 2);
        hw.display.println("Clock Tempo: " + String(clocks[0].BPM()));
        hw.display.display();
    }
}
