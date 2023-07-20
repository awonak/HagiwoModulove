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
#include <Encoder.h>
#include <Pushbutton.h>

// Script specific output class.
#include "output.h"

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OUTPUT_COUNT 6

// Input pins
#define CLOCK_IN 13  // Trigger Input to advance the rhythm counter
#define RESET_IN 11  // Trigger Input to reset the rhythm counter

// Output pins
#define CLOCK_LED 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Encoder encoder(3, 2);
Pushbutton button(12);
ProbablisticOutput outputs[6];

// Flag for enabling debug print to serial monitoring output.
// NOTE: debug will enable the serial port which locks LED 4 & 5 on HIGH.
const bool DEBUG = false;

// Script state variables.
bool trig = 0;  // External trigger input detect
bool old_trig = 0;

void setup() {
    // Initialize each of the outputs with it's GPIO pins and probability.
    outputs[0].Init(5, 14, 0.96);
    outputs[1].Init(6, 15, 0.82);
    outputs[2].Init(7, 16, 0.64);
    outputs[3].Init(8, 0, 0.48);
    outputs[4].Init(9, 1, 0.32);
    outputs[5].Init(10, 17, 0.18);

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

    if (DEBUG) Serial.begin(9600);
}

void loop() {
    old_trig = trig;
    trig = digitalRead(CLOCK_IN);

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
}