#include "modulove_arythmatik.h"

using namespace modulove;

void Arythmatik::Init() {
    InitInputs();
    InitEncoder();
    InitOutputs();
    InitDisplay();

    // CLOCK LED (DIGITAL)
    pinMode(CLOCK_LED, OUTPUT);
}

void Arythmatik::InitInputs() {
    Clk.Init(CLK_PIN);
    Rst.Init(RST_PIN);
}

void Arythmatik::InitEncoder() {
    // Rotary encoder and push switch.
    SimpleRotary encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_SW_PIN);
}

void Arythmatik::InitDisplay() {
    // OLED Display configuration.
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    delay(1000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("DISPLAY READY");
    display.display();
}

void Arythmatik::InitOutputs() {
    // Initialize each of the outputs with it's GPIO pins and probability.
    outputs[0].Init(OUT_CH1, LED_CH1);
    outputs[1].Init(OUT_CH2, LED_CH2);
    outputs[2].Init(OUT_CH3, LED_CH3);
    outputs[3].Init(OUT_CH4, LED_CH4);
    outputs[4].Init(OUT_CH5, LED_CH5);
    outputs[5].Init(OUT_CH6, LED_CH6);
}

void Arythmatik::ProcessInputs() {
    Clk.Process();
    Rst.Process();

    // Clock In LED indicator mirrors the clock input.
    if (Clk.State() == DigitalInput::STATE_RISING) {
        digitalWrite(CLOCK_LED, HIGH);
    } else if (Clk.State() == DigitalInput::STATE_FALLING) {
        digitalWrite(CLOCK_LED, LOW);
    }
}
