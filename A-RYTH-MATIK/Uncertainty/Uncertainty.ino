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
#include <EEPROM.h>

// Script specific output class.
#include "output.h"

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Flag for rotating the panel 180 degrees.
// #define ROTATE_PANEL

// Flag for reversing the encoder direction.
// #define ENCODER_REVERSED

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

// Script state & storage variables.
const char SCRIPT_NAME[] = "UNCERTAINTY";
const uint8_t SCRIPT_VER = 1;

struct State {
    // Version check.
    char script[sizeof(SCRIPT_NAME)];
    uint8_t version;
    // State variables.
    float probability[OUTPUT_COUNT];
    Mode mode;
};
State state;

bool state_changed = false;

// Script state variables.
byte selected_out = 0;
byte selected_param = 0;

bool update_display = true;

// Initialize script state from EEPROM or default values.
void InitState() {
    EEPROM.get(0, state);

    // Check if the data in memory matches expected values.
    if ((strcmp(state.script, SCRIPT_NAME) != 0) || (state.version != SCRIPT_VER)) {
        // Set script version identifier values.
        strcpy(state.script, SCRIPT_NAME);
        state.version = SCRIPT_VER;

        // Default state vars.
        state.mode = TRIGGER;
        state.probability[0] = 0.96;
        state.probability[1] = 0.82;
        state.probability[2] = 0.64;
        state.probability[3] = 0.48;
        state.probability[4] = 0.32;
        state.probability[5] = 0.18;
    }
}

// Save state to EEPROM if state has changed.
void SaveChanges() {
    if (!state_changed) return;
    state_changed = false;
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        state.probability[i] = outputs[i].GetProb();
    }
    state.mode = outputs[0].GetMode();
    EEPROM.put(0, state);
}

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

    hw.eb.setClickHandler(ShortPress);
    hw.eb.setLongPressHandler(LongPress);
    hw.eb.setEncoderHandler(UpdateParameter);

    hw.AttachClockHandler(HandleClockPinChange);

    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Initialize script state from EEPROM or default values.
    InitState();

    // Initialize each of the outputs with it's GPIO pins and probability.
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        outputs[i].Init(hw.outputs[i], state.probability[i], state.mode);
    }

    // CLOCK LED (DIGITAL)
    pinMode(CLOCK_LED, OUTPUT);

    // OLED Display configuration.
    hw.display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    hw.display.setTextColor(WHITE);
    hw.display.clearDisplay();
    hw.display.display();
}

void loop() {
    // Read inputs to determine state.
    hw.ProcessInputs();

    // Render any new UI changes to the OLED display.
    UpdateDisplay();
}

void HandleClockPinChange() {
    // Input clock has gone high, call each output's On() for a chance to
    // trigger that output.
    if (hw.clk.Read() == HIGH) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].On();
        }
    }

    // Input clock has gone low, turn off Outputs.
    if (hw.clk.Read() == LOW) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].Off();
        }
    }
}

// Short button press. Change editable parameter.
void ShortPress(EncoderButton &eb) {
    // Next param on Main page.
    if (selected_page == PAGE_MAIN)
        selected_param = ++selected_param % PARAM_COUNT;

    SaveChanges();
    update_display = true;
}

// Long button press. Change menu page.
void LongPress(EncoderButton &eb) {
    if (selected_page == PAGE_MAIN) {
        selected_page = PAGE_MODE;
        selected_param = 2;
    } else {
        selected_page = PAGE_MAIN;
        selected_param = 0;
    }
    SaveChanges();
    update_display = true;
}

// Read encoder for a change in direction and update the selected parameter.
void UpdateParameter(EncoderButton &eb) {
    // Convert the configured encoder direction to an integer equivalent value.
    int dir = hw.EncoderDirection() == DIRECTION_INCREMENT ? 1 : -1;
    if (selected_param == 0) UpdateOutput(dir);
    if (selected_param == 1) UpdateProb(dir);
    if (selected_param == 2) UpdateMode(dir);
    update_display = true;
}

void UpdateOutput(int dir) {
    if (dir == 1 && selected_out < OUTPUT_COUNT - 1)
        selected_out++;
    if (dir == -1 && selected_out > 0)
        selected_out--;
    update_display = true;
}

void UpdateMode(int dir) {
    Mode newMode;
    if (dir == 1) newMode = Mode::FLIP;
    if (dir == -1) newMode = Mode::TRIGGER;
    // Update the mode for all outputs.
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        outputs[i].SetMode(newMode);
    }
    update_display = true;
    state_changed = true;
}

void UpdateProb(int dir) {
    if (dir == 1) outputs[selected_out].IncProb();
    if (dir == -1) outputs[selected_out].DecProb();
    update_display = true;
    state_changed = true;
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
