#ifndef PATTERN_H
#define PATTERN_H

// Include the Modulove library.
#include <arythmatik.h>

using namespace modulove;

#define MAX_PATTERN_LEN 32

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
      \param output arythmatik cv output.
      \param steps total length of the pattern.
      \param hits the number of "beats" to distribute evenly across the pattern steps.
      \param offset rotation of the pattern's hits.
      \param padding additional empty steps added to the pattern.
    */
    void Init(int _steps, int _hits, int _offset, int _padding) {
        steps = _steps;
        hits = _hits;
        offset = _offset;
        padding = _padding;
        updatePattern();
    }

    // Advance the euclidean rhythm to the next step in the pattern.
    bool NextStep() {
        current_step = (current_step < steps - 1) ? current_step + 1 : 0;
        return GetStep(current_step);
    }

    bool GetStep(int i) { return (pattern_[i] == 1); }

    void ChangeSteps(int val) {
        if (val == 1 && steps < MAX_PATTERN_LEN) {
            steps++;
        } else if (val == -1 && steps > 0) {
            steps--;
            hits = min(hits, steps);
        }
        updatePattern();
    }

    void ChangeHits(int val) {
        if (val == 1 && hits < steps) {
            hits++;
        } else if (val == -1 && hits > 1) {
            hits--;
        }
        updatePattern();
    }

    void ChangeOffset(int val) {
        if (val == 1 && offset < steps - 1) {
            offset++;
        } else if (val == -1 && offset > 0) {
            offset--;
        }
        updatePattern();
    }

    void Reset() { current_step = steps; }

   private:
    uint8_t pattern_[MAX_PATTERN_LEN];

    // Update the euclidean rhythm pattern when attributes change.
    void updatePattern() {
        // Erase current pattern
        for (int i = 0; i < MAX_PATTERN_LEN; i++) {
            pattern_[i] = 0;
        }

        // Populate the euclidean rhythm pattern according to the current
        // instance variables.
        int bucket = 0;
        pattern_[offset] = 1;
        for (int i = 1; i < steps; i++) {
            bucket += hits;
            if (bucket >= steps) {
                bucket -= steps;
                pattern_[(i + offset) % steps] = 1;
            } else {
                pattern_[(i + offset) % steps] = 0;
            }
        }
    }
};

#endif
