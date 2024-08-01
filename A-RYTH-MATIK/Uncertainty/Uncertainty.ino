/**
 * @file Uncertainty.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Stochastic gate processor based on the Olivia Artz Modular's Uncertainty.
 * @version 0.2
 * @date 2023-07-19
 *
 * @copyright Copyright (c) 2023
 *
 * Connect a trigger or gate source to the CLK input and the each output will
 * mirror that signal according to a decreasing probability.
 */

// Include the Modulove library.
#include <arythmatik.h>

// Script specific output class.
#include "output.h"

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Flag for reversing the encoder direction.
//#define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

ProbablisticOutput outputs[OUTPUT_COUNT];

// Enum constants for current display page.
enum MenuPage {
    PAGE_MAIN,
    PAGE_MODE,
};
MenuPage selected_page = PAGE_MAIN;

// Script config definitions
const uint8_t PARAM_COUNT = 2;  // Count of editable parameters.

// Script state variables.
byte selected_out = 0;
byte selected_param = 0;

bool update_display = true;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(9600);
#endif

    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Initialize each of the outputs with it's GPIO pins and probability.
    outputs[0].Init(hw.outputs[0], 0.96);
    outputs[1].Init(hw.outputs[1], 0.82);
    outputs[2].Init(hw.outputs[2], 0.64);
    outputs[3].Init(hw.outputs[3], 0.48);
    outputs[4].Init(hw.outputs[4], 0.32);
    outputs[5].Init(hw.outputs[5], 0.18);

    // CLOCK LED (DIGITAL)
    pinMode(CLOCK_LED, OUTPUT);

    // OLED Display configuration.
    delay(1000);
    hw.display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    hw.display.setTextColor(WHITE);
    hw.display.clearDisplay();
    hw.display.display();
}

void loop() {
    // Read inputs to determine state.
    hw.ProcessInputs();

    // Input clock has gone high, call each output's On() for a chance to
    // trigger that output.
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].On();
        }
    }

    // Input clock has gone low, turn off Outputs.
    if (hw.clk.State() == DigitalInput::STATE_FALLING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].Off();
        }
    }

    // Read the encoder button press event.
    Encoder::PressType press = hw.encoder.Pressed();

    // Short button press. Change editable parameter.
    if (press == Encoder::PRESS_SHORT) {
        // Next param on Main page.
        if (selected_page == PAGE_MAIN)
            selected_param = ++selected_param % PARAM_COUNT;

        update_display = true;
    }

    // Long button press. Change menu page.
    if (press == Encoder::PRESS_LONG) {
        if (selected_page == PAGE_MAIN) {
            selected_page = PAGE_MODE;
            selected_param = 2;
        } else {
            selected_page = PAGE_MAIN;
            selected_param = 0;
        }
        update_display = true;
    }

    // Read encoder for a change in direction and update the selected parameter.
    UpdateParameter(hw.encoder.Rotate());

    // Render any new UI changes to the OLED display.
    UpdateDisplay();
}

void UpdateParameter(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED) return;
    if (selected_param == 0) UpdateOutput(dir);
    if (selected_param == 1) UpdateProb(dir);
    if (selected_param == 2) UpdateMode(dir);
}

void UpdateOutput(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_INCREMENT && selected_out < OUTPUT_COUNT - 1)
        selected_out++;
    if (dir == Encoder::DIRECTION_DECREMENT && selected_out > 0)
        selected_out--;
    update_display = true;
}

void UpdateMode(Encoder::Direction dir) {
    Mode newMode;
    if (dir == Encoder::DIRECTION_INCREMENT) newMode = Mode::FLIP;
    if (dir == Encoder::DIRECTION_DECREMENT) newMode = Mode::TRIGGER;
    // Update the mode for all outputs.
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        outputs[i].SetMode(newMode);
    }
    update_display = true;
}

void UpdateProb(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_INCREMENT) outputs[selected_out].IncProb();
    if (dir == Encoder::DIRECTION_DECREMENT) outputs[selected_out].DecProb();
    update_display = true;
}

void UpdateDisplay() {
    if (!update_display) return;
    update_display = false;
    hw.display.clearDisplay();

    // Draw app title.
    hw.display.setTextSize(0);
    hw.display.setCursor(36, 0);
    hw.display.println("UNCERTAINTY");
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

    switch (selected_page) {
        case PAGE_MAIN:
            DisplayMainPage();
            break;
        case PAGE_MODE:
            DisplayModePage();
            break;
    }

    hw.display.display();
}

void DisplayMainPage() {
    // Draw boxes for pattern length.
    int top = 16;
    int left = 16;
    int barWidth = 10;
    int barHeight = 42;
    int padding = 8;

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        // Draw output probability bar.
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        // Fill current bar probability.
        byte probFill = (float(barHeight) * outputs[i].GetProb());
        byte probTop = top + barHeight - probFill;
        hw.display.fillRect(left, probTop, barWidth, probFill, 1);

        // Show selected output.
        if (i == selected_out) {
            (selected_param == 0)
                ? hw.display.drawChar(left + 2, SCREEN_HEIGHT - 6, 0x18, 1, 0, 1)
                : hw.display.drawChar(left + 2, SCREEN_HEIGHT - 6, 0x25, 1, 0, 1);
        }

        left += barWidth + padding;
    }
}

void DisplayModePage() {
    // Show mode edit page.
    hw.display.setTextSize(2);
    hw.display.setCursor(6, 32);
    hw.display.println("MODE: " + outputs[0].DisplayMode());
}
