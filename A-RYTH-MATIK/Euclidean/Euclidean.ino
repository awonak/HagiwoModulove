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
 *      Short press to change between selecting a prameter and editing the parameter.
 *      Hold & rotate to change current output channel pattern.
 *
 * CLK: Clock input used to advance the patterns.
 *
 * RST: Trigger this input to reset all patterns.
 *
 */

// Include the Modulove hardware library.
#include <arythmatik.h>

// Script specific helper libraries.
#include "pattern.h"
#include "save_state.h"

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

// Flag for rotating the panel 180 degrees.
// #define ROTATE_PANEL

// Flag for reversing the encoder direction.
#define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

const byte MAX_STEPS = 32;
const byte TRIGGER_DURATION_MS = 50;
const byte UI_TRIGGER_DURATION_MS = 200;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Pattern patterns[OUTPUT_COUNT];
long last_clock_input = 0;
bool state_changed = false;
bool trigger_active = false;

// State variables for tracking OLED and editable parameter changes.
bool ui_trigger_active = false;
bool update_display = true;

// Enum constants for selecting an editable attribute.
enum Parameter {
    PARAM_STEPS,
    PARAM_HITS,
    PARAM_OFFSET,
    PARAM_PADDING,
    PARAM_NONE,
};
Parameter selected_param = PARAM_STEPS;

enum UIMode {
    UIMODE_SELECT,
    UIMODE_EDIT,
    UIMODE_PAGE,
    UIMODE_LAST,
};
UIMode selected_mode = UIMODE_SELECT;

// Enum constants for current display page.
enum MenuPage {
    PAGE_MAIN,
    PAGE_MODE,
    PAGE_CLOCK,
    PAGE_LAST,
};
MenuPage selected_page = PAGE_MAIN;


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

    // Set up encoder parameters
    hw.eb.setEncoderHandler(HandleRotate);
    hw.eb.setClickHandler(HandlePress);
    hw.eb.setLongPressHandler(HandleLongPress);
    hw.eb.setEncoderPressedHandler(HandlePressedRotation);

    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Initial patterns.
    InitState(patterns);

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
            if (patterns[i].NextStep() == 1) {
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
            patterns[i].Reset();
        }
        update_display = true;
    }

    // If state has changed, write changes to memory.
    if (state_changed) {
        SaveChanges(patterns);
        state_changed = false;
    }

    // Call update display to refresh the screen if required.
    if (update_display) UpdateDisplay();
}

void HandlePress(EncoderButton &eb) {

    switch (selected_mode) {
    case UIMODE_PAGE:
        selected_mode = UIMODE_SELECT;
        update_display = true;
        break;

    case UIMODE_EDIT:
        switch (selected_page) {
        case PAGE_MAIN:
            // If leaving EDIT mode, save state.
            selected_mode = UIMODE_SELECT;
            state_changed = true;
            update_display = true;
            break;
        }
        break;

    case UIMODE_SELECT:
        switch (selected_page) {
        case PAGE_MAIN:
            selected_mode = UIMODE_EDIT;
            update_display = true;
            break;
        case PAGE_MODE:
            selected_page = PAGE_MAIN;
            selected_mode = UIMODE_SELECT;
            update_display = true;
            break;
        }
        break;
    }
}

// Long press handler will toggle between page select mode and parameter edit
// mode for the current page.
void HandleLongPress(EncoderButton &eb) {
    // Toggle between menu page select mode.
    selected_mode = (selected_mode == UIMODE_PAGE) 
        ? UIMODE_SELECT
        : UIMODE_PAGE;
    update_display = true;
}

void HandleRotate(EncoderButton &eb) {
    // Read encoder for a change in direction and update the selected page or
    // parameter.
    Direction dir = hw.EncoderDirection();
    if (selected_mode == UIMODE_PAGE) {
        UpdatePage(dir);
        return;
    }
    switch (selected_page) {
    case PAGE_MAIN:
        UpdateParameter(dir);
        break;
    case PAGE_MODE:
        UpdateMode(dir);
        break;
    }
}

// When in select page mode, scroll through menu pages.
void UpdatePage(Direction dir) {
    if (dir == DIRECTION_INCREMENT && selected_page < PAGE_LAST - 1) {
        selected_page = static_cast<MenuPage>((selected_page + 1) % PAGE_LAST);
        update_display = true;
    }
    else if (dir == DIRECTION_DECREMENT && selected_page > PAGE_MAIN) {
        selected_page = static_cast<MenuPage>((selected_page - 1) % PAGE_LAST);
        update_display = true;
    }
}

void UpdateParameter(Direction dir) {
    // Convert the configured encoder direction to an integer equivalent value.
    int val = dir == DIRECTION_INCREMENT ? 1 : -1;

    switch (selected_mode) {
    case UIMODE_SELECT:
        if (static_cast<Parameter>(selected_param) == 0 && val < 0) {
            selected_param = static_cast<Parameter>(PARAM_NONE - 1);
        } else {
            selected_param = static_cast<Parameter>((selected_param + val) % PARAM_NONE);
        }
        break;
    
    case UIMODE_EDIT:
        // Handle rotation for current parameter.
        switch (selected_param) {
        case PARAM_STEPS:
            patterns[state.selected_out].ChangeSteps(val);
            break;
        case PARAM_HITS:
            patterns[state.selected_out].ChangeHits(val);
            break;
        case PARAM_OFFSET:
            patterns[state.selected_out].ChangeOffset(val);
            break;
        case PARAM_PADDING:
            patterns[state.selected_out].ChangePadding(val);
            break;
        }
        break;
    }
    update_display = true;
}

