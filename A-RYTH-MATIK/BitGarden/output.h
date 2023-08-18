#pragma once

const int MaxRandRange = 100;

/**
  Class for handling probablistic triggers.
*/
class FixedProbablisticOutput {
   public:
    FixedProbablisticOutput() {}
    ~FixedProbablisticOutput() {}

    /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param cv_pin gpio pin for the cv output.
      \param led_pin gpio pin for the LED.
      \param probability percentage chance to trigger as a float from 0 to 1.
    */
    void Init(uint8_t cv_pin, uint8_t led_pin, float probability) {
        pinMode(cv_pin, OUTPUT);   // Gate/Trigger Output
        pinMode(led_pin, OUTPUT);  // LED
        cv_pin_ = cv_pin;
        led_pin_ = led_pin;
        setProb(probability);
    }

    // Turn the CV and LED High according to the probability value.
    inline void Update(uint8_t input_state) {
        if (input_state == 0) return;  // Unchanged
        if (input_state == 1) high();  // Rising
        if (input_state == 2) low();   // Falling
    }

    // Return the bool representing the on/off state of the output.
    inline bool State() { return state_; }

    // Return the float trigger probability of the output.
    inline float GetProb() {
        return float(prob_) / float(MaxRandRange);
    }

   private:
    uint8_t cv_pin_;
    uint8_t led_pin_;
    bool state_;
    int prob_;

    inline void setProb(float probability) {
        prob_ = constrain(int(float(MaxRandRange) * probability), 0, MaxRandRange);
    }

    inline void high() {
        if (random(0, MaxRandRange) < prob_) {
            update(HIGH);
        }
    }

    inline void low() { update(LOW); }

    void update(uint8_t state) {
        digitalWrite(cv_pin_, state);
        digitalWrite(led_pin_, state);
        state_ = state;
    }
};
