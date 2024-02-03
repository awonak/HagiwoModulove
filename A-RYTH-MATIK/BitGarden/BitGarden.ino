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

// Flag for reversing the encoder direction.
// #define ENCODER_REVERSED

// Graphics identifiers
#define RIGHT_TRIANGLE 0x10
#define LEFT_TRIANGLE 0x11
#define UP_ARROW 0x18
#define PERCENT 0x25

// Include the Modulove hardware library.
#include "src/libmodulove/arythmatik.h"

// Script specific output class.
#include "output.h"
#include "seed_packet.h"

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

ProbablisticOutput outputs[OUTPUT_COUNT];

// Script config definitions
const uint8_t MIN_LENGTH = 4;
const uint8_t MAX_LENGTH = 32;

// Enum constants for current display page.
enum MenuPage {
    PAGE_MAIN,
    PAGE_MODE,
    PAGE_PROB,
    PAGE_SEED,
    PAGE_LAST,
};
MenuPage selected_page = PAGE_MAIN;

// Enum constants for user editable parameters.
enum MainParameter {
    PARAM_NONE,
    PARAM_LENGTH,
    PARAM_SEED,
    PARAM_LAST,
};
MainParameter selected_param = PARAM_NONE;

enum ProbabilityParameter {
    PROB_OUTPUT,
    PROB_PERCENTAGE,
    PROB_LAST,
};
ProbabilityParameter prob_param = PROB_OUTPUT;

// Script helper variables.
uint8_t step_count = 0;  // Count of trigger steps since reset
SeedPacket packet;       // SeedPacket contains the buffer of previous seeds
uint8_t seed_index;      // Indicated the seed digit to edit on Seed page.
uint16_t temp_seed;      // Temporary seed for editing the current seed.
Mode mode = TRIGGER;     // Current state for ouput behavior.
byte selected_out = 0;   // Selected output for changing probability.

// Script state & storage variables.
// Expected version string for this firmware.
const char SCRIPT_NAME[] = "BIT GARDEN";
const uint8_t SCRIPT_VER = 2;

struct State {
    // Version check.
    char script[sizeof(SCRIPT_NAME)];
    uint8_t version;
    // State variables.
    uint8_t step_length;
    uint16_t seed;
    Mode mode;
    uint16_t probability[OUTPUT_COUNT];
};
State state;

// State variables for tracking OLED and editable parameter changes.
bool page_select = false;
bool state_changed = false;
bool update_display = true;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(115200);
#endif
    // Initialize the A-RYTH-MATIK peripherials.
    hw.Init();

    // Initial state variables from EEPROM or default values.
    InitState();

    // Update seed packet with the initialized seed.
    packet.SetSeed(state.seed);

    // Initialize the outputs & probabilities from InitState.
    for (int i; i < OUTPUT_COUNT; i++) {
        outputs[i].Init(hw.outputs[i], float(state.probability[i])/100.0f);
    }
}

void loop() {
    // Read inputs to determine state.
    hw.ProcessInputs();

    // When RST goes high, reseed and reset.
    if (hw.rst.State() == DigitalInput::STATE_RISING) {
        Reset();
    }

    // Input clock has gone high, call each output's On() for a chance to
    // trigger that output.
    if (hw.clk.State() == DigitalInput::STATE_RISING) {
        // When step count wraps, reset step count and reseed.
        step_count = ++step_count % state.step_length;
        if (step_count == 0) packet.Reseed();

        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].On();
        }
        update_display = true;
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
        HandleShortPress();
    }
    // Long button press. Change menu page.
    else if (press == Encoder::PRESS_LONG) {
        HandleLongPress();
    }

    // Read encoder for a change in direction and update the selected page or
    // parameter.
    (page_select)
        ? UpdatePage(hw.encoder.Rotate())
        : UpdateParameter(hw.encoder.Rotate());

    // Render any new UI changes to the OLED display.
    UpdateDisplay();
}

// Initialize random seed and step length from EEPROM or default values.
void InitState() {
    // Read previously put Storage struct from EEPROM memory. If it doesn't
    EEPROM.get(0, state);

    // Check if the data in memory matches expected values.
    if ((strcmp(state.script, SCRIPT_NAME) != 0) || (state.version != SCRIPT_VER)) {
        // Set script version identifier values.
        strcpy(state.script, SCRIPT_NAME);
        state.version = SCRIPT_VER;

        // Default step length.
        state.step_length = 16;

        // Use a new arbitrary random seed for state.
        state.seed = 0x8f26;

        // Default mode.
        state.mode = Mode::TRIGGER;

        // Provide a even distribution of default probabilities.
        state.probability[0] = 92;
        state.probability[1] = 86;
        state.probability[2] = 64;
        state.probability[3] = 48;
        state.probability[4] = 32;
        state.probability[5] = 18;
    }
}

