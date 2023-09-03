#pragma once

// Oled setting
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h>

// Encoder & button
#include <SimpleRotary.h>

#include "modulove_arythmatik_peripherials.h"
#include "modulove_digital_input.h"
#include "modulove_digital_output.h"

namespace modulove {

class Arythmatik {
   public:
    /** Constructor */
    Arythmatik() : 
      encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_SW_PIN),
      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {}
    /** Destructor */
    ~Arythmatik() {}

    /** Initializes the Arduino, and A-RYTH-MATIK hardware.*/
    void Init();

    void ProcessInputs();

    Adafruit_SSD1306 display;
    SimpleRotary encoder;
    DigitalOutput outputs[arythmatik::OUTPUT_COUNT];

    DigitalInput Clk;
    DigitalInput Rst;

   private:
    void InitInputs();
    void InitEncoder();
    void InitDisplay();
    void InitOutputs();
};
}  // namespace modulove
