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
 * Heavily inspired by Luther's
 * https://github.com/PierreIsCoding/sdiy/tree/main/Spark
 *
 */

// ALTERNATE HARDWARE CONFIGURATION
// #define SYNCHRONIZER

// GPIO Pin mapping.
#define P1 A0  // Burst Rate
#define P2 A1  // Burst Count
#define P3 A3  // Full Burst Slope (fade in, flat, fade out)
#define P4 A5  // Individual Burst Slope (fade in, flat, fade out)

#define DIGITAL_IN 3  // Trigger Input to advance step
#define CV_OUT 10     // CV Output for current step

#ifdef SYNCHRONIZER
#define B1 A2    // Trigger burst
#define B2 12    // Retrig / Loop
#define LED1 11  // Slider LED 1
#define LED2 10  // Slider LED 2
byte button1_state;
byte button2_state;
unsigned long _last_press;
const int DEBOUNCE_MS = 10;
#endif

// The max burst gate count.
const int MAX_BURST_COUNT = 16;

// The duration of each burst spark in milliseconds.
const int MIN_BURST_RATE = 10;
const int MAX_BURST_RATE = 512;

// Script state.
int counter = 0;
byte individual_cv = 0;
byte full_cv = 0;
float individual_progress = 0;
float full_progress = 0;
unsigned long burst_start = 0;
unsigned long last_burst = 0;
int burst_rate;
int burst_count;

enum SlopeMode {
    FADE_IN,
    FLAT,
    FADE_OUT,
};
SlopeMode full_slope_mode = FLAT;
SlopeMode individual_slope_mode = FLAT;

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
    // Detect if a new trigger has been received. If so, read inputs for burst
    // settings.
    if (beginBurst()) {
        // Initialize burst state variables.
        burst_start = millis();
        counter = 0;

        // Read the current knob parameters.
        burst_rate =
            map(analogRead(P1), 0, 1024, MIN_BURST_RATE, MAX_BURST_RATE);
        burst_count = map(analogRead(P2), 0, 1023, 1, MAX_BURST_COUNT);
        full_slope_mode = readSlopeMode(P3);
        individual_slope_mode = readSlopeMode(P4);
    }

    // Update burst cv if withing a bursting state.
    if (millis() < burst_start + ((2 * burst_rate) * burst_count)) {
        // Increment counter for current phase of duty cycle.
        if (millis() > burst_start + (counter * burst_rate)) {
            counter++;
            if (counter % 2) {
                last_burst = millis();
                // Calculate the progress percentage from current time to
                // burst_duration.
                full_progress = float(millis() - burst_start) /
                                float(((2 * burst_rate) * burst_count));
            }
        }

        // Calculate progress of current burst slope.
        if (millis() < (last_burst + (2 * burst_rate))) {
            individual_progress =
                float(millis() - last_burst) / float(2 * burst_rate);
        }
        switch (individual_slope_mode) {
            case FLAT:
                individual_cv = 255;
                break;
            case FADE_IN:
                individual_cv = individual_progress * 255.f;
                break;
            case FADE_OUT:
                individual_cv = (255.f - (individual_progress * 255.f));
                break;
        }

        // Calculate overall burst progress.
        switch (full_slope_mode) {
            case FLAT:
                full_cv = individual_cv;
                break;
            case FADE_IN:
                full_cv = full_progress * individual_cv;
                break;
            case FADE_OUT:
                full_cv = (individual_cv - (full_progress * individual_cv));
                break;
        }
        // Duty cycle check for flat burst shape.
        if (individual_slope_mode == FLAT) {
            full_cv = (counter % 2) ? full_cv : 0;
        }

        analogWrite(CV_OUT, full_cv);
    }
    else {
        analogWrite(CV_OUT, 0);
    }
}

bool beginBurst() {
    static int old_din;
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

    // If the button has been pressed, set trigger_start to True if it isn't
    // already.
    begin_burst |= _pressed;

    // Update variables for next loop
    button1_state = new_button1_state;
    _last_press = (_pressed || _released) ? millis() : _last_press;
#endif

    return begin_burst;
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
