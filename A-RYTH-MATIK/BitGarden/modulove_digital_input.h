#pragma once

namespace modulove {

class DigitalInput {
   public:
    // Enum constants for clk input rising/falling state.
    enum InputState {
        STATE_UNCHANGED,
        STATE_RISING,
        STATE_FALLING,
    };

    DigitalInput() {}
    ~DigitalInput() {}

    /**
        Initializes a CV Input object.
          \param cv_pin gpio pin for the cv output.
        */
    void Init(uint8_t cv_pin) {
        pinMode(cv_pin, INPUT);
        cv_pin_ = cv_pin;
    }

    void Process() {
        old_read_ = read_;
        read_ = digitalRead(cv_pin_);

        // Determine current clock input state.
        state_ = STATE_UNCHANGED;
        if (old_read_ == 0 && read_ == 1) {
            state_ = STATE_RISING;
            on_ = true;
        } else if (old_read_ == 1 && read_ == 0) {
            state_ = STATE_FALLING;
            on_ = false;
        }
    }

    inline InputState State() { return state_; }

    inline bool On() { return on_; }

   private:
    uint8_t cv_pin_;
    uint8_t read_;
    uint8_t old_read_;
    InputState state_;
    bool on_;
};

}  // namespace modulove