// Change the current output mode selection.
void UpdateMode(Direction dir) {
    if (dir == DIRECTION_INCREMENT && state.output_mode < OUTPUTMODE_LAST - 1) {
        state.output_mode = static_cast<OutputMode>(state.output_mode + 1);
    } else if (dir == DIRECTION_DECREMENT && state.output_mode > 0) {
        state.output_mode = static_cast<OutputMode>(state.output_mode - 1);
    }
    // Update the mode for all outputs.
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        // outputs[i].SetMode(state.output_mode);
    }
    update_display = true;
    state_changed = true;
}

void HandlePressedRotation(EncoderButton &eb) {
    ChangeSelectedOutput(hw.EncoderDirection());
}

void ChangeSelectedOutput(Direction dir) {
    switch (dir) {
        case DIRECTION_DECREMENT:
            if (state.selected_out > 0) --state.selected_out;
            update_display = true;
            break;
        case DIRECTION_INCREMENT:
            if (state.selected_out < OUTPUT_COUNT - 1) ++state.selected_out;
            update_display = true;
            break;
    }
}

void UpdateDisplay() {
    if (!update_display) return;
    update_display = false;
    hw.display.clearDisplay();
    DisplaySelectedUIMode();

    switch (selected_page) {
        case PAGE_MAIN:
            DisplayMainPage();
            break;
        case PAGE_MODE:
            DisplayOutputModePage();
            break;
        case PAGE_CLOCK:
            DisplayClockPage();
            break;
    }
    hw.display.display();
}

// Page title centered within 10 characters.
void PageTitle(char *title) {
    hw.display.setTextSize(0);
    hw.display.setCursor(32, 0);
    hw.display.println(title);
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);
}

void DisplayMainPage() {
    PageTitle(" EUCLIDEAN");
    DisplayParam();
    DisplayChannels();
    DisplayPattern();
}

void DisplayParam() {
    hw.display.setCursor(26, 18);
    hw.display.setTextSize(0);
    if (selected_param == PARAM_STEPS) {
        hw.display.print(F("Steps: "));
        hw.display.print(patterns[state.selected_out].steps);
    } else if (selected_param == PARAM_HITS) {
        hw.display.print(F("Hits: "));
        hw.display.print(patterns[state.selected_out].hits);
    } else if (selected_param == PARAM_OFFSET) {
        hw.display.print(F("Offset: "));
        hw.display.print(patterns[state.selected_out].offset);
    } else if (selected_param == PARAM_PADDING) {
        hw.display.print(F("Padding: "));
        hw.display.print(patterns[state.selected_out].padding);
    }
}

void DisplaySelectedUIMode() {
    switch (selected_mode) {
        case UIMODE_SELECT:
            if (selected_page == PAGE_MAIN) {
                hw.display.drawChar(116, 18, LEFT_TRIANGLE, 1, 0, 1);
            } else if (selected_page == PAGE_MODE) {
                hw.display.drawBitmap(112, 16, pencil_gfx, 12, 12, 1);
            }
            break;
        case UIMODE_EDIT:
            hw.display.drawBitmap(112, 16, pencil_gfx, 12, 12, 1);
            break;
        case UIMODE_PAGE:
            hw.display.drawChar(116, 0, LEFT_TRIANGLE, 1, 0, 1);
            break;
    }
}

void DisplayChannels() {
    hw.display.setCursor(4, 20);
    hw.display.setTextSize(2);
    hw.display.println(state.selected_out + 1);

    int start = 42;
    int top = start;
    int left = 4;
    int boxSize = 4;
    int margin = 2;
    int wrap = 3;

    for (int i = 1; i <= OUTPUT_COUNT; i++) {
        // Draw box, fill current step.
        (i == state.selected_out + 1)
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

    int steps = patterns[state.selected_out].steps;
    int offset = patterns[state.selected_out].offset;
    int padding = patterns[state.selected_out].padding;

    for (int i = 0; i < steps + padding; i++) {
        // Draw box, fill current step.
        switch(patterns[state.selected_out].GetStep(i)) {
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
        if (ui_trigger_active && i == patterns[state.selected_out].current_step) {
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


void DisplayOutputModePage() {
    PageTitle("OUTPUT MODE");
    // Draw output modes
    (state.output_mode == TRIGGER)
        ? hw.display.fillRect(12, 20, 8, 8, 1)
        : hw.display.drawRect(12, 20, 8, 8, 1);

    (state.output_mode == GATE)
        ? hw.display.fillRect(12, 32, 8, 8, 1)
        : hw.display.drawRect(12, 32, 8, 8, 1);

    (state.output_mode == FLIP)
        ? hw.display.fillRect(12, 44, 8, 8, 1)
        : hw.display.drawRect(12, 44, 8, 8, 1);

    hw.display.setCursor(32, 20);
    hw.display.println(F("Trig Mode"));
    hw.display.setCursor(32, 32);
    hw.display.println(F("Gate Mode"));
    hw.display.setCursor(32, 46);
    hw.display.println(F("Flip Mode"));
}

void DisplayClockPage() {
    PageTitle("   CLOCK  ");

}