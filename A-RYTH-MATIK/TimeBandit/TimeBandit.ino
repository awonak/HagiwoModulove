/**
 * @file TimeBandit.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Fixed binary clock divider and sub oscillator.
 * @version 0.2
 * @date 2024-01-29
 *
 * @copyright Copyright (c) 2023
 *
 * The 6 digital outputs will produce a 50% duty cycle square wave in fixed
 * binary divisions of the incoming CLK signal. This can be used as a typical
 * clock divider or provide sub octaves of the incoming audio rate signal.
 * Each output is one octave lower than the previous.
 *
 * ENCODER: Select which division to edit. Single press to change the
            selected output. Single press again to leave edit mode.
 *
 * CLK: Clock input used to derrive fixed binary divisions.
 *
 * RST: Trigger this input to reset the division counter.
 *
 */
#include <arythmatik.h>

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Flag for rotating the panel 180 degrees.
// #define ROTATE_PANEL

// Flag for reversing the encoder direction.
// #define ENCODER_REVERSED

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
int counter;
byte mode;  // Normal, Select Output, Edit.
byte selected_out = 0;
bool update_display = true;

// Enum constants for script behavior mode.
enum Mode {
    MODE_DEFAULT,
    MODE_SELECT,
    MODE_EDIT,
    MODE_LAST,
};
Mode selected_mode = MODE_DEFAULT;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(115200);
#endif

#ifdef ROTATE_PANEL
    hw.config.RotatePanel = true;
#endif

#ifdef ENCODER_REVERSED
    hw.config.ReverseEncoder = true;
#endif

    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Set up encoder parameters
    hw.eb.setEncoderHandler(UpdateRotate);
    hw.eb.setClickHandler(UpdatePress);

    // Define each of the fixed clock divisions.
    // NOTE: This is binary value, clock divisions are bit shifted left by one.
    clockDiv[0] = {hw.outputs[0], 1};
    clockDiv[1] = {hw.outputs[1], 2};
    clockDiv[2] = {hw.outputs[2], 4};
    clockDiv[3] = {hw.outputs[3], 8};
    clockDiv[4] = {hw.outputs[4], 16};
    clockDiv[5] = {hw.outputs[5], 32};

    // Display each clock division on the OLED.
    UpdateDisplay();
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

    UpdateDisplay();
}

void UpdatePress(EncoderButton &eb) {
    // Next param on Main page.
    selected_mode = static_cast<Mode>((selected_mode + 1) % MODE_LAST);
    update_display = true;
}

void UpdateRotate(EncoderButton &eb) {
    if (selected_mode == MODE_SELECT) {
        switch (hw.EncoderDirection()) {
            case DIRECTION_DECREMENT:
                if (selected_out > 0) --selected_out;
                update_display = true;
                break;
            case DIRECTION_INCREMENT:
                if (selected_out < OUTPUT_COUNT - 1) ++selected_out;
                update_display = true;
                break;
        }
    }
    if (selected_mode == MODE_EDIT) {
        int division = clockDiv[selected_out].division;
        switch (hw.EncoderDirection()) {
            case DIRECTION_DECREMENT:
                if (division > 1) {
                    clockDiv[selected_out].division = division >> 1;
                    counter = 0;
                    update_display = true;
                }
                break;
            case DIRECTION_INCREMENT:
                if (division < 4096) {
                    clockDiv[selected_out].division = division << 1;
                    counter = 0;
                    update_display = true;
                }
                break;
        }
    }
}

void UpdateDisplay() {
    if (!update_display) return;
    update_display = false;
    hw.display.clearDisplay();

    // Draw page title.
    hw.display.setTextSize(0);
    hw.display.setCursor(36, 0);
    hw.display.println("TIME BANDIT");
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

    // Draw each clock division.
    double ymod = SCREEN_HEIGHT / 4;
    double xmod = SCREEN_WIDTH / 2;
    int ypos = 4;
    int xpos = 6;
    int count;
    String label = "CLK /";
    for (int i = 0; i < 2; i++) {      // columns
        for (int j = 0; j < 3; j++) {  // rows
            ypos += ymod;
            hw.display.setCursor(xpos, ypos);
            if (selected_mode != MODE_DEFAULT) {
                label = (selected_out == count) ? "EDIT/" : "CLK /";
            }
            if (selected_mode == MODE_EDIT && selected_out == count) {
                hw.display.drawRect(xpos - 4, ypos - 4, SCREEN_WIDTH / 2 - 4, 16, 1);
            }
            hw.display.println(label + String(clockDiv[count].division << 1));
            count++;
        }
        xpos += xmod;
        ypos = 4;
    }
    hw.display.display();
}
