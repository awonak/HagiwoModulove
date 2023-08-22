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

// Oled setting
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h>

// Encoder & button
#include <SimpleRotary.h>

// Script specific output class.
#include "output.h"
#include "seed_packet.h"

// OLED Display config
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Peripheral input pins
#define ENCODER_PIN1 2
#define ENCODER_PIN2 3
#define ENCODER_SW_PIN 12
#define CLK_PIN 13
#define RST_PIN 11

// Output Pins
#define CLOCK_LED 4
#define OUT_CH1 5
#define OUT_CH2 6
#define OUT_CH3 7
#define OUT_CH4 8
#define OUT_CH5 9
#define OUT_CH6 10
#define LED_CH1 14
#define LED_CH2 15
#define LED_CH3 16
#define LED_CH4 0
#define LED_CH5 1
#define LED_CH6 17

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
// #define DEBUG

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SimpleRotary encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_SW_PIN);
FixedProbablisticOutput outputs[6];

// Script config definitions
const uint8_t OUTPUT_COUNT = 6;
const uint8_t MIN_LENGTH = 4;
const uint8_t MAX_LENGTH = 32;

// EEPROM address references for saving state.
const int SEED_ADDR = 0;
const int LENGTH_ADDR = sizeof(uint16_t);

// Enum constants for clk input rising/falling state.
enum InputState {
    STATE_UNCHANGED,
    STATE_RISING,
    STATE_FALLING,
};
InputState clk_state = STATE_UNCHANGED;

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

int clk = 0;  // External CLK trigger input read value
int old_clk = 0;
int rst = 0;  // External RST trigger input read value
int old_rst = 0;

// State variables for tracking OLED and editable parameter changes.
bool state_changed = true;
bool update_display = true;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(115200);
#endif

    // Initial random seed and step length from EEPROM or default values.
    InitState();

    // Initialize each of the outputs with it's GPIO pins and probability.
    outputs[0].Init(OUT_CH1, LED_CH1, 0.96);
    outputs[1].Init(OUT_CH2, LED_CH2, 0.82);
    outputs[2].Init(OUT_CH3, LED_CH3, 0.64);
    outputs[3].Init(OUT_CH4, LED_CH4, 0.48);
    outputs[4].Init(OUT_CH5, LED_CH5, 0.32);
    outputs[5].Init(OUT_CH6, LED_CH6, 0.18);

    // CLOCK LED (DIGITAL)
    pinMode(CLOCK_LED, OUTPUT);

    // OLED Display configuration.
    delay(1000);
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.display();
}

void loop() {
    old_clk = clk;
    clk = digitalRead(CLK_PIN);

    old_rst = rst;
    rst = digitalRead(RST_PIN);

    // When RST goes high, reseed and reset.
    if (old_rst == 0 && rst == 1) {
        Reset();
    }

    // Clock In LED indicator mirrors the clock input.
    digitalWrite(CLOCK_LED, clk);

    // Determine current clock input state.
    clk_state = STATE_UNCHANGED;
    if (old_clk == 0 && clk == 1) {
        clk_state = STATE_RISING;
    } else if (old_clk == 1 && clk == 0) {
        clk_state = STATE_FALLING;
    }

    // When step count wraps, reset step count and reseed.
    if (clk_state == STATE_RISING) {
        step_count = ++step_count % step_length;
        if (step_count == 0) packet.Reseed();
        update_display = true;
    }

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        // With the seeded random, probablistic random calls will be determistic.
        outputs[i].Update(clk_state);
    }

    // Check for long press to endable editing seed.
    // press and release for < 1 second to return 1 for short press
    // press and release for > 1 second to return 2 for long press.
    byte press = encoder.pushType(1000);

    // Short button press. Change editable parameter.
    if (press == 1) {
        // Next param on Main page.
        if (selected_page == PAGE_MAIN)
            selected_param = static_cast<Parameter>((selected_param + 1) % PARAM_LAST);

        // Next seed digit on Seed page.
        if (selected_page == PAGE_SEED)
            seed_index = ++seed_index % 4;

        // Save changes made from previous parameter edits.
        SaveChanges();
        update_display = true;
    }

    // Long button press. Change menu page.
    if (press == 2) {
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
    // rotate() returns 0 for unchanged, 1 for increment, 2 for decrement.
    UpdateParameter(encoder.rotate());

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
void UpdateParameter(byte encoder_dir) {
    if (selected_page == PAGE_MAIN) {
        if (selected_param == PARAM_SEED) UpdateSeed(encoder_dir);
        if (selected_param == PARAM_LENGTH) UpdateLength(encoder_dir);
    }
    if (selected_page == PAGE_SEED) {
        EditSeed(encoder_dir);
    }
}

// Randomize the current seed if the encoder has moved in either direction.
void UpdateSeed(byte dir) {
    if (dir == 0) return;
    if (dir == 1) packet.NextSeed();
    if (dir == 2) packet.PrevSeed();
    update_display = true;
    state_changed = true;
}

// Adjust the step length for the given input direction (1=increment, 2=decrement).
void UpdateLength(byte dir) {
    if (dir == 1 && step_length <= MAX_LENGTH) SetLength(++step_length);
    if (dir == 2 && step_length >= MIN_LENGTH) SetLength(--step_length);
}

// Edit the current seed.
void EditSeed(byte dir) {
    if (dir == 0) return;

    int change;
    if (seed_index == 0)
        change = 0x1000;
    else if (seed_index == 1)
        change = 0x0100;
    else if (seed_index == 2)
        change = 0x0010;
    else if (seed_index == 3)
        change = 0x0001;

    if (dir == 1)
        temp_seed += change;
    else if (dir == 2)
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

    display.clearDisplay();

    // Draw app title.
    display.setTextSize(0);
    display.setCursor(36, 0);
    display.println("BIT GARDEN");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

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

    display.display();
}

void DisplayMainPage() {
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
        if (step_length <= 16) _top += 6;
        if (step_length <= 24) _top += 4;
        // Draw box, fill current step.
        (i == step_count + 1)
            ? display.fillRect(left, _top, boxSize, boxSize, 1)
            : display.drawRect(left, _top, boxSize, boxSize, 1);

        // Advance the draw cursors.
        left += boxSize + padding + 1;

        // Show edit icon for length if it's selected.
        if (selected_param == PARAM_LENGTH && i == step_length) {
            display.drawChar(left, _top, 0x11, 1, 0, 1);
        }

        // Wrap the box draw cursor if we hit wrap count.
        if (i % wrap == 0) {
            top += boxSize + padding;
            left = start;
        }
    }

    // Draw the current step and length on the bottom left.
    display.setCursor(8, 56);
    String str_step = (step_count < 9)
                          ? " " + String(step_count + 1)
                          : String(step_count + 1);
    display.println("Step:" + str_step + "/" + String(step_length));

    // Draw a die and the hex seed value on the bottom right.
    display.drawRoundRect(82, 55, 9, 9, 2, 1);
    display.drawPixel(85, 57, 1);
    display.drawPixel(87, 60, 1);
    display.setCursor(94, 56);
    display.println(String(packet.GetSeed(), HEX));

    // Show edit icon for seed if it's selected.
    if (selected_param == PARAM_SEED) {
        display.drawChar(120, 56, 0x11, 1, 0, 1);
    }
}

void DisplaySeedPage() {
    // Draw seed
    display.setTextSize(2);
    display.setCursor(42, 32);
    display.println(String(temp_seed, HEX));

    // Draw line under current editable digit.
    int start = 42;
    int top = 50;
    display.drawFastHLine(start + (seed_index * 12), top, 10, WHITE);
}
