#pragma once

enum Mode {
    TRIGGER,
    FLIP,
};

const int MaxRandRange = 100;

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
        Init(cv_pin, led_pin, probability, TRIGGER);
    }

    /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param cv_pin gpio pin for the cv output.
      \param led_pin gpio pin for the LED.
      \param probability percentage chance to trigger.
      \param mode percentage chance to trigger.
    */
    void Init(uint8_t cv_pin, uint8_t led_pin, float probability, Mode mode) {
        pinMode(cv_pin, OUTPUT);   // Gate/Trigger Output
        pinMode(led_pin, OUTPUT);  // LED
        cv_pin_ = cv_pin;
        led_pin_ = led_pin;
        SetProb(probability);
        SetMode(mode);
    }

    // Turn the CV and LED High according to the probability value.
    inline void On() {
        if (random(0, MaxRandRange) > prob_) return;

        switch (mode_) {
            case TRIGGER:
                high();
                break;
            case FLIP:
                update(!state_);
                break;
        }
    }

    // Turn off CV and LED.
    inline void Off() {
        switch (mode_) {
            case TRIGGER:
                low();
                break;
        }
    }

    inline bool State() { return state_; }
    inline bool GetMode() { return mode_; }
    inline void SetMode(Mode mode) { mode_ = mode; }
    inline String DisplayMode() { return (mode_ == 0) ? "Trig" : "Flip"; }
    inline float GetProb() { return float(prob_) / float(MaxRandRange); }
    inline void IncProb() { prob_ = constrain(++prob_, 0, MaxRandRange); }
    inline void DecProb() { prob_ = constrain(--prob_, 0, MaxRandRange); }
    inline void SetProb(float probability) { prob_ = constrain(int(float(MaxRandRange) * probability), 0, MaxRandRange); }

   private:
    uint8_t cv_pin_;
    uint8_t led_pin_;
    bool state_;
    int prob_;
    Mode mode_;

    inline void high() { update(HIGH); }

    inline void low() { update(LOW); }

    void update(uint8_t state) {
        digitalWrite(cv_pin_, state);
        digitalWrite(led_pin_, state);
        state_ = state;
    }
};
