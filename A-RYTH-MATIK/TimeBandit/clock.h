#include "src/libmodulove/arythmatik.h"

#define PPQN 4
#define DEFAULT_BPM 50

// One minute in miliseconds.
const uint16_t MINUTE = 60 * 1000;

class Clock {
   public:
    Clock() {}
    ~Clock() {}

    /// @brief Enum constants for clock modification type.
    enum Mod {
        MOD_DIVISION,
        MOD_MULTIPLY,
    };

    void Init(modulove::DigitalOutput output, Mod mod, int factor) {
        output_ = output;
        mod_ = mod;
        factor_ = factor;
        SetBPM(DEFAULT_BPM, millis());
        output.High(); // Go!
    }

    int BPM() {
        return bpm_;
    }

    void SetBPM(int bpm, long now) {
        bpm_ = bpm;
        period_ = MINUTE / bpm_ / PPQN;
        recalculateDuty(now);
    }

    // Process clock state upon receiving an external trigger.
    void Process(long now) {
        period_ = now - lastInput_;
        lastInput_ = now;

        int bpm = MINUTE / (period_ * PPQN);
        if (bpm_ != bpm) {
            bpm_ = bpm;
            recalculateDuty(now);
            // TODO: we want to restart deadline, but not on divisions.
            // high_ = true;
            // output_.High();
        }
    }

    // Check for change to clock state with each cpu cycle.
    void Tick(long now) {
        if (now >= deadline_) {
            deadline_ = now + duty_;
            high_ = !high_;
            high_ ? output_.High() : output_.Low();
        }
    }

   private:
    modulove::DigitalOutput output_;
    Mod mod_;
    int factor_;

    // Initial default state.
    int bpm_;
    int period_;
    int duty_;
    long deadline_;
    long lastInput_ = millis();
    bool high_ = true;

    // Determine the duty cycle for the clock modification.
    void recalculateDuty(long now) {
        if (mod_ == MOD_DIVISION) {
            duty_ = (period_ * factor_) / 2;
        }
        else if (mod_ == MOD_MULTIPLY) {
            duty_ = ((period_ / factor_) / 2);
        }
        deadline_ = now + duty_;
    }

};
