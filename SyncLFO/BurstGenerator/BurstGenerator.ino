/**
 * @file BurstGenerator.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief 
 * @version 0.1
 * @date 2024-04-08
 *
 * @copyright Copyright (c) 2024
 * 
 * Burst generator with rate, duration and slope settings.
 * 
 * Repeat the incoming trigger for a given rate and duration.
 * 
 * Heavily inspired by Luther's https://github.com/PierreIsCoding/sdiy/tree/main/Spark
 *
 */

// ALTERNATE HARDWARE CONFIGURATION
// #define SYNCHRONIZER

// GPIO Pin mapping.
#define P1 A0  // Burst Rate
#define P2 A1  // Burst Duration
#define P3 A3  // Slope (fade in, flat, fade out)
#define P4 A5  // Envelope speed

#define DIGITAL_IN 3   // Trigger Input to advance step
#define CV_OUT 10      // CV Output for current step

#ifdef SYNCHRONIZER
    #define B1 A2  // Trigger burst
    #define B2 12  // Retrig / Loop
    #define LED1 11  // Slider LED 1
    #define LED2 10  // Slider LED 2
    byte button1_state;
    byte button2_state;
    unsigned long _last_press;
    const int DEBOUNCE_MS = 10;
#endif

// The max burst gate duration.
const int MAX_BURST_DURATION = 2000;

// The duration of each burst spark in milliseconds. 
const int MAX_BURST_RATE = 1024;

// Script state.
uint8_t old_din = 0;  // Digital input read value.

enum SlopeMode {
    FADE_IN,
    FLAT,
    FADE_OUT,
};
SlopeMode slope_mode = FLAT;

void setup() {
    pinMode(DIGITAL_IN, INPUT);
    pinMode(CV_OUT, OUTPUT);
    pinMode(P1, INPUT);
    pinMode(P2, INPUT);
    pinMode(P3, INPUT);
    pinMode(P4, INPUT);

#ifdef SYNCHRONIZER
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
    // Detect if a new trigger has been received. If so, read inputs for burst settings.
    if (beginBurstCheck()) {
        // Kick off burst generator with the current knob parameters.
        int burst_rate = map(analogRead(P1), 0, 1023, 0, MAX_BURST_RATE);
        int burst_duration = map(analogRead(P2), 0, 1023, 20, MAX_BURST_DURATION);
        int slope_mode = readSlopeMode(P3);
        doBurst(burst_rate, burst_duration, slope_mode);
    }
}

bool beginBurstCheck() {
    bool begin_burst;
    
    // Read all inputs.
    int new_din = digitalRead(DIGITAL_IN);
    begin_burst = new_din == 1 && old_din == 0;
    old_din = new_din;

#ifdef SYNCHRONIZER
    // Echo digital input on LED 1.
    digitalWrite(LED1, old_din);

    // Read current button state.
    bool new_button1_state = digitalRead(B1);

    // State check with debounce.
    bool _debounced = (millis() - _last_press) > DEBOUNCE_MS;
    bool _pressed = button1_state == 1 && new_button1_state == 0 && _debounced;
    bool _released = button1_state == 0 && new_button1_state == 1 && _debounced;

    // If the button has been pressed, set trigger_start to True if it isn't already.
    begin_burst |= _pressed;

    // Update variables for next loop
    button1_state = new_button1_state;
    _last_press = (_pressed || _released) ? millis(): _last_press;
#endif

    return begin_burst;
}

void doBurst(int burst_rate, int burst_duration, int slope_mode) {
    byte output = 0;
    int counter = 0;
	float progress = 0;
    unsigned long burst_start = millis();

    while (millis() < burst_start + burst_duration) {

        // Increment counter for current phase of duty cycle.
        if (millis() > burst_start + (counter * burst_rate)) {
            counter++;
			// Calculate the progress percentage from current time to burst_duration.
			progress = float(millis() - burst_start) / float(burst_duration);
        }

        switch (slope_mode) {
        case FLAT:
            output = (counter % 2) ? 255 : 0;
            break;

        case FADE_IN:
            // Calculate current step voltage.
            output = (counter % 2) ? progress * 255.f : 0;
            break;
        
        case FADE_OUT:
            // Calculate current step voltage.
            output = (counter % 2) ? (255.f - (progress * 255.f)) : 0;
            break;
        
        default:
            break;
        }

        analogWrite(CV_OUT, output);
    }

    analogWrite(CV_OUT, 0);
}

SlopeMode readSlopeMode(uint8_t pin) {
    if (analogRead(pin) <= 341) {
        return FADE_IN;
    } else if (analogRead(pin) <= 682) {
        return FLAT;
    } else {
        return FADE_OUT;
    }
}
