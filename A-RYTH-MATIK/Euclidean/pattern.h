#ifndef PATTERN_H
#define PATTERN_H

// Include the Modulove library.
#include <arythmatik.h>

using namespace modulove;

#define MAX_PATTERN_LEN 32

struct PatternState {
    uint8_t steps;
    uint8_t hits;
    uint8_t offset;
    uint8_t padding;
};

class Pattern {
   public:
    Pattern() {}
    ~Pattern() {}

    /**
    Initializes the euclidean rhythm pattern with the given attributes.
      \param steps total length of the pattern.
      \param hits the number of "beats" to distribute evenly across the pattern steps.
      \param offset rotation of the pattern's hits.
      \param padding additional empty steps added to the pattern.
    */
    void Init(PatternState state) {
        steps_ = constrain(state.steps, 0, MAX_PATTERN_LEN);
        hits_ = constrain(state.hits, 1, steps_);
        padding_ = constrain(state.padding, 0, MAX_PATTERN_LEN - steps_);
        offset_ = constrain(state.offset, 0, (steps_ + padding_) - 1);
        updatePattern();
    }

    PatternState GetState() { return {steps_, hits_, offset_, padding_}; }

    // Get the current step value and advance the euclidean rhythm step index
    // to the next step in the pattern.
    // Returns 1 for hit, 0 for rest, and 2 for padding.
    int NextStep() {
        if (steps_ == 0) return 0;

        int value = GetStep(current_step_);
        current_step_ =
            (current_step_ < steps_ + padding_ - 1) ? current_step_ + 1 : 0;
        return value;
    }

    // Returns 1 for hit, 0 for rest, and 2 for padding.
    int GetStep(int i) { return pattern_[i]; }

    void ChangeSteps(int val) {
        if (val == 1 && steps_ < MAX_PATTERN_LEN) {
            steps_++;
            padding_ = min(padding_, MAX_PATTERN_LEN - steps_);
            updatePattern();
        } else if (val == -1 && steps_ > 1) {
            steps_--;
            hits_ = min(hits_, steps_);
            offset_ = min(offset_, (steps_ + padding_) - 1);
            updatePattern();
        } else if (val == -1 && steps_ == 1) {
            // Mute this pattern.
            steps_ = 0;
            offset_ = 0;
            padding_ = 0;
            updatePattern();
        }
    }

    void ChangeHits(int val) {
        hits_ = constrain(hits_ + val, 0, steps_);
        updatePattern();
    }

    void ChangeOffset(int val) {
        offset_ = constrain(offset_ + val, 0, (steps_ + padding_));
        updatePattern();
    }

    void ChangePadding(int val) {
        if (val == 1 && padding_ + steps_ < MAX_PATTERN_LEN) {
            padding_++;
            updatePattern();
        } else if (val == -1 && padding_ > 0) {
            padding_--;
            offset_ = min(offset_, (padding_ + steps_) - 1);
            updatePattern();
        }
    }

    void Reset() { current_step_ = 0; }

    inline uint8_t steps() { return steps_; }
    inline uint8_t hits() { return hits_; }
    inline uint8_t offset() { return offset_; }
    inline uint8_t padding() { return padding_; }
    inline uint8_t current_step() { return current_step_; }

   private:
    uint8_t steps_ = 0;
    uint8_t hits_ = 0;
    uint8_t offset_ = 0;
    uint8_t padding_ = 0;
    uint8_t current_step_ = 0;
    uint8_t pattern_[MAX_PATTERN_LEN];

    // Update the euclidean rhythm pattern when attributes change.
    void updatePattern() {
        // Fill current pattern with "padding" steps, then overwrite with hits
        // and rests.
        for (int i = 0; i < MAX_PATTERN_LEN; i++) {
            pattern_[i] = 2;
        }

        // Populate the euclidean rhythm pattern according to the current
        // instance variables.
        int bucket = 0;
        pattern_[offset_] = (hits_ > 0) ? 1 : 0;
        for (int i = 1; i < steps_; i++) {
            bucket += hits_;
            if (bucket >= steps_) {
                bucket -= steps_;
                pattern_[(i + offset_) % (steps_ + padding_)] = 1;
            } else {
                pattern_[(i + offset_) % (steps_ + padding_)] = 0;
            }
        }
    }
};

#endif
