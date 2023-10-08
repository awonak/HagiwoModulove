/**
 * @file Polyrhythm.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Generate polyrhythms based on 16 step counter knobs for HAGIWO Sync Mod LFO (demo: TODO)
 * @version 0.2
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <Arduino.h>
#include <avr/io.h>

// GPIO Pin mapping.
#define P1 0  // Octave Range
#define P2 1  // Speed (TODO: maybe clock div?)
#define P3 3  // Scale (Maj, Min, Pent, etc...)
#define P4 5  // Mode (forward, backward, bi-directional, random)

#define TRIG_IN 3  // Trigger Input to advance the rhythm counter
#define CV_OUT 10  // CV Output for polyrhythm triggers

// Readable name for register
#define PWMValue OCR1B

// const int PWMMax = 4095;  // 12 bit over 10v.
const int PWMMax = 1024; // 10 bit over 10v.
// const int PWMMax = 255; // 8 bit over 10v.

// Flag for enabling debug print to serial monitoring output.
const bool DEBUG = false;

// Rest between steps in milliseconds.
const int REST = 1000;

// Script state variables.
bool clk = 0;  // External trigger input detect
bool old_clk = 0;
unsigned long counter;
unsigned long now;

float octave = PWMMax / 10;  // Output range is 10v.
float semitone = octave / 12;
float value;

void setup() {
    // Serial.begin(115200);
    pinMode(TRIG_IN, INPUT);  // Trigger in
    pinMode(CV_OUT, OUTPUT);  // Polyrhythm trigger cv output

    // Use Fast PWM and 12 bit PWM resolution for higher accuracy.
    // TCCR1A = (1 << COM1B1) | (1 << WGM11) | (1 << WGM10);               // Enable Fast PWM on OCR1A (Pin 10)
    TCCR1A = (1 << COM1B1) | (1 << WGM11);               // Enable Fast PWM on OCR1A (Pin 10)
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  // Mode 14 Fast PWM/ (TOP = ICR1), pre-scale = 1
    // TCCR1A = _BV(COM1B1) | _BV(WGM11) | _BV(WGM10);
    // TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);       // prescaler /8 gives period of ~2.048mS
    OCR1A = 0x0fff;  // period
    OCR1B = 0x0;     //~0% duty

    ICR1 = PWMMax;  // Set the TOP value for 12-bit PWM
    PWMValue = 0;   // Set the PWM output to full off.

    now = millis();

    delay(100);
    // Serial.println("Start: ");
}

void loop() {
    old_clk = clk;
    clk = digitalRead(TRIG_IN);

    // Detect if new trigger received, advance counter and check for hit on the beat.
    if (old_clk == 0 && clk == 1) {
        // Advance the beat counter and get the hits on this beat.
        counter++;
    }

    // Detect if trigger has just ended and turn off the trigger cv output.
    if (old_clk == 1 && clk == 0) {
        // analogWrite(CV_OUT, 0);
    }

    if (millis() - now > REST) {
        counter++;
        nextStep();
        now = millis();
    }
}

void nextStep() {
    value += (semitone * 5);
    // Cut the PWM Max in half for 5v max output.
    if (value > PWMMax / 2) {
        value = 0;
    }
    // analogWrite(CV_OUT, value);
    // Serial.println("PWM: " + String(int(value)));

    // TCCR1A |= _BV(COM1B1);
    OCR1B = value;
}
