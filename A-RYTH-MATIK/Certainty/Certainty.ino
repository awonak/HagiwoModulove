/**
 * @file Certainty.ino
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

// Enum constants for user editable parameters.
enum Parameter {
    PARAM_NONE,
    PARAM_SEED,
    PARAM_LENGTH,
    PARAM_LAST,
};
Parameter selected_param = PARAM_NONE;

// Script state variables.
uint16_t seed;             // Store the current seed used for psudo random number generator
uint8_t step_length = 16;  // Length of psudo random trigger sequence
uint8_t step_count = 0;    // Count of trigger steps since reset

int clk = 0;  // External CLK trigger input read value
int old_clk = 0;
int rst = 0;  // External RST trigger input read value
int old_rst = 0;

// State variables for tracking OLED refresh changes.
bool state_changed = true;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(9600);
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

    // Determine current RST input state.
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
        if (step_count == 0) Reseed();
        state_changed = true;
    }

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        // With the seeded random, probablistic random calls will be determistic.
        outputs[i].Update(clk_state);
    }

    // Read for a button press event.
    if (encoder.push()) {
        selected_param = static_cast<Parameter>((selected_param + 1) % PARAM_LAST);
        state_changed = true;
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
    if (_seed != 0) {
        seed = _seed;
    } else {
        NewSeed();
    }
    // Read previously stored step_length from EEPROM memory. Update step_length if the value is non-zero.
    uint8_t _step_length;
    EEPROM.get(LENGTH_ADDR, _step_length);
    if (_step_length != 0) {
        SetLength(_step_length);
    }
}

// Reset the seed and pattern length to restart the psudo random deterministic pattern.
void Reset() {
    Reseed();
    step_count = 0;
    state_changed = true;
}

// Generate a new random seed and store it in EEPROM.
void NewSeed() {
    randomSeed(micros());
    seed = random(UINT16_MAX);
    EEPROM.put(SEED_ADDR, seed);
    Reseed();
    state_changed = true;
}

// Set the pattern length and store it in EEPROM.
void SetLength(uint8_t _step_length) {
    step_length = constrain(_step_length, MIN_LENGTH, MAX_LENGTH);
    EEPROM.put(LENGTH_ADDR, step_length);
    state_changed = true;
}

// Reseed the random number generator with the current seed.
void Reseed() {
    randomSeed(seed);
}

// Update the current selected parameter with the current movement of the encoder.
void UpdateParameter(byte encoder_dir) {
    if (selected_param == PARAM_SEED) UpdateSeed(encoder_dir);
    if (selected_param == PARAM_LENGTH) UpdateLength(encoder_dir);
}

// Right now just randomize up or down from current seed.
void UpdateSeed(byte dir) {
    if (dir != 0) NewSeed();
}

// Adjust the step length.
void UpdateLength(byte dir) {
    if (dir == 1 && step_length <= MAX_LENGTH) SetLength(++step_length);
    if (dir == 2 && step_length >= MIN_LENGTH) SetLength(--step_length);
}

// Display the current state of the module params.
void UpdateDisplay() {
    if (!state_changed) return;
    state_changed = false;
    display.clearDisplay();

    // Draw a die and the hex seed value on the top.
    display.drawRoundRect(38, 1, 9, 9, 2, 1);
    display.drawPixel(41, 3, 1);
    display.drawPixel(44, 6, 1);
    display.setCursor(50, 2);
    display.println(String(seed, HEX));

    // Show edit icon for seed if it's selected.
    if (selected_param == PARAM_SEED) {
        display.drawChar(28, 1, 0x10, 1, 0, 1);
    }

    // Draw boxes for pattern length.
    {
        int start = 20;
        int top = 14;
        int left = start;
        int boxSize = 7;
        int padding = 2;
        int wrap = 8;

        for (int i = 1; i <= step_length; i++) {
            // Determine how much top padding to use.
            int _top = top;
            if(step_length <= 8) _top += 7;
            if(step_length <= 16) _top += 5;
            if(step_length <= 24) _top += 3;
            // Draw box, fill current step.
            (i == step_count+1)
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
    }

    // Draw the current step and length on the bottom.
    display.setCursor(8, 56);
    display.println("Step:" + String(step_count + 1));
    display.setCursor(64, 56);
    display.println("Length:" + String(step_length));

    display.display();
}