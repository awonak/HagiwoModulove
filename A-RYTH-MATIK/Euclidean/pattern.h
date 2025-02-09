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

    uint8_t steps = 0;
    uint8_t hits = 0;
    uint8_t offset = 0;
    uint8_t padding = 0;
    uint8_t current_step = 0;

    /**
    Initializes the euclidean rhythm pattern with the given attributes.
      \param steps total length of the pattern.
      \param hits the number of "beats" to distribute evenly across the pattern steps.
      \param offset rotation of the pattern's hits.
      \param padding additional empty steps added to the pattern.
    */
    void Init(PatternState state) {
        steps = constrain(state.steps, 0, MAX_PATTERN_LEN);
        hits = constrain(state.hits, 1, steps);
        padding = constrain(state.padding, 0, MAX_PATTERN_LEN - steps);
        offset = constrain(state.offset, 0, (steps + padding) - 1);
        updatePattern();
    }

    PatternState GetState() { return {steps, hits, offset, padding}; }

    // Advance the euclidean rhythm to the next step in the pattern.
    // Returns 1 for hit, 0 for rest, and 2 for padding.
    int NextStep() {
        if (steps == 0) return 0;
        
        current_step =
            (current_step < steps + padding - 1) ? current_step + 1 : 0;
        return GetStep(current_step);
    }

    // Returns 1 for hit, 0 for rest, and 2 for padding.
    int GetStep(int i) { return pattern_[i]; }

    void ChangeSteps(int val) {
        if (val == 1 && steps < MAX_PATTERN_LEN) {
            steps++;
            padding = min(padding, MAX_PATTERN_LEN - steps);
            updatePattern();
        } else if (val == -1 && steps > 1) {
            steps--;
            hits = min(hits, steps);
            offset = min(offset, (steps + padding) - 1);
            updatePattern();
        } else if (val == -1 && steps == 1) {
            // Mute this pattern.
            steps = 0;
            offset = 0;
            padding = 0;
            updatePattern();
        }
    }

    void ChangeHits(int val) {
        if (val == 1 && hits < steps) {
            hits++;
            updatePattern();
        } else if (val == -1 && hits > 0) {
            hits--;
            updatePattern();
        }
    }

    void ChangeOffset(int val) {
        if (val == 1 && offset < (steps + padding) - 1) {
            offset++;
            updatePattern();
        } else if (val == -1 && offset > 0) {
            offset--;
            updatePattern();
        }
    }

    void ChangePadding(int val) {
        if (val == 1 && padding + steps < MAX_PATTERN_LEN) {
            padding++;
            updatePattern();
        } else if (val == -1 && padding > 0) {
            padding--;
            offset = min(offset, (padding + steps) - 1);
            updatePattern();
        }
    }

    void Reset() { current_step = steps; }

   private:
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
        pattern_[offset] = (hits > 0) ? 1 : 0;
        for (int i = 1; i < steps; i++) {
            bucket += hits;
            if (bucket >= steps) {
                bucket -= steps;
                pattern_[(i + offset) % (steps + padding)] = 1;
            } else {
                pattern_[(i + offset) % (steps + padding)] = 0;
            }
        }
    }
};

#endif
