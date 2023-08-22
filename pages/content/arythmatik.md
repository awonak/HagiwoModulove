---
title: "A-RYTH-MATIK"
layout: "single"
---

## Bit Garden

Deterministic psudo random gate processor. [[source](https://github.com/awonak/HagiwoModulove/tree/main/A-RYTH-MATIK/BitGarden)]

Each output will mirror CLK according to a decreasing deterministic probability
set by the seed value. RST input will reset the psudo random sequence. Use the
encoder to randomize the seed or adjust the pattern length. The user
configurable parameters of seed and step length will be saved to EEPROM and
will be recalled the next time you power on the module.

```yaml
Encoder:
     short press: Toggle between editing the step length and selecting a seed.
     long press: Enter seed edit mode to manually provide a seed value.

CLK: Provide a gate or trigger for each output to repeat with decreasing
     probability in each output.

RST: Trigger this input to reseed the psudo random sequence.

CV1-6: Gate output with decreasing probability.
```

{{< firmware_button hex="A-RYTH-MATIK_BitGarden.hex" >}}

## Uncertainty

Configurable stochastic gate processor. [[source](https://github.com/awonak/HagiwoModulove/tree/main/A-RYTH-MATIK/Uncertainty)]

This firmware is based on the [Olivia Artz Modular's Uncertainty](https://oamodular.org/products/uncertainty).
Connect a trigger or gate source to the CLK input and the each output will
mirror that signal according to a decreasing probability. Long press the
encoder to change trigger modes. `Trig` will simply echo the trigger or
gate received from the CLK input. `Flip` mode will toggle the output on
or off upon each new CLK input.

```yaml
Encoder:
     short press: Toggle between selecting an output and editing the outputs probability.
     long press: Enter Mode edit menu.

CLK: Provide a gate or trigger for each output to repeat with decreasing
     probability in each output.

CV1-6: Gate output with decreasing probability.

```

{{< firmware_button hex="A-RYTH-MATIK_Uncertainty.hex" >}}
