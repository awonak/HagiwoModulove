/**
 * @file BitGarden.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Deterministic psudo random gate processor. Somewhat an inversion of the Olivia Artz Modular's Uncertainty.
 * @version 0.1
 * @date 2023-07-28
 *
 * @copyright Copyright (c) 2023
 *
 * Connect a trigger or gate source to the CLK input and the each output will
 * mirror that signal according to a decreasing deterministic probability set
 * by the seed value. RST input will reset the psudo random sequence. Use the
 * encoder to randomize the seed or adjust the pattern length. The user
 * configurable parameters of seed and step length will be saved to EEPROM and
 * will be recalled the next time you power on the module.
 *
 * Encoder:
 *      short press: Toggle between editing the step length and selecting a seed.
 *      long press: Enter seed edit mode to manually provide a seed value.
 *
 * CLK: Provide a gate or trigger for each output to repeat with decreasing
 *      probability in each output.
 *
 * RST: Trigger this input to reseed the psudo random sequence.
 *
 */

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

// Include the Modulove hardware library.
#include "src/libmodulove/arythmatik.h"

// Script specific output class.
#include "output.h"
#include "seed_packet.h"

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

FixedProbablisticOutput outputs[OUTPUT_COUNT];

// Script config definitions
const uint8_t MIN_LENGTH = 4;
const uint8_t MAX_LENGTH = 32;

// EEPROM address references for saving state.
const int SEED_ADDR = 0;
const int LENGTH_ADDR = sizeof(uint16_t);

// Enum constants for current display page.
enum MenuPage {
    PAGE_MAIN,
    PAGE_SEED,
};
MenuPage selected_page = PAGE_MAIN;

// Enum constants for user editable parameters.
enum Parameter {
    PARAM_NONE,
    PARAM_LENGTH,
    PARAM_SEED,
    PARAM_LAST,
};
Parameter selected_param = PARAM_NONE;

// Script state variables.
uint8_t step_length = 16;  // Length of psudo random trigger sequence (default 16 steps)
uint8_t step_count = 0;    // Count of trigger steps since reset
SeedPacket packet;         // SeedPacket contains the buffer of previous seeds
uint8_t seed_index;        // Indicated the seed digit to edit on Seed page.
uint16_t temp_seed;        // Temporary seed for editing the current seed.

// State variables for tracking OLED and editable parameter changes.
bool state_changed = true;
bool update_display = true;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(115200);
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

    // Initial random seed and step length from EEPROM or default values.
    InitState();
}

void loop() {
    // Read inputs to determine state.
    hw.ProcessInputs();

    // When RST goes high, reseed and reset.
    if (hw.rst.State() == DigitalInput::STATE_RISING) {
        Reset();
    }

    // When step count wraps, reset step count and reseed.
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        step_count = ++step_count % step_length;
        if (step_count == 0) packet.Reseed();
        update_display = true;
    }

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        // With the seeded random, probablistic random calls will be determistic.
        outputs[i].Update(hw.clk.State());
    }

    // Read the encoder button press event.
    Encoder::PressType press = hw.encoder.Pressed();

    // Short button press. Change editable parameter.
    if (press == Encoder::PRESS_SHORT) {
        // Next param on Main page.
        if (selected_page == PAGE_MAIN)
            selected_param = static_cast<Parameter>((selected_param + 1) % PARAM_LAST);

        // Next seed digit on Seed page.
        else if (selected_page == PAGE_SEED)
            seed_index = ++seed_index % 4;

        // Save changes made from previous parameter edits.
        SaveChanges();
        update_display = true;
    }
    // Long button press. Change menu page.
    else if (press == Encoder::PRESS_LONG) {
        if (selected_page == PAGE_MAIN) {
            selected_param = PARAM_NONE;
            temp_seed = packet.GetSeed();
            selected_page = PAGE_SEED;
        } else {
            seed_index = 0;
            packet.UpdateSeed(temp_seed);
            selected_page = PAGE_MAIN;
        }
        update_display = true;
    }

    // Read encoder for a change in direction and update the selected parameter.
    UpdateParameter(hw.encoder.Rotate());

    // Render any new UI changes to the OLED display.
    UpdateDisplay();
}

// Initialize random seed and step length from EEPROM or default values.
void InitState() {
    // Read previously stored seed from EEPROM memory. If it is empty, generate a new random seed.
    uint16_t _seed;
    EEPROM.get(SEED_ADDR, _seed);
    if (_seed != 0xFFFF) {
        packet.SetSeed(_seed);
    } else {
        packet.NewRandomSeed();
    }
    // Read previously stored step_length from EEPROM memory. Update step_length if the value is non-zero.
    uint8_t _step_length;
    EEPROM.get(LENGTH_ADDR, _step_length);
    if (_step_length != 0 && _step_length != 0xFF) {
        SetLength(_step_length);
    }
}

// Save state to EEPROM if state has changed.
void SaveChanges() {
    if (!state_changed) return;
    state_changed = false;
    EEPROM.put(LENGTH_ADDR, step_length);
    EEPROM.put(SEED_ADDR, packet.GetSeed());
}

