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
 *      Long press to change global settings like output mode (trigger, gate, flip) and internal clock tempo.
 *      Hold & rotate to change current output channel pattern.
 *
 * CLK: Clock input used to advance the patterns.
 *
 * RST: Trigger this input to reset all patterns.
 *
 */
#include <FlexiTimer2.h>

// Include the Modulove hardware library.
#include <arythmatik.h>

#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

// Script specific helper libraries.
#include "pattern.h"
#include "save_state.h"

// Graphics identifiers
#define RIGHT_TRIANGLE 0x10
#define LEFT_TRIANGLE 0x11

// 'pencil', 12x12px
const unsigned char pencil_gfx[] PROGMEM = {
    0x00, 0x00, 0x00, 0xc0, 0x01, 0xa0, 0x02, 0x60, 0x04, 0x40, 0x08, 0x80, 0x11, 0x00, 0x22, 0x00,
    0x64, 0x00, 0x78, 0x00, 0x70, 0x00, 0x00, 0x00};

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Flag for rotating the panel 180 degrees.
// #define ROTATE_PANEL

// Flag for reversing the encoder direction.
// #define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

const byte TRIGGER_DURATION_MS = 15;
const byte UI_TRIGGER_DURATION_MS = 200;

const int MAX_TEMPO = 250;
const int MIN_TEMPO = 20;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;
Pattern patterns[OUTPUT_COUNT];
bool state_changed = false;
bool update_internal_clock = false;

// State variables that may be changed inside ISR must be volatile.
volatile unsigned long last_clock_input = 0;
volatile bool update_display = true;
volatile bool trigger_active = false;
volatile bool ui_trigger_active = false;

// Enum constants for selecting an editable attribute.
enum Parameter {
    PARAM_STEPS,
    PARAM_HITS,
    PARAM_OFFSET,
    PARAM_PADDING,
    PARAM_LAST,
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

    // Dsiable echo clock when using Interrupt for CLK input.
    hw.config.DisableEchoClock = true;

    // Thanks to Sitka Instruments for the tip and docs from https://dronebotworkshop.com/interrupts/
    // Enable PCIE0 Bit0 = 1 (Port B)
    PCICR |= B00000001;
    // Enable PCINT5 & PCINT3 (Pin 13 & Pin 11)
    PCMSK0 |= B00101000;
    // ISR (PCINT0_vect) - ISR for Port B (D8 - D13)

    // Set up encoder parameters
    hw.eb.setEncoderHandler(HandleRotate);
    hw.eb.setClickHandler(HandlePress);
    hw.eb.setLongPressHandler(HandleLongPress);
    hw.eb.setEncoderPressedHandler(HandlePressedRotation);
    // Reduce encoder read rate to read no more than once every 50ms.
    hw.eb.setRateLimit(50);

    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Initial patterns.
    InitState(patterns);

    if (state.internal_clock) {
        StartClock();
    }

    // Display each clock division on the OLED.
    UpdateDisplay();
}

void loop() {
    // Read cv inputs and process encoder state to determine state for this
    // loop.
    hw.ProcessInputs();

    // If internal clock was changed, restart clock:
    if (update_internal_clock) {
        update_internal_clock = false;
        FlexiTimer2::stop();
        if (state.internal_clock) {
            StartClock();
        }
    }

    // Turn off all outputs after trigger duration.
    if (trigger_active && millis() > last_clock_input + TRIGGER_DURATION_MS) {
        if (state.output_mode == TRIGGER) {
            for (int i = 0; i < OUTPUT_COUNT; i++) {
                hw.outputs[i].Low();
            }
        }
        trigger_active = false;
    }

    // Turn off current step animation after ui trigger duration.
    if (ui_trigger_active && millis() > last_clock_input + UI_TRIGGER_DURATION_MS) {
        ui_trigger_active = false;
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

void StartClock() {
    // 4PPQN period in milliseconds.
    unsigned long interval = (60.0 * 1000) / double(state.tempo) / 4.0;
    FlexiTimer2::set(interval, 1.0 / 1000, PlayStep);
    FlexiTimer2::start();
}

void PlayStep() {
    GatesOff();
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        if (patterns[i].NextStep() == 1) {
            if (state.output_mode == FLIP) {
                hw.outputs[i].On()
                    ? hw.outputs[i].Low()
                    : hw.outputs[i].High();
            } else {
                hw.outputs[i].High();
            }
        }
    }
    last_clock_input = millis();
    trigger_active = true;
    ui_trigger_active = true;
    update_display = true;
}

void GatesOff() {
    if (state.output_mode == GATE) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            hw.outputs[i].Low();
        }
    }
}

// Pin Change Interrupt on Port B (D13 CLK).
ISR(PCINT0_vect) {
    // Reset all patterns to the first pattern step on RST input.
    if (hw.rst.Read() == HIGH) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            patterns[i].Reset();
        }
        update_display = true;
    }

    // Ignore CLK handler if using internal clock.
    if (state.internal_clock) return;

    // Advance the patterns on CLK input
    if (hw.clk.Read() == HIGH) {
        PlayStep();
        digitalWrite(CLOCK_LED, HIGH);
    } else {
        digitalWrite(CLOCK_LED, LOW);
    }
}

