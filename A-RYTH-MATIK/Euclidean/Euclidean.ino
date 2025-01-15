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
 *  - fix swapped clk/rst
 *  - fix param select encoder
 *  - add save state
 *  - replace E with pencil blit
 *  - implement offset
 *  - implement padding
 */

// Include the Modulove hardware library.
#include <arythmatik.h>

// Script specific helper libraries.
#include "pattern.h"

// Graphics identifiers
#define RIGHT_TRIANGLE 0x10
#define LEFT_TRIANGLE 0x11

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Flag for reversing the encoder direction.
// #define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

const byte MAX_STEPS = 32;
const byte TRIGGER_DURATION_MS = 50;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Pattern pattern[OUTPUT_COUNT];
byte selected_out = 0;
long last_clock_input = 0;
bool update_display = true;
bool trigger_active = false;

// Enum constants for selecting an editable attribute.
enum Parameter {
    PARAM_STEPS,
    PARAM_HITS,
    PARAM_OFFSET,
    // PARAM_PADDING,
    PARAM_CHANNEL,
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

    // Initialize the A-RYTH-MATIK peripherials.
    hw.config.ReverseEncoder = true;
    hw.Init();

    // Initial patterns.
    pattern[0].Init(11, 7, 0, 0);
    pattern[1].Init(16, 4, 0, 0);
    pattern[2].Init(16, 4, 0, 0);
    pattern[3].Init(16, 4, 0, 0);
    pattern[4].Init(16, 4, 0, 0);
    pattern[5].Init(16, 4, 0, 0);

    // Display each clock division on the OLED.
    UpdateDisplay();
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this loop.
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
    }

    // Trigger mode: turn off all outputs after trigger duration.
    if (trigger_active && millis() > last_clock_input + TRIGGER_DURATION_MS) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
           hw.outputs[i].Low();
        }
        trigger_active = false;
    } 

    // Reset all patterns to the first pattern step on RST input.
    if (hw.rst.State() == DigitalInput::STATE_RISING) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
           pattern[i].Reset();
        }
        update_display = true;
    }

    // Read and handle the encoder button press and rotate events.
    UpdatePress(hw.encoder.Pressed());
    UpdateRotate(hw.encoder.Rotate());

    // Call update display to refresh the screen if required.
    UpdateDisplay();
}

void UpdatePress(Encoder::PressType press) {
    // Return if no press detected
    if (press == Encoder::PRESS_NONE) {
        return;
    }

    // Short button press. Change attribute.
    if (press == Encoder::PRESS_SHORT) {
        // Change current mode.
        selected_mode = static_cast<Mode>((selected_mode + 1) % MODE_LAST);
        update_display = true;
    }
}

void UpdateRotate(Encoder::Direction  dir) {
    // Return if no rotation detected.
    if (dir == Encoder::DIRECTION_UNCHANGED) {
        return;
    }

    // Convert the direction to an integer equivalent value. 
    int val = (dir == Encoder::DIRECTION_INCREMENT) ? 1 : -1;

    if (selected_mode == MODE_SELECT) {
        if (static_cast<Parameter>(selected_param) == 0 && val == -1) {
            selected_param = static_cast<Parameter>(PARAM_LAST - 1);
        } else {
            selected_param = static_cast<Parameter>((selected_param + val) % PARAM_LAST);
        }        
    }
    else if (selected_mode == MODE_EDIT) {
        // Handle rotation for current parameter.
        switch (selected_param) {
            case PARAM_STEPS:
                pattern[selected_out].ChangeSteps(val);
                break;
            case PARAM_HITS:
                pattern[selected_out].ChangeHits(val);
                break;
            case PARAM_OFFSET:
                pattern[selected_out].ChangeOffset(val);
                break;
            case PARAM_CHANNEL:
                ChangeSelectedOutput(dir);
                break;
        }
    }
    update_display = true;
}

void ChangeSelectedOutput(Encoder::Direction dir) {
    switch (dir) {
        case Encoder::DIRECTION_DECREMENT:
            if (selected_out > 0) --selected_out;
            update_display = true;
            break;
        case Encoder::DIRECTION_INCREMENT:
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
    if (selected_param == PARAM_CHANNEL) {
        hw.display.println(F("Channel"));
    }
    else if (selected_param == PARAM_STEPS) {
        hw.display.print(F("Steps: "));
        hw.display.print(String(pattern[selected_out].steps));
    }
    else if (selected_param == PARAM_HITS) {
        hw.display.print(F("Hits: "));
        hw.display.print(String(pattern[selected_out].hits));
    }
    else if (selected_param == PARAM_OFFSET) {
        hw.display.print(F("Offset: "));
        hw.display.print(String(pattern[selected_out].offset));
    }
}

void DisplaySelectedMode() {
    switch (selected_mode) {
        case MODE_SELECT:
            hw.display.drawChar(116, 18, LEFT_TRIANGLE, 1, 0, 1);
            break;
        case MODE_EDIT:
            hw.display.drawChar(116, 18, 'E', 1, 0, 1);
            break;
    }
}

void DisplayChannels() {
    hw.display.setCursor(4, 20);
    hw.display.setTextSize(2);
    hw.display.println(String(selected_out+1));

    int start = 4;
    int top = 42;
    int left = start;
    int boxSize = 4;
    int padding = 2;
    int wrap = 2;

    for (int i = 1; i <= OUTPUT_COUNT; i++) {
        // Draw box, fill current step.
        (i == selected_out + 1)
            ? hw.display.fillRect(left, top, boxSize, boxSize, 1)
            : hw.display.drawRect(left, top, boxSize, boxSize, 1);

        // Advance the draw cursors.
        left += boxSize + padding + 1;

        // Wrap the box draw cursor if we hit wrap count.
        if (i % wrap == 0) {
            top += boxSize + padding;
            left = start;
        }
    }
}

void DisplayPattern() {
    // Draw boxes for pattern length.
    int start = 26;
    int top = 30;
    int left = start;
    int boxSize = 7;
    int padding = 2;
    int wrap = 8;

    int steps = pattern[selected_out].steps;

    for (int i = 0; i < steps; i++) {
        // Draw box, fill current step.
        (pattern[selected_out].GetStep(i))
            ? hw.display.fillRect(left, top, boxSize, boxSize, 1)
            : hw.display.drawRect(left, top, boxSize, boxSize, 1);

        // if (selected_mode == MODE_PLAY) {
        //     // Draw right arrow on current played step.
        //     if (trigger_active && i == pattern[selected_out].current_step) {
        //         hw.display.drawChar(left+1, top, RIGHT_TRIANGLE, 1, 0, 1);
        //     }
        // }

        // Advance the draw cursors.
        left += boxSize + padding + 1;

        // Wrap the box draw cursor if we hit wrap count.
        if ((i+1) % wrap == 0) {
            top += boxSize + padding;
            left = start;
        }
    }
}