// Reset the pattern sequence and reseed the psudo random deterministic pattern.
void Reset() {
    packet.NewRandomSeed();
    step_count = 0;
    update_display = true;
}

// Update the current selected parameter with the current movement of the encoder.
void UpdateParameter(Encoder::Direction dir) {
    if (selected_page == PAGE_MAIN) {
        if (selected_param == PARAM_SEED) UpdateSeed(dir);
        if (selected_param == PARAM_LENGTH) UpdateLength(dir);
    }
    if (selected_page == PAGE_SEED) {
        EditSeed(dir);
    }
}

// Randomize the current seed if the encoder has moved in either direction.
void UpdateSeed(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED) return;
    else if (dir == Encoder::DIRECTION_INCREMENT) packet.NextSeed();
    else if (dir == Encoder::DIRECTION_DECREMENT) packet.PrevSeed();
    update_display = true;
    state_changed = true;
}

// Adjust the step length for the given input direction (1=increment, 2=decrement).
void UpdateLength(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_INCREMENT && step_length <= MAX_LENGTH) {
        SetLength(++step_length);
    } else if (dir == Encoder::DIRECTION_DECREMENT && step_length >= MIN_LENGTH) {
        SetLength(--step_length);
    }
}

// Edit the current seed.
void EditSeed(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED) return;

    int change;
    if (seed_index == 0)
        change = 0x1000;
    else if (seed_index == 1)
        change = 0x0100;
    else if (seed_index == 2)
        change = 0x0010;
    else if (seed_index == 3)
        change = 0x0001;

    if (dir == Encoder::DIRECTION_INCREMENT)
        temp_seed += change;
    else if (dir == Encoder::DIRECTION_DECREMENT)
        temp_seed -= change;

    update_display = true;
}

// Set the pattern length..
void SetLength(uint8_t _step_length) {
    step_length = constrain(_step_length, MIN_LENGTH, MAX_LENGTH);
    update_display = true;
    state_changed = true;
}

// UI display of app state.
void UpdateDisplay() {
    // Only update when ui state changes.
    if (!update_display) return;
    update_display = false;

    hw.display.clearDisplay();

    switch (selected_page) {
        case PAGE_MAIN:
            DisplayMainPage();
            break;
        case PAGE_SEED:
            DisplaySeedPage();
            break;
        default:
            break;
    }

    hw.display.display();
}

void PageTitle(String title) {
    // Draw page title.
    hw.display.setTextSize(0);
    hw.display.setCursor(36, 0);
    hw.display.println(title);
    hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);
}

void DisplayMainPage() {
    PageTitle("BIT GARDEN");
    // Draw boxes for pattern length.
    int start = 26;
    int top = 16;
    int left = start;
    int boxSize = 7;
    int padding = 2;
    int wrap = 8;

    for (int i = 1; i <= step_length; i++) {
        // Determine how much top padding to use.
        int _top = top;
        if (step_length <= 8) _top += 8;
        else if (step_length <= 16) _top += 6;
        else if (step_length <= 24) _top += 4;
        // Draw box, fill current step.
        (i == step_count + 1)
            ? hw.display.fillRect(left, _top, boxSize, boxSize, 1)
            : hw.display.drawRect(left, _top, boxSize, boxSize, 1);

        // Advance the draw cursors.
        left += boxSize + padding + 1;

        // Show edit icon for length if it's selected.
        if (selected_param == PARAM_LENGTH && i == step_length) {
            hw.display.drawChar(left, _top, 0x11, 1, 0, 1);
        }

        // Wrap the box draw cursor if we hit wrap count.
        if (i % wrap == 0) {
            top += boxSize + padding;
            left = start;
        }
    }

    // Draw the current step and length on the bottom left.
    hw.display.setCursor(8, 56);
    String str_step = (step_count < 9)
                          ? " " + String(step_count + 1)
                          : String(step_count + 1);
    hw.display.println("Step:" + str_step + "/" + String(step_length));

    // Draw a die and the hex seed value on the bottom right.
    hw.display.drawRoundRect(82, 55, 9, 9, 2, 1);
    hw.display.drawPixel(85, 57, 1);
    hw.display.drawPixel(87, 60, 1);
    hw.display.setCursor(94, 56);
    hw.display.println(String(packet.GetSeed(), HEX));

    // Show edit icon for seed if it's selected.
    if (selected_param == PARAM_SEED) {
        hw.display.drawChar(120, 56, 0x11, 1, 0, 1);
    }
}

void DisplaySeedPage() {
    PageTitle("EDIT SEED");
    // Draw seed
    hw.display.setTextSize(2);
    hw.display.setCursor(42, 32);
    hw.display.println(String(temp_seed, HEX));

    // Draw line under current editable digit.
    int start = 42;
    int top = 50;
    hw.display.drawFastHLine(start + (seed_index * 12), top, 10, WHITE);
}
