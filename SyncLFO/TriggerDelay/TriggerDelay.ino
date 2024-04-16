/**
 * @file TriggerDelay.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief 
 * @version 0.1
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024
 * 
 * Trigger delay with gate and slope settings.
 * 
 * Repeat the incoming trigger with a delay up to 2 seconds set by P1 for a
 * duration up to 2 seconds set by P2. Additionally, the output rising edge
 * can be slewed using P3 and the falling edge can be slewed using P4.
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
    byte button2_state;
    unsigned long _last_press;
    const int DEBOUNCE_MS = 10;
#endif

// The max trigger delay / gate duration.
const int MAX_DELAY_MS = 2000;


enum Stage {
    DELAY,
    ATTACK,
    GATE,
    RELEASE,
    WAIT,
};
Stage stage = WAIT;

// Script state.
uint8_t din = 0;  // Digital input read value.
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
}

void loop() {
    bool trigger_start;
    int new_din;
    
    // Read all inputs.
    new_din = digitalRead(DIGITAL_IN);
    trigger_start = new_din == 1 && din == 0;
    din = new_din;

#ifdef JMNW
    // Echo digital input on LED 1.
    digitalWrite(LED1, din);

    // Read current button state.
    bool new_button1_state = digitalRead(B1);

    // State check with debounce.
    bool _debounced = (millis() - _last_press) > DEBOUNCE_MS;
    bool _pressed = button1_state == 1 && new_button1_state == 0 && _debounced;
    bool _released = button1_state == 0 && new_button1_state == 1 && _debounced;

    // If the button has been pressed, set trigger_start to True if it isn't already.
    trigger_start |= _pressed;

    // Update variables for next loop
    button1_state = new_button1_state;
    _last_press = (_pressed || _released) ? millis(): _last_press;
#endif

    // Detect if a new trigger has been received.
    if (trigger_start) {
        // Read the trigger delay and duration.
        trig_delay = map(analogRead(P1), 0, 1023, 0, MAX_DELAY_MS);
        trig_duration = map(analogRead(P2), 0, 1023, 20, MAX_DELAY_MS);
        trig_start = millis();

        // Calculate the exponential slope value.
        attack_count = analogRead(P3);
        release_count = analogRead(P4);
        attack_slope = slope(attack_count);
        release_slope = slope(release_count);

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

// Calculate the linear to exponential slope value.
int slope (int input) {
    return (input * log10(2)) / (log10(255));
}