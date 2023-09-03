#pragma once

using namespace modulove;

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
      \param output Instance of the cv output.
      \param probability percentage chance to trigger as a float from 0 to 1.
    */
    void Init(DigitalOutput output, float probability) {
        output_ = output;
        setProb(probability);
    }

    // Update CV and LED state according to the input state and configured probability value.
    inline void Update(DigitalInput::InputState input_state) {
        if (input_state == DigitalInput::STATE_UNCHANGED) return;
        if (input_state == DigitalInput::STATE_RISING) high();
        if (input_state == DigitalInput::STATE_FALLING) low();
    }

    // Return the bool representing the on/off state of the output.
    inline bool State() { return output_.On(); }

    // Return the float trigger probability of the output.
    inline float GetProb() {
        return float(prob_) / float(MaxRandRange);
    }

   private:
    DigitalOutput output_;
    int prob_;

    inline void setProb(float probability) {
        prob_ = constrain(int(float(MaxRandRange) * probability), 0, MaxRandRange);
    }

    inline void high() {
        if (random(0, MaxRandRange) < prob_) {
            output_.High();
        }
    }

    inline void low() { output_.Low(); }
};
