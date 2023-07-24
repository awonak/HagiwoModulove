/**
 * @file Uncertainty.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Stochastic gate processor based on the Olivia Artz Modular's Uncertainty.
 * @version 0.1
 * @date 2023-07-19
 *
 * @copyright Copyright (c) 2023
 *
 * Connect a trigger or gate source to the CLK input and the each output will
 * mirror that signal according to a decreasing probability.
 */

// Oled setting
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
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
ProbablisticOutput outputs[6];

// Script config definitions
const uint8_t OUTPUT_COUNT = 6;  // Count of outputs.
const uint8_t PARAM_COUNT = 3;  // Count of editable parameters.

// Script state variables.
bool trig = 0;  // External trigger input detect
bool old_trig = 0;

byte selected_out = 0;
byte selected_param = 0;

// State variables for tracking OLED refresh rate.
const int refresh_ms = 100;
uint32_t last_ui_update = 0;
bool state_changed = true;

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
    Serial.begin(9600);
#endif

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
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.setTextSize(1);
    display.setFont(&FreeMono9pt7b);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.display();
}

void loop() {
    old_trig = trig;
    trig = digitalRead(CLK_PIN);

    // Clock In LED indicator mirrors the clock input.
    digitalWrite(CLOCK_LED, trig);

    // Input clock has gone high, call each output's On() for a chance to
    // trigger that output.
    if (old_trig == 0 && trig == 1) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].On();
        }
    }

    // Input clock has gone low, turn off Outputs.
    if (old_trig == 1 && trig == 0) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].Off();
        }
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

void UpdateParameter(byte encoder_dir) {
    if (selected_param == 0) UpdateOutput(encoder_dir);
    if (selected_param == 1) UpdateMode(encoder_dir);
    if (selected_param == 2) UpdateProb(encoder_dir);
}

void UpdateOutput(byte encoder_dir) {
    if (encoder_dir == 0) return;
    if (encoder_dir == 1 && selected_out < OUTPUT_COUNT - 1) selected_out++;
    if (encoder_dir == 2 && selected_out > 0) selected_out--;
    state_changed = true;
}

void UpdateMode(byte encoder_dir) {
    if (encoder_dir == 0) return;
    if (encoder_dir == 1) outputs[selected_out].SetMode(Mode::FLIP);
    if (encoder_dir == 2) outputs[selected_out].SetMode(Mode::TRIGGER);
    state_changed = true;
}

void UpdateProb(byte encoder_dir) {
    if (encoder_dir == 0) return;
    if (encoder_dir == 1) outputs[selected_out].IncProb();
    if (encoder_dir == 2) outputs[selected_out].DecProb();
    state_changed = true;
}

void UpdateDisplay() {
    if (!state_changed) return;
    state_changed = false;
    display.clearDisplay();

    // Indicator for which config parameter is selected.
    int x1 = 22, y1 = 13;  // Set default indicator point origin.
    if (selected_param == 0) y1 = 13;
    if (selected_param == 1) y1 = 31;
    if (selected_param == 2) y1 = 49;
    display.fillTriangle(x1, y1, x1 - 3, y1 - 3, x1 - 3, y1 + 3, WHITE);

    // Display the 6 output channel boxes.
    int boxSize = 8;
    for (int i = 1; i < OUTPUT_COUNT + 1; i++) {
        display.drawRect(boxSize, i * boxSize, boxSize - 1, boxSize - 1, WHITE);
    }
    display.fillRect(boxSize, (selected_out * boxSize) + boxSize, boxSize - 1, boxSize - 1, WHITE);

    // Display config param label and value.
    display.setCursor(25, 18);
    display.println("Output " + String(selected_out + 1));
    display.setCursor(80, 18);

    display.setCursor(25, 36);
    display.println("Mode:");
    display.setCursor(80, 36);
    display.println(outputs[selected_out].DisplayMode());

    display.setCursor(25, 54);
    display.println("Prob:");
    display.setCursor(80, 54);
    display.println(outputs[selected_out].GetProb(), 2);

    display.display();
}