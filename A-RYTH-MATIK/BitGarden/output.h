#ifndef OUTPUT_H
#define OUTPUT_H

// Include the Modulove hardware library.
#include "src/libmodulove/arythmatik.h"

using namespace modulove;

enum Mode {
    // Follow the state of the input clock.
    TRIGGER,
    // 100% Duty cycle gate.
    GATE,
    // Toggle between on/off with each clock input rising edge.
    FLIP,

    MODE_LAST,
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
      \param output Instance of the cv output.
      \param probability percentage chance to trigger as a float from 0 to 1.
    */
    void Init(DigitalOutput output, float probability) {
        Init(output, probability, TRIGGER);
    }

    /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param output Instance of the cv output.
      \param probability percentage chance to trigger as a float from 0 to 1.
      \param mode defines the behavior of the rising / falling clock input on this output.
    */
    void Init(DigitalOutput output, float probability, Mode mode) {
        output_ = output;
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
            case GATE:
                low();
                high();
            case FLIP:
                output_.Update(!output_.On());
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

    inline bool State() { return output_.On(); }
    inline Mode GetMode() { return mode_; }
    inline void SetMode(Mode mode) { mode_ = mode; }
    inline String DisplayMode() { return (mode_ == 0) ? "TRIG" : "FLIP"; }
    inline float GetProb() { return float(prob_) / float(MaxRandRange); }
    inline void IncProb() { prob_ = constrain(++prob_, 0, MaxRandRange); }
    inline void DecProb() { prob_ = constrain(--prob_, 0, MaxRandRange); }
    inline void SetProb(float probability) { prob_ = constrain(int(float(MaxRandRange) * probability), 0, MaxRandRange); }

   private:
    DigitalOutput output_;
    int prob_;
    Mode mode_;

    inline void high() { output_.High(); }

    inline void low() { output_.Low(); }
};

#endif