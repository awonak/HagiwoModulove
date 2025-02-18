#ifndef CLOCK_H
#define CLOCK_H

// Include the Modulove library.
#include <arythmatik.h>
#include <FlexiTimer2.h>


using namespace modulove;
using namespace arythmatik;

#define MAX_PATTERN_LEN 32

// Default internal PPQN.
const byte PPQN = 24;

// CPU processing time to remove from clock interval in order to get more
// precise clock speeds. Measured for most precision at 120 BPM.
const byte CPU_LAG_MS = 10;

// Clock PPQN Resolution with pulse subdivision factor.
enum ClockResolution {
    CLOCK_RESOLUTION_4_PPQN,
    CLOCK_RESOLUTION_8_PPQN,
    CLOCK_RESOLUTION_24_PPQN,
    CLOCK_RESOLUTION_LAST
  };
int clock_resolution_divisor[3] = {6, 3, 1}; 
int clock_resolution_display[3] = {24, 8, 4}; 

// Modify the master clock speed with division of 128 to multiplication up to 8. 
enum ClockMod {
    CLOCK_MOD_MULT_8,
    CLOCK_MOD_MULT_6,
    CLOCK_MOD_MULT_4,
    CLOCK_MOD_MULT_3,
    CLOCK_MOD_MULT_2,
    CLOCK_MOD_MULT_1,
    CLOCK_MOD_DIV_2,
    CLOCK_MOD_DIV_3,
    CLOCK_MOD_DIV_4,
    CLOCK_MOD_DIV_6,
    CLOCK_MOD_DIV_8,
    CLOCK_MOD_DIV_12,
    CLOCK_MOD_DIV_16,
    CLOCK_MOD_DIV_24,
    CLOCK_MOD_DIV_32,
    CLOCK_MOD_DIV_64,
    CLOCK_MOD_DIV_128,
    CLOCK_MOD_LAST,
};

// The clock mod tick value represents the number of "pulses" between a 24PPQN resolution master clock.
int clock_mod_ticks[] = {3, 4, 6, 8, 12, 24, 48, 72, 96, 155, 192, 288, 384, 576, 768, 1536, 3072}; 

// Clock modifier per output channel.
static ClockMod clock_mod[OUTPUT_COUNT] {
    CLOCK_MOD_MULT_1,
    CLOCK_MOD_MULT_1,
    CLOCK_MOD_MULT_1,
    CLOCK_MOD_MULT_1,
    CLOCK_MOD_MULT_1,
    CLOCK_MOD_MULT_1,
};

void ChangeClockMod(int val, int channel) {
    clock_mod[channel] = static_cast<ClockMod>((clock_mod[channel] + val) % CLOCK_MOD_LAST);
}

void StartClock(uint8_t tempo, uint8_t ppqn, void (*f)()) {
    // 24PPQN period in milliseconds.
    unsigned long interval = ((60.0 * 10000) / (double(tempo))) / ppqn;
    FlexiTimer2::set(interval - CPU_LAG_MS, (1.0 / 10000.0 / 4.0), f);
    FlexiTimer2::start();
}

void StopClock() {
    FlexiTimer2::stop();
}

#endif
