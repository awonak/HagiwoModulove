#pragma once

namespace modulove {

class DigitalOutput {
   public:
    /**
    Initializes an CV Output paired object.
        \param cv_pin gpio pin for the cv output.
    */
    void Init(uint8_t cv_pin) {
        pinMode(cv_pin, OUTPUT);  // Gate/Trigger Output
        cv_pin_ = cv_pin;
    }
    /**
    Initializes an LED & CV Output paired object.
        \param cv_pin gpio pin for the cv output.
        \param led_pin gpio pin for the LED.
    */
    void Init(uint8_t cv_pin, uint8_t led_pin) {
        pinMode(led_pin, OUTPUT);  // LED
        led_pin_ = led_pin;
        #define LED_PIN_DEFINED
        Init(cv_pin);
    }

    // Turn the CV and LED High according to the input state.
    inline void Update(uint8_t state) {
        if (state == HIGH) High();  // Rising
        if (state == LOW) Low();    // Falling
    }

    inline void High() { update(HIGH); }

    inline void Low() { update(LOW); }

    // Return the bool representing the on/off state of the output.
    inline bool On() { return on_; }

   private:
    uint8_t cv_pin_;
    uint8_t led_pin_;
    bool on_;

    void update(uint8_t state) {
        digitalWrite(cv_pin_, state);
        #ifdef LED_PIN_DEFINED
        digitalWrite(led_pin_, state);
        #endif
        on_ = state == HIGH;
    }
};

}  // namespace modulove