// Save state to EEPROM if state has changed.
void SaveChanges() {
    if (!state_changed) return;
    state_changed = false;
    state.seed = packet.GetSeed();
    uint16_t _prob[OUTPUT_COUNT] = {
        outputs[0].GetProbInt(),
        outputs[1].GetProbInt(),
        outputs[2].GetProbInt(),
        outputs[3].GetProbInt(),
        outputs[4].GetProbInt(),
        outputs[5].GetProbInt(),
    };
    memccpy(state.probability, _prob, sizeof(_prob), sizeof(_prob));
    EEPROM.put(0, state);
}

// Reset the pattern sequence and reseed the psudo random deterministic pattern.
void Reset() {
    packet.NewRandomSeed();
    step_count = 0;
    update_display = true;
}

// Short press handler.
void HandleShortPress() {
    // If we are in page select mode, set current page.
    if (page_select) {
        page_select = false;
    }

    // Next param on Main page.
    else if (selected_page == PAGE_MAIN) {
        switch (selected_param) {
            case PARAM_NONE:
                selected_param = PARAM_LENGTH;
                break;
            case PARAM_LENGTH:
                selected_param = PARAM_SEED;
                break;
            case PARAM_SEED:
                selected_param = PARAM_NONE;
        }
    }

    // Toggle between output and probability parameter selection.
    else if (selected_page == PAGE_PROB) {
        prob_param = static_cast<ProbabilityParameter>((prob_param + 1) % PROB_LAST);
    }

    // Next seed digit on Seed page.
    else if (selected_page == PAGE_SEED) {
        seed_index = --seed_index % 4;
    }

    // Short press on Output Mode page will return to the page selection mode.
    else if (selected_page == PAGE_MODE) {
        page_select = true;
    }
    SaveChanges();
    update_display = true;
}

// Long press handler.will toggle between page select mode and parameter edit
// mode for the current page.
void HandleLongPress() {
    // Toggle between menu page select mode.
    if (!page_select) {
        page_select = true;
        selected_param = PARAM_NONE;
        temp_seed = packet.GetSeed();
        seed_index = 3;  // Least significant digit
    } else {
        page_select = false;
        selected_param = PARAM_NONE;
    }
    SaveChanges();
    update_display = true;
}

// When in select page mode, scroll through menu pages.
void UpdatePage(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED)
        return;
    else if (dir == Encoder::DIRECTION_INCREMENT && selected_page < PAGE_LAST - 1)
        selected_page = static_cast<MenuPage>((selected_page + 1) % PAGE_LAST);
    else if (dir == Encoder::DIRECTION_DECREMENT && selected_page > PAGE_MAIN)
        selected_page = static_cast<MenuPage>((selected_page - 1) % PAGE_LAST);
    update_display = true;
}

// Update the current selected parameter with the current movement of the encoder.
void UpdateParameter(Encoder::Direction dir) {
    if (selected_page == PAGE_MAIN) {
        if (selected_param == PARAM_SEED) UpdateSeed(dir);
        if (selected_param == PARAM_LENGTH) UpdateLength(dir);
    } else if (selected_page == PAGE_SEED) {
        EditSeed(dir);
    } else if (selected_page == PAGE_MODE) {
        UpdateMode(dir);
    } else if (selected_page == PAGE_PROB) {
        UpdateProbability(dir);
    }
}

// Select seed from the previous seeds in the packet or add new random seed to packet.
void UpdateSeed(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED)
        return;
    else if (dir == Encoder::DIRECTION_INCREMENT)
        packet.NextSeed();
    else if (dir == Encoder::DIRECTION_DECREMENT)
        packet.PrevSeed();
    update_display = true;
    state_changed = true;
}

// Adjust the step length for the given input direction (1=increment, 2=decrement).
void UpdateLength(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_INCREMENT && state.step_length <= MAX_LENGTH) {
        SetLength(++state.step_length);
    } else if (dir == Encoder::DIRECTION_DECREMENT && state.step_length >= MIN_LENGTH) {
        SetLength(--state.step_length);
    }
}

// Change the current output mode selection.
void UpdateMode(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED) return;

    if (dir == Encoder::DIRECTION_INCREMENT && state.mode < MODE_LAST - 1) {
        state.mode = static_cast<Mode>(state.mode + 1);

    } else if (dir == Encoder::DIRECTION_DECREMENT && state.mode > 0) {
        state.mode = static_cast<Mode>(state.mode - 1);
    }
    // Update the mode for all outputs.
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        outputs[i].SetMode(state.mode);
    }
    update_display = true;
    state_changed = true;
}

void UpdateProbability(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_UNCHANGED) return;
    if (prob_param == PROB_OUTPUT) UpdateOutput(dir);
    if (prob_param == PROB_PERCENTAGE) UpdatePercentage(dir);
}

void UpdateOutput(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_INCREMENT && selected_out < OUTPUT_COUNT - 1)
        selected_out++;
    if (dir == Encoder::DIRECTION_DECREMENT && selected_out > 0)
        selected_out--;
    update_display = true;
}

