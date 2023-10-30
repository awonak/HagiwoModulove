#include "src/libmodulove/arythmatik.h"

class Stutter {
   public:
    Stutter() {}
    ~Stutter() {}


    void Init(modulove::DigitalOutput output, int factor, int repeats) {
        output_ = output;
        factor_ = factor;
        repeats_ = repeats;
    }

    // Process clock to determine period duration.
    void ProcessCLK(long now) {
        period_ = now - lastInput_;
        lastInput_ = now;
        duty_ = period_ / factor_ / 2;
    }

    // Start the burst generator for this clock.
    void ProcessRST(long now) {
        output_.High();
        deadline_ = now + duty_;
        counter_ = repeats_;
    }

    // Check for change to clock state with each cpu cycle.
    void Tick(long now) {
        if (repeats_ > 0 && now >= deadline_) {
            repeats_--;
            deadline_ = now + duty_;
            output_.On() ? output_.Low() : output_.High();
        }
    }

   private:
    modulove::DigitalOutput output_;
    int factor_;
    int repeats_;

    // Initial default state.
    int period_;
    int duty_;
    int counter_;
    long deadline_;
    long lastInput_ = millis();

    // Determine the duty cycle for the clock modification.
    void recalculateDuty(long now) {
    }

};