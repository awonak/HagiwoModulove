/**
 * @file TriggerDelay.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief 
 * @version 0.1
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2023
 *
 */

// ALTERNATE HARDWARE CONFIGURATION
#define JMNW

// GPIO Pin mapping.
#define P1 A0  // Trigger Delay
#define P2 A1  // Gate Duration
#define P3 A3  // Attack Slope
#define P4 A5  // Release Slope

#define DIGITAL_IN 3   // Trigger Input to advance step
#define CV_OUT 10      // CV Output for current step

#ifdef JMNW
#define B1 A2  // Button 1 (bodged from original D13)
#define B2 12  // Button 2
#define LED1 11  // Slider LED 1
#define LED2 10  // Slider LED 2
byte button1_state;
byte old_button1_state;
byte button2_state;
#endif

// Global state variables.
uint8_t din = 0;  // Digital input read value.
uint8_t old_din = 0;
float R;  // R holds the linear to exponential conversion factor value.


enum Stage {
    DELAY,
    ATTACK,
    GATE,
    RELEASE,
    WAIT,
};
Stage stage = WAIT;

// Script state.
unsigned long trig_start;
int trig_delay;
int trig_duration;
int attack_slope;
int release_slope;
int attack_count;
int release_count;
int counter;
int output;

void setup() {
    pinMode(DIGITAL_IN, INPUT);
    pinMode(CV_OUT, OUTPUT);
    pinMode(P1, INPUT);
    pinMode(P2, INPUT);
    pinMode(P3, INPUT);
    pinMode(P4, INPUT);

#ifdef JMNW
    pinMode(B1, INPUT_PULLUP);
    pinMode(B2, INPUT_PULLUP);
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
#endif

    // Register setting for high frequency PWM.
    TCCR1A = 0b00100001;
    TCCR1B = 0b00100001;

    // Calculate the linear to exponential R value (only needs to be done once at setup).
    R = (1023 * log10(2)) / (log10(255));
}

void loop() {
    bool trigger_start;
    
    // Read all inputs.
    old_din = din;
    din = digitalRead(DIGITAL_IN);
    trigger_start = old_din == 0 && din == 1;

#ifdef JMNW
    old_button1_state = button1_state;
    button1_state = digitalRead(B1);
    // Check for button 1 press.
    // TODO: Handle debounce
    trigger_start |= old_button1_state == 0 && button1_state == 1;;
#endif

    // Detect if a new trigger has been received.
    if (trigger_start) {
        // Read the trigger delay and duration.
        trig_delay = map(analogRead(P1), 0, 1023, 0, 2000);
        trig_duration = map(analogRead(P2), 0, 1023, 20, 2000);
        trig_start = millis();

        // Calculate the exponential slope value.
        attack_count = analogRead(P3);
        release_count = analogRead(P4);
        attack_slope = (attack_count * log10(2)) / log10(255);
        release_slope = (release_count * log10(2)) / log10(255);

        stage = DELAY;
        output = 0;
        counter = 1;
    }

    switch (stage) {
        case DELAY:
            if (millis() > trig_start + trig_delay) {
                stage = ATTACK;
            }
            break;
        case ATTACK:
            if (counter >= attack_count) {
                stage = GATE;
                trig_start = millis();
                counter = release_count;
            } else {
                output = min(pow(2, (float(counter) / float(attack_slope))), 255);
                counter = min(counter++, attack_count);
            }
            break;
        case GATE:
            output = 255;
            if (millis() > trig_start + trig_duration) {
                stage = RELEASE;
            }
            break;
        case RELEASE:
            if (counter == 0) {
                stage = WAIT;
                output = 0;
            } else {
                output = min(pow(2, float(counter) / float(release_slope)), 255);
                if (counter > 0) counter--;
            }
            break;
        case WAIT:
            break;
    }

    // Set output voltage.
    analogWrite(CV_OUT, output);
}

// Convert a linear value to its exponential compliment for led visual correction.
int brightness(int v) { return (v > 32) ? pow(2, (v / R)) : 0; }