void UpdatePercentage(Encoder::Direction dir) {
    if (dir == Encoder::DIRECTION_INCREMENT) outputs[selected_out].IncProb();
    if (dir == Encoder::DIRECTION_DECREMENT) outputs[selected_out].DecProb();
    update_display = true;
    state_changed = true;
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

    packet.UpdateSeed(temp_seed);
    state_changed = true;
    update_display = true;
}

// Set the pattern length..
void SetLength(uint8_t _step_length) {
    state.step_length = constrain(_step_length, MIN_LENGTH, MAX_LENGTH);
    update_display = true;
    state_changed = true;
}

// UI display of app state.
void UpdateDisplay() {
    // Only update when ui state changes.
    if (!update_display) return;
    update_display = false;

    hw.display.clearDisplay();

    // Show UI indicator if in page select mode.
    if (page_select) {
        hw.display.drawChar(4, 0, RIGHT_TRIANGLE, 1, 0, 1);
    }

    switch (selected_page) {
        case PAGE_MAIN:
            DisplayMainPage();
            break;
        case PAGE_SEED:
            DisplaySeedPage();
            break;
        case PAGE_MODE:
            DisplayOutputModePage();
            break;
        case PAGE_PROB:
            DisplayProbabilityPage();
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

    for (int i = 1; i <= state.step_length; i++) {
        // Determine how much top padding to use.
        int _top = top;
        if (state.step_length <= 8)
            _top += 8;
        else if (state.step_length <= 16)
            _top += 6;
        else if (state.step_length <= 24)
            _top += 4;
        // Draw box, fill current step.
        (i == step_count + 1)
            ? hw.display.fillRect(left, _top, boxSize, boxSize, 1)
            : hw.display.drawRect(left, _top, boxSize, boxSize, 1);

        // Advance the draw cursors.
        left += boxSize + padding + 1;

        // Show edit icon for length if it's selected.
        if (selected_param == PARAM_LENGTH && i == state.step_length) {
            hw.display.drawChar(left, _top, LEFT_TRIANGLE, 1, 0, 1);
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
    hw.display.println("Step:" + str_step + "/" + String(state.step_length));

    // Draw a die and the hex seed value on the bottom right.
    hw.display.drawRoundRect(82, 55, 9, 9, 2, 1);
    hw.display.drawPixel(85, 57, 1);
    hw.display.drawPixel(87, 60, 1);
    hw.display.setCursor(94, 56);
    hw.display.println(String(packet.GetSeed(), HEX));

    // Show edit icon for seed if it's selected.
    if (selected_param == PARAM_SEED) {
        hw.display.drawChar(120, 56, LEFT_TRIANGLE, 1, 0, 1);
    }
}

void DisplaySeedPage() {
    PageTitle("EDIT SEED");
    // Draw seed
    int xoffset;
    if (temp_seed >= 0x1000)
        xoffset = 42;
    else if (temp_seed >= 0x0100)
        xoffset = 54;
    else if (temp_seed >= 0x0010)
        xoffset = 66;
    else if (temp_seed >= 0x0001)
        xoffset = 78;
    hw.display.setTextSize(2);
    hw.display.setCursor(xoffset, 32);
    hw.display.println(String(temp_seed, HEX));

    if (!page_select) {
        // Draw line under current editable digit.
        int start = 42;
        int top = 50;
        hw.display.drawFastHLine(start + (seed_index * 12), top, 10, WHITE);
    }
}

void DisplayOutputModePage() {
    PageTitle("OUTPUT MODE");
    // Draw output modes
    (state.mode == TRIGGER)
        ? hw.display.fillRect(12, 20, 8, 8, 1)
        : hw.display.drawRect(12, 20, 8, 8, 1);

    (state.mode == GATE)
        ? hw.display.fillRect(12, 32, 8, 8, 1)
        : hw.display.drawRect(12, 32, 8, 8, 1);

    (state.mode == FLIP)
        ? hw.display.fillRect(12, 44, 8, 8, 1)
        : hw.display.drawRect(12, 44, 8, 8, 1);

    hw.display.setCursor(32, 20);
    hw.display.println(String("Trig Mode"));
    hw.display.setCursor(32, 32);
    hw.display.println(String("Gate Mode"));
    hw.display.setCursor(32, 46);
    hw.display.println(String("Flip Mode"));

    // Draw edit cursor.
    if (!page_select) {
        // Draw line under current editable digit.
        int start = 4;
        int top = 20;
        int yoffset = 12;
        int _mode = static_cast<int>(state.mode);
        hw.display.drawChar(start, top + (_mode * yoffset), RIGHT_TRIANGLE, WHITE, BLACK, 1);
    }
}

void DisplayProbabilityPage() {
    PageTitle("PROBABILITY");
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
        if (i == selected_out & !page_select) {
            (prob_param == PROB_OUTPUT)
                ? hw.display.drawChar(left + 2, SCREEN_HEIGHT - 6, UP_ARROW, 1, 0, 1)
                : hw.display.drawChar(left + 2, SCREEN_HEIGHT - 6, PERCENT, 1, 0, 1);
        }

        left += barWidth + padding;
    }
}