void HandlePress(EncoderButton &eb) {
    switch (selected_mode) {
        case UIMODE_PAGE:
            selected_mode = UIMODE_SELECT;
            break;

        case UIMODE_EDIT:
            switch (selected_page) {
                case PAGE_MAIN:
                    // If leaving EDIT mode, save state.
                    selected_mode = UIMODE_SELECT;
                    break;
                case PAGE_CLOCK:
                    selected_mode = UIMODE_SELECT;
                    selected_page = PAGE_MAIN;
                    update_internal_clock = true;
                    break;
            }
            state_changed = true;
            break;

        case UIMODE_SELECT:
            switch (selected_page) {
                case PAGE_MAIN:
                    selected_mode = UIMODE_EDIT;
                    break;
                case PAGE_MODE:
                    selected_mode = UIMODE_SELECT;
                    selected_page = PAGE_MAIN;
                    state_changed = true;
                    break;
                case PAGE_CLOCK:
                    if (state.internal_clock) {
                        selected_mode = UIMODE_EDIT;
                    } else {
                        selected_mode = UIMODE_SELECT;
                        selected_page = PAGE_MAIN;
                        update_internal_clock = true;
                        state_changed = true;
                    }
                    break;
            }
            break;
    }
    update_display = true;
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
        case PAGE_CLOCK:
            UpdateClock(dir);
            break;
    }
}

// When in select page mode, scroll through menu pages.
void UpdatePage(Direction dir) {
    if (dir == DIRECTION_INCREMENT && selected_page < PAGE_LAST - 1) {
        selected_page = static_cast<MenuPage>((selected_page + 1) % PAGE_LAST);
        update_display = true;
    } else if (dir == DIRECTION_DECREMENT && selected_page > PAGE_MAIN) {
        selected_page = static_cast<MenuPage>((selected_page - 1) % PAGE_LAST);
        update_display = true;
    }
}

void UpdateParameter(Direction dir) {
    // Convert the configured encoder direction to a single integer equivalent value.
    int val = dir == DIRECTION_INCREMENT ? 1 : -1;

    switch (selected_mode) {
        case UIMODE_SELECT:
            if (static_cast<Parameter>(selected_param) == 0 && val < 0) {
                selected_param = static_cast<Parameter>(PARAM_LAST - 1);
            } else {
                selected_param = static_cast<Parameter>((selected_param + val) % PARAM_LAST);
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
    update_display = true;
}

// Select between internal and external clock mode.
void UpdateClock(Direction dir) {
    switch (selected_mode) {
        case UIMODE_SELECT:
            state.internal_clock = (dir == DIRECTION_INCREMENT);
            break;
        case UIMODE_EDIT:
            // Read the accelerated amount of encoder rotations for adjusting tempo.
            int amount = hw.eb.increment() * hw.eb.increment();
            if (dir == DIRECTION_INCREMENT && state.tempo < MAX_TEMPO) {
                state.tempo = min(state.tempo + amount, MAX_TEMPO);
            } else if (dir == DIRECTION_DECREMENT && state.tempo > 0) {
                state.tempo = max(state.tempo - amount, MIN_TEMPO);
            }
            break;
    }
    update_display = true;
}

void HandlePressedRotation(EncoderButton &eb) {
    if (selected_page == PAGE_MAIN) {
        ChangeSelectedOutput(hw.EncoderDirection());
    }
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

void DisplayMainPage() {
    hw.display.setCursor(37, 0);
    hw.display.println(F("EUCLIDEAN"));
    hw.display.drawLine(0, 10, 128, 10, 1);
    DisplayParam();
    DisplayChannels();
    DisplayPattern();
}

void DisplayParam() {
    hw.display.setCursor(26, 18);
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
            if (selected_page == PAGE_MAIN || selected_page == PAGE_CLOCK) {
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
    hw.display.setCursor(4, 26);
    hw.display.setFont(&FreeSansBold9pt7b);
    hw.display.println(state.selected_out + 1);
    hw.display.setFont();

    const byte start = 42;
    const byte boxSize = 4;
    const byte margin = 2;
    const byte wrap = 3;
    byte top = start;
    byte left = 4;

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
    const byte start = 26;
    const byte boxSize = 7;
    const byte margin = 2;
    const byte wrap = 8;
    byte top = 30;
    byte left = start;

    byte steps = patterns[state.selected_out].steps;
    byte offset = patterns[state.selected_out].offset;
    byte padding = patterns[state.selected_out].padding;

    for (int i = 0; i < steps + padding; i++) {
        // Draw box, fill current step.
        switch (patterns[state.selected_out].GetStep(i)) {
            case 1:
                hw.display.fillRect(left, top, boxSize, boxSize, 1);
                break;
            case 0:
                hw.display.drawRect(left, top, boxSize, boxSize, 1);
                break;
            case 2:
                hw.display.drawRect(left, top, boxSize, boxSize, 1);
                hw.display.drawLine(left + boxSize - 1, top, left, top + boxSize - 1, 1);
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
    hw.display.setCursor(32, 0);
    hw.display.println(F("OUTPUT MODE"));
    hw.display.drawLine(0, 10, 128, 10, 1);
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
    hw.display.setCursor(48, 0);
    hw.display.println(F("CLOCK"));
    hw.display.drawLine(0, 10, 128, 10, 1);

    hw.display.setCursor(26, 18);
    hw.display.print(F("Mode: "));
    hw.display.println((state.internal_clock) ? F("INTERNAL") : F("EXTERNAL"));

    if (state.internal_clock) {
        (state.tempo > 99)
            ? hw.display.setCursor(34, 50)
            : hw.display.setCursor(46, 50);
        hw.display.setFont(&FreeSans18pt7b);
        hw.display.print(state.tempo);
        hw.display.setFont();
    }
}