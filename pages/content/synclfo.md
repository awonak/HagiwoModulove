---
title: "SyncLFO"
layout: "single"
---

## ADSR

Envelope Generator firmware for HAGIWO Sync Mod LFO.

{{< code language="docs" title="Knob mapping" id="1" expand="Show" collapse="Hide" isCollapsed="true" >}}
Knob 1: Attack
Knob 2: Decay
Knob 3: Sustain
Knob 4: Release
{{< /code >}}

{{< firmware_button
    hex="SyncLFO_ADSR.hex"
    buttonText="Flash Firmware" >}}

## Baby4

4 step cv sequencer firmware for HAGIWO Sync Mod LFO

{{< firmware_button
    hex="SyncLFO_Baby4.hex"
    buttonText="Flash Firmware" >}}

## GenerativeSequencer

Generative Sequencer firmware for HAGIWO Sync Mod LFO

demo video: [https://youtu.be/okj47TcXJXg](https://youtu.be/okj47TcXJXg)

{{< code language="docs" title="Knob mapping" id="2" expand="Show" collapse="Hide" isCollapsed="true" >}}
Knob 1: Probability
Knob 2: Sequence step length
Knob 3: Amplitiude
Knob 4: Refrain count
{{< /code >}}

{{< firmware_button
    hex="SyncLFO_GenerativeSequencer.hex"
    buttonText="Flash Firmware" >}}

## MultiModeEnv

Multi-mode Envelope Generator firmware for HAGIWO Sync Mod LFO

{{< code language="docs" title="Knob mapping" id="3" expand="Show" collapse="Hide" isCollapsed="true" >}}
Knob 1: Attack
Knob 2: Release
Knob 3: Mode (AR, ASR, SLOW AR (10x), LFO)
Knob 4: Curve (LOG, LINEAR, EXP)
{{< /code >}}

{{< firmware_button
    hex="SyncLFO_MultiModeEnv.hex"
    buttonText="Flash Firmware" >}}

## Polyrhythm

Generate polyrhythms based on 16 step counter knobs for HAGIWO Sync Mod LFO

Each knob acts as a subdivision of the incoming clock. When a rhythm knob
is fully CCW (0) no rhythm is set. Moving the rhythm knob CW, the first
subdivision is every 16 beats. When the knob is fully CW, that is a
subdivision of 1 and the polyrhythm will trigger every beat.

When in OR mode, any beat that has more than one rhythm trigger will output
an accented 5v, otherwise a single rhythm trigger will output 3v.

{{< code language="docs" title="Knob mapping & logic modes" id="4" expand="Show" collapse="Hide" isCollapsed="true" >}}
Knob 1: Polyrhythm count 1
Knob 2: Polyrhythm count 2
Knob 3: Polyrhythm count 3
Knob 4: Logic mode selection (OR, XOR, NOR, AND)

Logic Modes:

OR -  Trigger if any rhythm hits on the beat and will accent with more than one hit.
XOR - Only trigger if one and only one rhythm hits on this beat.
NOR - Only trigger when a polyphythm does not hit.
AND - Only trigger if more than on rhythm hits on this beat.
{{< /code >}}

{{< firmware_button
    hex="SyncLFO_MultiModeEnv.hex"
    buttonText="Flash Firmware" >}}
