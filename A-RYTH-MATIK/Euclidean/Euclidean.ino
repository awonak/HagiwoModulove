/**
 * @file Euclidean.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Euclidean rhythm generator inspired by Pam's Pro Workout.
 * @version 0.1
 * @date 2025-01-05
 *
 * @copyright Copyright (c) 2025
 *
 * ENCODER:
 *      Short press to change current Euclidean rhythm attribute for editing.
 *      Long press to change current pattern.
 *
 * CLK: Clock input used to advance the patterns.
 *
 * RST: Trigger this input to reset all patterns.
 *
 * TODO:
 *  - add save state
 */

// Include the Modulove hardware library.
#include <arythmatik.h>

// Script specific helper libraries.
#include "pattern.h"

// Graphics identifiers
#define RIGHT_TRIANGLE 0x10
#define LEFT_TRIANGLE 0x11

// 'pencil', 12x12px
const unsigned char pencil_gfx [] PROGMEM = {
	0x00, 0x00, 0x00, 0xc0, 0x01, 0xa0, 0x02, 0x60, 0x04, 0x40, 0x08, 0x80, 0x11, 0x00, 0x22, 0x00, 
	0x64, 0x00, 0x78, 0x00, 0x70, 0x00, 0x00, 0x00
};

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Flag for reversing the encoder direction.
// #define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

const byte MAX_STEPS = 32;
const byte TRIGGER_DURATION_MS = 50;
const byte UI_TRIGGER_DURATION_MS = 200;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Pattern pattern[OUTPUT_COUNT];
byte selected_out = 0;
long last_clock_input = 0;
bool update_display = true;
bool trigger_active = false;
bool ui_trigger_active = false;

// Enum constants for selecting an editable attribute.
enum Parameter {
    PARAM_STEPS,
    PARAM_HITS,
    PARAM_OFFSET,
    PARAM_PADDING,
    PARAM_LAST,
};
Parameter selected_param = PARAM_STEPS;

enum Mode {
    MODE_SELECT,
    MODE_EDIT,
    MODE_LAST,
};
Mode selected_mode = MODE_SELECT;

void setup() {
    // Only enable Serial monitoring if DEBUG is defined.
    // Note: this affects performance and locks LED 4 & 5 on HIGH.
    // #ifdef DEBUG
    // Serial.begin(9600);
    // Serial.println("DEBUG READY");
    // #endif

    // Set up encoder parameters
    hw.eb.setEncoderHandler(UpdateRotate);
    hw.eb.setClickHandler(UpdatePress);
    hw.eb.setEncoderPressedHandler(UpdatePressedRotation);

    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Initial patterns.
    pattern[0].Init(16, 4, 0, 0);
    pattern[1].Init(16, 4, 0, 0);
    pattern[2].Init(16, 4, 0, 0);
    pattern[3].Init(16, 4, 0, 0);
    pattern[4].Init(16, 4, 0, 0);
    pattern[5].Init(16, 4, 0, 0);

    // Display each clock division on the OLED.
    UpdateDisplay();
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this
    // loop.
    hw.ProcessInputs();

    // Advance the patterns on CLK input
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            if (pattern[i].NextStep()) {
                hw.outputs[i].High();
            }
        }
        last_clock_input = millis();
        trigger_active = true;
        ui_trigger_active = true;
        update_display = true;
    }

    // Turn off all outputs after trigger duration.
    if (trigger_active && millis() > last_clock_input + TRIGGER_DURATION_MS) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            hw.outputs[i].Low();
        }
        trigger_active = false;
    }

    // Turn off current step animation after ui trigger duration.
    if (ui_trigger_active && millis() > last_clock_input + UI_TRIGGER_DURATION_MS) {
        ui_trigger_active = false;
        update_display = true;
    }

    // Reset all patterns to the first pattern step on RST input.
    if (hw.rst.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            pattern[i].Reset();
        }
        update_display = true;
    }

    // Call update display to refresh the screen if required.
    if (update_display) UpdateDisplay();
}

void UpdatePress(EncoderButton &eb) {
    // Change current mode.
    selected_mode = static_cast<Mode>((selected_mode + 1) % MODE_LAST);
    update_display = true;
}

