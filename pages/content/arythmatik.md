---
title: "A-RYTH-MATIK"
layout: "single"
---

## Bit Garden

Each output will mirror CLK according to a decreasing deterministic probability
set by the seed value. RST input will reset the psudo random sequence. Use the
encoder to randomize the seed or adjust the pattern length. The user
configurable parameters of seed and step length will be saved to EEPROM and
will be recalled the next time you power on the module.

**Encoder**:
     short press: Toggle between editing the step length and selecting a seed.
     long press: Enter seed edit mode to manually provide a seed value.

**CLK**: Provide a gate or trigger for each output to repeat with decreasing
     probability in each output.

**RST**: Trigger this input to reseed the psudo random sequence.

{{< firmware_button hex="A-RYTH-MATIK_BitGarden.hex" >}}

## Uncertainty

Stochastic gate processor based on the Olivia Artz Modular's Uncertainty.
Connect a trigger or gate source to the CLK input and the each output will
mirror that signal according to a decreasing probability.

**CLK**: Provide a gate or trigger for each output to repeat with decreasing
     probability in each output.

{{< firmware_button hex="A-RYTH-MATIK_Uncertainty.hex" >}}
