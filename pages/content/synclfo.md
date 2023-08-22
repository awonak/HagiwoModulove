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

{{< firmware_button hex="SyncLFO_ADSR.hex" >}}

## Baby4

4 step cv sequencer firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/Baby4/Baby4.ino)]

Each knob can be set to
a 0 to 10v value based on the knob position. Each trigger received by the
clock input will advance the output voltage to the next knob value.

```yaml
Knob 1: Step 1 voltage
Knob 2: Step 2 voltage
Knob 3: Step 3 voltage
Knob 4: Step 4 voltage

TRIG_IN: Advance sequence step
CV_OUT: Current step voltage
```

{{< firmware_button hex="SyncLFO_Baby4.hex" >}}

## GenerativeSequencer

Generative Sequencer firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/GenerativeSequencer/GenerativeSequencer.ino)]

This random voltage
sequencer is inspired by the Deja Vu mode of the Mutable Instruments Marbles.
The probability knob determines the chance for a new voltage to be set at the
current sequence step. The sequence length knob sets the number of steps
in the sequence, ranging from 1 to 16 steps. Refrain count determines the
number of times the sequence must complete before a new voltage may be set.

demo video: [https://youtu.be/okj47TcXJXg](https://youtu.be/okj47TcXJXg)

```yaml
Knob 1: Probability for new random voltage
Knob 2: Sequence step length
Knob 3: Amplitiude
Knob 4: Refrain count

TRIG_IN: Advance sequence step
CV_OUT: Current step voltage
```

{{< firmware_button hex="SyncLFO_GenerativeSequencer.hex" >}}

## MultiModeEnv

Multi-mode Envelope Generator firmware. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/MultiModeEnv/MultiModeEnv.ino)]

This envelope generator has several modes and curve algorithms. AR mode will
simply rise and fall based on the attack and release times set. ASR mode will
hold high while the input gate is high. Slow AR will take 10x as long as the
default AR with a max envelope time of about 20 seconds. Lastly, the LFO mode
does not respond to the trigger input; instead it endlessly cycles between the
attack and release phases. Curve options include logarithmic, linear and
exponential.

```yaml
Knob 1: Attack
Knob 2: Release
Knob 3: Mode (AR, ASR, SLOW AR (10x), LFO)
Knob 4: Curve (LOG, LINEAR, EXP)

GATE_IN: Gate in / Re-trig
CV_OUT: Envelope Output
```

{{< firmware_button hex="SyncLFO_MultiModeEnv.hex" >}}

## Polyrhythm

Generate polyrhythms based on 16 step counter knobs. [[source](https://github.com/awonak/HagiwoModulove/blob/main/SyncLFO/Polyrhythm/Polyrhythm.ino)]

Each knob acts as a subdivision of the incoming clock. When a rhythm knob
is fully CCW (0) no rhythm is set. Moving the rhythm knob CW, the first
subdivision is every 16 beats. When the knob is fully CW, that is a
subdivision of 1 and the polyrhythm will trigger every beat.

When in OR mode, any beat that has more than one rhythm trigger will output
an accented 5v, otherwise a single rhythm trigger will output 3v.

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

{{< firmware_button hex="SyncLFO_Polyrhythm.hex" >}}
