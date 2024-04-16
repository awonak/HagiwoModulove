---
title: "SyncLFO"
---

## ADSR

ADSR envelope generator firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/ADSR/ADSR.ino)]

Input a trigger
to begin the rising attack phase. A gate input will hold the sustain value
until the gate closes. When the trigger or gate ends, the release phase will
begin. The Attack, Decay, and Release knob defines the amount of time each
phase should take to complete, with a range of 0 to ~2.5 seconds.

```yaml
Knob 1: Attack
Knob 2: Decay
Knob 3: Sustain
Knob 4: Release

GATE_IN: Gate in / Re-trig
CV_OUT: Envelope Output
```

{{< firmware_button hex="SyncLFO_ADSR" buttonText="Flash ADSR Firmware" >}}

## Baby4

4 step cv sequencer firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/Baby4/Baby4.ino)]

Each knob can be set to
a 0 to 10v value based on the knob position. Each trigger received by the
clock input will advance the output voltage to the next knob value.

{{< youtube f330NK89ajE >}}

```yaml
Knob 1: Step 1 voltage
Knob 2: Step 2 voltage
Knob 3: Step 3 voltage
Knob 4: Step 4 voltage

TRIG_IN: Advance sequence step
CV_OUT: Current step voltage
```

{{< firmware_button hex="SyncLFO_Baby4" buttonText="Flash Baby4 Firmware" >}}

## GenerativeSequencer

Generative Sequencer firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/GenerativeSequencer/GenerativeSequencer.ino)]

This random voltage
sequencer is inspired by the Deja Vu mode of the Mutable Instruments Marbles.
The probability knob determines the chance for a new voltage to be set at the
current sequence step. The sequence length knob sets the number of steps
in the sequence, ranging from 1 to 16 steps. Refrain count determines the
number of times the sequence must complete before a new voltage may be set.

{{< youtube okj47TcXJXg >}}

```yaml
Knob 1: Probability for new random voltage
Knob 2: Sequence step length
Knob 3: Amplitiude
Knob 4: Refrain count

TRIG_IN: Advance sequence step
CV_OUT: Current step voltage
```

{{< firmware_button hex="SyncLFO_GenerativeSequencer" buttonText="Flash GenerativeSequencer Firmware" >}}

## MultiModeEnv

Multi-mode Envelope Generator firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/MultiModeEnv/MultiModeEnv.ino)]

This envelope generator has several modes and curve algorithms. AR mode will
simply rise and fall based on the attack and release times set. ASR mode will
hold high while the input gate is high. Slow AR will take 10x as long as the
default AR with a max envelope time of about 20 seconds. Lastly, the LFO mode
does not respond to the trigger input; instead it endlessly cycles between the
attack and release phases. Curve options include logarithmic, linear and
exponential.

{{< youtube r2EI1wnIoRg >}}

```yaml
Knob 1: Attack
Knob 2: Release
Knob 3: Mode (AR, ASR, SLOW AR (10x), LFO)
Knob 4: Curve (LOG, LINEAR, EXP)

GATE_IN: Gate in / Re-trig
CV_OUT: Envelope Output
```

{{< firmware_button hex="SyncLFO_MultiModeEnv" buttonText="Flash MultiModeEnv Firmware" >}}

## Perlin Noise

Smooth random voltages to chaotic noise using the Perlin noise algorithm. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/PerlinNoise/PerlinNoise.ino)]

Unlike random() in which each new random value has no correlation to it's
previous value, Perlin noise has a more organic appearance because it
produces a naturally ordered "smooth" sequence of pseudo-random numbers.

Additionally, the random values produced by the Perlin noise algorithm
adhere to a bell curve distribution, meaning the OUT cv will hover around
5v and the BI cv output will hover around 0v. When the Knob 1 offset is
fully CCW the output will produce 0-5v hovering around 2.5v and when it is
fully CW the output will produce 5-10v hovering around 8.5v.

{{< youtube 7n8kDnDUpMI >}}

```yaml
Knob 1: Offset: Increase or decrease the center point the voltage hovers around
Knob 2: Frequency: the rate at which the value changes
Knob 3: Bit Crush: Reduce the bit depth of the output
Knob 4: Input mode: Open, Sample & Hold, Track & Hold, Gate

Input Trigger/Gate Modes:
    OPEN: The output will continually produce random voltages regardless of CLK input.
    SAMPLE_HOLD: Produce a new stable random voltage upon each trigger input.
    TRACK_HOLD: Track and Hold, output the random voltage and hold the value upon gate input.
    GATE: Also known as Hold and Track, only output voltage while the Gate input is high.

```

{{< firmware_button hex="SyncLFO_PerlinNoise" buttonText="Flash Perlin Noise Firmware" >}}

## Polyrhythm

Generate polyrhythms based on 16 step counter knobs. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/Polyrhythm/Polyrhythm.ino)]

Each knob acts as a subdivision of the incoming clock. When a rhythm knob
is fully CCW (0) no rhythm is set. Moving the rhythm knob CW, the first
subdivision is every 16 beats. When the knob is fully CW, that is a
subdivision of 1 and the polyrhythm will trigger every beat.

When in OR mode, any beat that has more than one rhythm trigger will output
an accented 5v, otherwise a single rhythm trigger will output 3v.

{{< youtube wcTAcc08-tY >}}

```yaml
Knob 1: Polyrhythm count 1
Knob 2: Polyrhythm count 2
Knob 3: Polyrhythm count 3
Knob 4: Logic mode selection (OR, XOR, NOR, AND)

Logic Modes:
    OR:  Trigger if any rhythm hits on the beat and will accent with more than one hit.
    XOR: Only trigger if one and only one rhythm hits on this beat.
    NOR: Only trigger when a polyphythm does not hit.
    AND: Only trigger if more than on rhythm hits on this beat.

TRIG_IN: Trigger Input to advance the rhythm counter
CV_OUT: CV Output for polyrhythm triggers
```

{{< firmware_button hex="SyncLFO_Polyrhythm" buttonText="Flash Polyrhythm Firmware" >}}

## Trigger Delay

Trigger delay with gate and slope settings. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/TriggerDelay/TriggerDelay.ino)]

Repeat the incoming trigger with a delay up to 2 seconds set by P1 for a
duration up to 2 seconds set by P2. Additionally, the output rising edge
can be slewed using P3 and the falling edge can be slewed using P4.

{{< youtube crwqMRxqejU >}}

```yaml
Knob 1: Trigger delay
Knob 2: Gate duration
Knob 3: Attack slew
Knob 4: Release slew

TRIG_IN: Start or restart the trigger delay
CV_OUT: Delayed CV output
```

{{< firmware_button hex="SyncLFO_TriggerDelay" buttonText="Flash Trigger Delay Firmware" >}}
