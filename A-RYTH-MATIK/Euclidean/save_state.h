#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <EEPROM.h>
#include <arythmatik.h>

#include "pattern.h"

using namespace modulove;
using namespace arythmatik;

// Script state & storage variables.
// Expected version string for this firmware.
const char SCRIPT_NAME[] = "EUCLIDEAN";
const uint8_t SCRIPT_VER = 4;

const PatternState default_pattern = {16, 4, 0, 0};

// Enum for the current selected output mode.
enum OutputMode {
    // Follow the state of the input clock.
    TRIGGER,
    // 100% Duty cycle gate.
    GATE,
    // Toggle between on/off with each clock input rising edge.
    FLIP,

    OUTPUTMODE_LAST,
};

struct State {
    // Version check.
    char script[sizeof(SCRIPT_NAME)];
    uint8_t version;
    // State variables.
    PatternState pattern[OUTPUT_COUNT];
    OutputMode output_mode;
    uint8_t selected_out;
    uint8_t tempo;
    bool internal_clock;
};
State state;

// Limit the total number of save slots available.
const int SAVE_SLOT_COUNT = 4;

// Save state to EEPROM if state has changed.
void SaveChanges(Pattern *patterns) {
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        state.pattern[i] = (PatternState){patterns[i].GetState()};
    }
    EEPROM.put(0, state);
}

int GetBankAddress(byte bank) {
    byte _bank = constrain(bank, 0, SAVE_SLOT_COUNT);
    return sizeof(State) + (sizeof(PatternState) * OUTPUT_COUNT * _bank);
}

void SavePreset(Pattern *patterns, byte bank) {
    PatternState pattern_to_save[OUTPUT_COUNT];

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        pattern_to_save[i] = (PatternState){patterns[i].GetState()};
    }

    EEPROM.put(GetBankAddress(bank), pattern_to_save);
}

void LoadPreset(Pattern *patterns, byte bank) {
    PatternState saved_pattern[OUTPUT_COUNT];
    EEPROM.get(GetBankAddress(bank), saved_pattern);

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        patterns[i].Init(saved_pattern[i]);
    }
}

// Initialize script state from EEPROM or default values.
void InitState(Pattern *patterns) {
    // Read previously put state from EEPROM memory. If it doesn't exist or
    // match version, then populate the State struct with default values.
    EEPROM.get(0, state);

    // Check if the data in memory matches expected values.
    if ((strcmp(state.script, SCRIPT_NAME) != 0) || (state.version != SCRIPT_VER)) {
        // Set script version identifier values.
        strcpy(state.script, SCRIPT_NAME);
        state.version = SCRIPT_VER;

        // Default state vars.
        state.output_mode = TRIGGER;
        state.selected_out = 0;
        state.tempo = 130;
        state.internal_clock = false;

        // Provide a even distribution of default probabilities.
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            patterns[i].Init(default_pattern);
        }

        // Write default pattern to all save banks.
        for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
            SavePreset(patterns, i);
        }
    } else {
        // Provide a even distribution of default probabilities.
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            patterns[i].Init(state.pattern[i]);
        }
    }
}

#endif
