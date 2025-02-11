---
title: "A-RYTH-MATIK"
layout: "single"
---

## Bit Garden

Deterministic psudo random gate processor. [[source](https://github.com/awonak/HagiwoModulove/tree/main/A-RYTH-MATIK/BitGarden)]

Each output will mirror CLK according to a decreasing deterministic probability
set by the seed value. RST input will reset the psudo random sequence. Short
press of the encoder will select a diffent editable parameter on the current
page. Long press the encoder to select a different page. The user configurable
parameters will be saved to EEPROM and will be recalled the next time you power
on the module.

Main page - adjust the pattern length or randomize the seed.

Probability page - configure the probability for each output

Output Mode page - set the output behavior (trigger, gate, flip)

Seed page - manually enter a seed value.

{{< youtube PlufCV3CNw8 >}}

```yaml
Encoder:
     short press: Toggle between editable parameters on the current page.
     long press: Enter page select mode.

CLK: Provide a gate or trigger for each output to repeat with decreasing
     probability in each output.

RST: Trigger this input to reseed the psudo random sequence.

CV1-6: Gate output with decreasing probability.
```

{{< encoder_firmware_button hex="A-RYTH-MATIK_BitGarden" buttonText="Flash Bit Garden Firmware" >}}

## Euclidean

Euclidean Rhythms. [[source](https://github.com/awonak/HagiwoModulove/tree/main/A-RYTH-MATIK/Euclidean)]

Configure each output with a Euclidean Rhythm defined by the number of steps,
the number of hits or beats evenly distributed across the steps, a rotation
offset of the pattern, and finally rest padding appended to the end of the
pattern.

Use the encoder to select which parameter to edit, and click the encoder
button to chanve between select and edit mode. Hold and rotate the encoder
to change the selected output channel. Long press to change global settings
like output mode (trigger, gate, flip) and internal clock tempo.

{{< youtube 5RXct6RRtnQ >}}

```yaml
ENCODER:
     Short press to change between selecting a prameter and editing the parameter.
     Long press to change global settings like output mode (trigger, gate, flip) and internal clock tempo.
     Double Click to reset all patterns back to the first step.
     Hold & rotate to change current output channel pattern.

CLK: Clock input used to advance the patterns.

RST: Trigger this input to reset all pattern back to the first step.
```

{{< encoder_firmware_button hex="A-RYTH-MATIK_Euclidean" buttonText="Flash Euclidean Firmware" >}}

## Time Bandit

Fixed binary clock divider and sub oscillator. [[source](https://github.com/awonak/HagiwoModulove/tree/main/A-RYTH-MATIK/TimeBandit/TimeBandit.ino)]

The 6 digital outputs will produce a 50% duty cycle square wave in fixed
binary divisions of the incoming CLK signal. This can be used as a typical
clock divider or provide sub octaves of the incoming audio rate signal.
Each output is one octave lower than the previous.

{{< youtube xAgkP6kvcgA >}}

```yaml

ENCODER: Select which division to edit. Single press to change the
         selected output. Single press again to leave edit mode.

CLK: Clock input used to produce fixed binary divisions.

RST: Trigger this input to reset the division counter.

CV1-6: Configurable binary clock divisions of 2, 4, 8, 16, 32..8192

```

{{< encoder_firmware_button hex="A-RYTH-MATIK_TimeBandit" buttonText="Flash Time Bandit Firmware">}}

## Uncertainty

Configurable stochastic gate processor. [[source](https://github.com/awonak/HagiwoModulove/tree/main/A-RYTH-MATIK/Uncertainty)]

This firmware is based on the [Olivia Artz Modular's Uncertainty](https://oamodular.org/products/uncertainty).
Connect a trigger or gate source to the CLK input and the each output will
mirror that signal according to a decreasing probability. Long press the
encoder to change trigger modes. `Trig` will simply echo the trigger or
gate received from the CLK input. `Flip` mode will toggle the output on
or off upon each new CLK input.

{{< youtube SHZaS9hu4qI >}}

```yaml
Encoder:
     short press: Toggle between selecting an output and editing the outputs probability.
     long press: Enter Mode edit menu.

CLK: Provide a gate or trigger for each output to repeat with decreasing
     probability in each output.

CV1-6: Gate output with decreasing probability.

```

{{< encoder_firmware_button hex="A-RYTH-MATIK_Uncertainty" buttonText="Flash Uncertainty Firmware">}}
