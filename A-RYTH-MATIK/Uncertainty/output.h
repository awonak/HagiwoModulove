#pragma once

const int MaxRandRange = 1023;

/**
  Class for handling probablistic triggers.
*/
class ProbablisticOutput {
   public:
    ProbablisticOutput() {}
    ~ProbablisticOutput() {}

    /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param cv_pin gpio pin for the cv output.
      \param led_pin gpio pin for the LED.
      \param probability percentage chance to trigger.
    */
    void Init(uint8_t cv_pin, uint8_t led_pin, float probability) {
        pinMode(cv_pin, OUTPUT);   // Gate/Trigger Output
        pinMode(led_pin, OUTPUT);  // LED
        cv_pin_ = cv_pin;
        led_pin_ = led_pin;
        prob_ = int(float(MaxRandRange) * probability);
    }

    // Turn the CV and LED High according to the probability value.
    inline void On() {
        if (random(0, MaxRandRange) < prob_) high();
    }

    // Turn off CV and LED.
    inline void Off() { low(); }

    // Return the current state of the output as a bool.
    inline bool State() { return state_; }

   private:
    uint8_t cv_pin_;
    uint8_t led_pin_;
    bool state_;
    int prob_;

    inline void high() { update(HIGH); }

    inline void low() { update(LOW); }

    void update(uint8_t state) {
        digitalWrite(cv_pin_, state);
        digitalWrite(led_pin_, state);
        state_ = state;
    }
};
