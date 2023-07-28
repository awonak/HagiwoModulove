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
 * encoder to randomize the seed or adjust the pattern length.
 */

// Oled setting
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
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
const uint8_t OUTPUT_COUNT = 6;  // Count of outputs.
const uint8_t PARAM_COUNT = 3;   // Count of editable parameters [seed, length]
const uint8_t MIN_LENGTH = 4;
const uint8_t MAX_LENGTH = 64;

// Enum constants for clk input rising/falling state.
enum InputState {
    STATE_UNCHANGED,
    STATE_RISING,
    STATE_FALLING,
};
InputState clk_state = STATE_UNCHANGED;

// Script state variables.
uint16_t seed = 27926;     // Hex 0x6d16 used as initial random seed
uint8_t step_length = 16;  // Repeat 16 psudo random triggers
uint8_t step_count = 0;    // Count of trigger steps since reset
uint8_t selected_param = 0;

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

    // Initial random seed.
    Reseed();

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
    display.setFont(&FreeMono9pt7b);
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
        if (step_count == 0) UpdateSeed(seed);
        state_changed = true;
    }

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        // With the seeded random, probablistic random calls will be determistic.
        outputs[i].Update(clk_state);
    }

    // Read for a button press event.
    if (encoder.push()) {
        selected_param = ++selected_param % PARAM_COUNT;
        state_changed = true;
    }

    // Read encoder for a change in direction and update the selected parameter.
    // rotate() returns 0 for unchanged, 1 for increment, 2 for decrement.
    UpdateParameter(encoder.rotate());

    // Render any new UI changes to the OLED display.
    UpdateDisplay();
}

// Reset the seed and pattern length to restart the psudo random deterministic pattern.
void Reset() {
    Reseed();
    step_count = 0;
    state_changed = true;
}

// Reseed the random number generator with the current seed.
void Reseed() {
    randomSeed(seed);
}

// Update the current selected parameter with the current movement of the encoder.
void UpdateParameter(byte encoder_dir) {
    if (selected_param == 0) UpdateSeed(encoder_dir);
    if (selected_param == 1) UpdateLength(encoder_dir);
}

// Right now just randomize up or down from current seed.
void UpdateSeed(byte dir) {
    if (dir == 0) return;
    seed = random(0, UINT16_MAX);
    Reset();
}

// Adjust the step length.
void UpdateLength(byte dir) {
    if (dir == 0) return;
    if (dir == 1 && step_length < MAX_LENGTH - 1) step_length++;
    if (dir == 2 && step_length > MIN_LENGTH) step_length--;
    state_changed = true;
}

// Display the current state of the module params.
void UpdateDisplay() {
    if (!state_changed) return;
    state_changed = false;
    display.clearDisplay();

    // Display config param label and value.
    display.setFont((selected_param == 0) ? &FreeMonoBold9pt7b : &FreeMono9pt7b);
    display.setCursor(4, 18);
    display.println("Seed: " + String(seed, HEX));

    display.setFont((selected_param == 1) ? &FreeMonoBold9pt7b : &FreeMono9pt7b);
    display.setCursor(4, 36);
    display.println("Length: " + String(step_length));

    display.setFont(&FreeMono9pt7b);
    display.setCursor(4, 54);
    display.println("Step:" + String(step_count + 1));

    display.display();
}