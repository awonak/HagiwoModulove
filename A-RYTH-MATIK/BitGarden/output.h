#ifndef OUTPUT_H
#define OUTPUT_H

// Include the Modulove hardware library.
#include <arythmatik.h>

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


/**
  Class for handling probablistic triggers.
*/
class ProbablisticOutput {
   public:
    ProbablisticOutput() {}
    ~ProbablisticOutput() {}

    const static int MaxRandRange = 100;

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
        if (mode_ == GATE) low();

        if (random(0, MaxRandRange) > prob_) return;

        switch (mode_) {
            case TRIGGER:
            case GATE:
                high();
                break;
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
    inline float GetProb() { return fprob_; }
    inline int GetProbInt() { return prob_; }
    inline void IncProb() { SetProb(float(constrain(++prob_, 0, MaxRandRange)) / float(MaxRandRange)); }
    inline void DecProb() { SetProb(float(constrain(--prob_, 0, MaxRandRange)) / float(MaxRandRange)); }
    inline void SetProb(float probability) {
        fprob_ = probability; 
        prob_ = constrain(int(float(MaxRandRange) * probability), 0, MaxRandRange); 
    }

   private:
    DigitalOutput output_;
    int prob_;
    float fprob_;
    Mode mode_;

    inline void high() { output_.High(); }

    inline void low() { output_.Low(); }
};

#endif