void UpdateRotate(EncoderButton &eb) {
    // Convert the direction to an integer equivalent value.
    int dir = eb.increment() > 0 ? 1 : -1;

    if (selected_mode == MODE_SELECT) {
        if (static_cast<Parameter>(selected_param) == 0 && dir < 0) {
            selected_param = static_cast<Parameter>(PARAM_LAST - 1);
        } else {
            selected_param = static_cast<Parameter>((selected_param + dir) % PARAM_LAST);
        }
    } else if (selected_mode == MODE_EDIT) {
        // Handle rotation for current parameter.
        switch (selected_param) {
            case PARAM_STEPS:
                pattern[selected_out].ChangeSteps(dir);
                break;
            case PARAM_HITS:
                pattern[selected_out].ChangeHits(dir);
                break;
            case PARAM_OFFSET:
                pattern[selected_out].ChangeOffset(dir);
                break;
            case PARAM_PADDING:
                pattern[selected_out].ChangePadding(dir);
                break;
        }
    }
    update_display = true;
}

void UpdatePressedRotation(EncoderButton &eb) {
    ChangeSelectedOutput(eb.increment());
}

void ChangeSelectedOutput(int dir) {
    switch (dir) {
        case -1:
            if (selected_out > 0) --selected_out;
            update_display = true;
            break;
        case 1:
            if (selected_out < OUTPUT_COUNT - 1) ++selected_out;
            update_display = true;
            break;
    }
}

void UpdateDisplay() {
    if (!update_display) return;
    update_display = false;
    hw.display.clearDisplay();

    // Draw page title.
    hw.display.setTextSize(0);
    hw.display.setCursor(37, 0);
    hw.display.println(F("EUCLIDEAN"));
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

    DisplayMode();
    DisplaySelectedMode();
    DisplayChannels();
    DisplayPattern();

    hw.display.display();
}

void DisplayMode() {
    hw.display.setCursor(26, 18);
    hw.display.setTextSize(0);
    if (selected_param == PARAM_STEPS) {
        hw.display.print(F("Steps: "));
        hw.display.print(String(pattern[selected_out].steps));
    } else if (selected_param == PARAM_HITS) {
        hw.display.print(F("Hits: "));
        hw.display.print(String(pattern[selected_out].hits));
    } else if (selected_param == PARAM_OFFSET) {
        hw.display.print(F("Offset: "));
        hw.display.print(String(pattern[selected_out].offset));
    } else if (selected_param == PARAM_PADDING) {
        hw.display.print(F("Padding: "));
        hw.display.print(String(pattern[selected_out].padding));
    }
}

void DisplaySelectedMode() {
    switch (selected_mode) {
        case MODE_SELECT:
            hw.display.drawChar(116, 18, LEFT_TRIANGLE, 1, 0, 1);
            break;
        case MODE_EDIT:
            hw.display.drawBitmap(112, 16, pencil_gfx, 12, 12, 1);
            break;
    }
}

void DisplayChannels() {
    hw.display.setCursor(4, 20);
    hw.display.setTextSize(2);
    hw.display.println(String(selected_out + 1));

    int start = 42;
    int top = start;
    int left = 4;
    int boxSize = 4;
    int margin = 2;
    int wrap = 3;

    for (int i = 1; i <= OUTPUT_COUNT; i++) {
        // Draw box, fill current step.
        (i == selected_out + 1)
            ? hw.display.fillRect(left, top, boxSize, boxSize, 1)
            : hw.display.drawRect(left, top, boxSize, boxSize, 1);

        // Advance the draw cursors.
        top += boxSize + margin + 1;

        // Wrap the box draw cursor if we hit wrap count.
        if (i % wrap == 0) {
            top = start;
            left += boxSize + margin;
        }
    }
}

void DisplayPattern() {
    // Draw boxes for pattern length.
    int start = 26;
    int top = 30;
    int left = start;
    int boxSize = 7;
    int margin = 2;
    int wrap = 8;

    int steps = pattern[selected_out].steps;
    int offset = pattern[selected_out].offset;
    int padding = pattern[selected_out].padding;

    for (int i = 0; i < steps + padding; i++) {
        // Draw box, fill current step.
        switch(pattern[selected_out].GetStep(i)) {
            case 1:
                hw.display.fillRect(left, top, boxSize, boxSize, 1);
                break;
            case 0:
                hw.display.drawRect(left, top, boxSize, boxSize, 1);
                break;
            case 2:
                hw.display.drawRect(left, top, boxSize, boxSize, 1);
                hw.display.drawLine(left+boxSize-1, top, left, top+boxSize-1, 1);
                break;
        }

        // Draw right arrow on current played step.
        if (ui_trigger_active && i == pattern[selected_out].current_step) {
            hw.display.drawChar(left + 1, top, RIGHT_TRIANGLE, 1, 0, 1);
        }

        // Advance the draw cursors.
        left += boxSize + margin + 1;

        // Wrap the box draw cursor if we hit wrap count.
        if ((i + 1) % wrap == 0) {
            top += boxSize + margin;
            left = start;
        }
    }
}