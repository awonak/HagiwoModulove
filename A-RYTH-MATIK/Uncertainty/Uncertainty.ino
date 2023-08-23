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

// Oled setting
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

// Enum constants for current display page.
enum MenuPage {
    PAGE_MAIN,
    PAGE_MODE,
};
MenuPage selected_page = PAGE_MAIN;

// Script config definitions
const uint8_t OUTPUT_COUNT = 6;  // Count of outputs.
const uint8_t PARAM_COUNT = 2;   // Count of editable parameters.

// Script state variables.
bool clk = 0;  // External trigger input detect
bool old_clk = 0;

byte selected_out = 0;
byte selected_param = 0;

bool update_display = true;

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
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.display();
}

void loop() {
    old_clk = clk;
    clk = digitalRead(CLK_PIN);

    // Clock In LED indicator mirrors the clock input.
    digitalWrite(CLOCK_LED, clk);

    // Input clock has gone high, call each output's On() for a chance to
    // trigger that output.
    if (old_clk == 0 && clk == 1) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].On();
        }
    }

    // Input clock has gone low, turn off Outputs.
    if (old_clk == 1 && clk == 0) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            outputs[i].Off();
        }
    }

    // Check for long press to endable editing seed.
    // press and release for < 1 second to return 1 for short press
    // press and release for > 1 second to return 2 for long press.
    byte press = encoder.pushType(1000);

    // Short button press. Change editable parameter.
    if (press == 1) {
        // Next param on Main page.
        if (selected_page == PAGE_MAIN)
            selected_param = ++selected_param % PARAM_COUNT;

        update_display = true;
    }

    // Long button press. Change menu page.
    if (press == 2) {
        if (selected_page == PAGE_MAIN) {
            selected_page = PAGE_MODE;
            selected_param = 2;
        } else {
            selected_page = PAGE_MAIN;
            selected_param = 0;
        }
        update_display = true;
    }

    // Read encoder for a change in direction and update the selected parameter.
    // rotate() returns 0 for unchanged, 1 for increment, 2 for decrement.
    UpdateParameter(encoder.rotate());

    // Render any new UI changes to the OLED display.
    UpdateDisplay();
}

void UpdateParameter(byte encoder_dir) {
    if (encoder_dir == 0) return;
    if (selected_param == 0) UpdateOutput(encoder_dir);
    if (selected_param == 1) UpdateProb(encoder_dir);
    if (selected_param == 2) UpdateMode(encoder_dir);
}

void UpdateOutput(byte encoder_dir) {
    if (encoder_dir == 1 && selected_out < OUTPUT_COUNT - 1) selected_out++;
    if (encoder_dir == 2 && selected_out > 0) selected_out--;
    update_display = true;
}

void UpdateMode(byte encoder_dir) {
    Mode newMode;
    if (encoder_dir == 1) newMode = Mode::FLIP;
    if (encoder_dir == 2) newMode = Mode::TRIGGER;
    // Update the mode for all outputs.
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        outputs[i].SetMode(newMode);
    }
    update_display = true;
}

void UpdateProb(byte encoder_dir) {
    if (encoder_dir == 1) outputs[selected_out].IncProb();
    if (encoder_dir == 2) outputs[selected_out].DecProb();
    update_display = true;
}

void UpdateDisplay() {
    if (!update_display) return;
    update_display = false;
    display.clearDisplay();

    // Draw app title.
    display.setTextSize(0);
    display.setCursor(36, 0);
    display.println("UNCERTAINTY");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

    switch (selected_page) {
        case PAGE_MAIN:
            DisplayMainPage();
            break;
        case PAGE_MODE:
            DisplayModePage();
            break;
    }

    display.display();
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
        display.drawRect(left, top, barWidth, barHeight, 1);

        // Fill current bar probability.
        byte probFill = (float(barHeight) * outputs[i].GetProb());
        byte probTop = top + barHeight - probFill;
        display.fillRect(left, probTop, barWidth, probFill, 1);

        // Show selected output.
        if (i == selected_out) {
            (selected_param == 0)
                ? display.drawChar(left + 2, SCREEN_HEIGHT - 6, 0x18, 1, 0, 1)
                : display.drawChar(left + 2, SCREEN_HEIGHT - 6, 0x25, 1, 0, 1);
        }

        left += barWidth + padding;
    }
}

void DisplayModePage() {
    // Show mode edit page.
    display.setTextSize(2);
    display.setCursor(6, 32);
    display.println("MODE: " + outputs[0].DisplayMode());
}
