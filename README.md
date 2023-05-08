# Hagiwo/Modulove Modules

This is a collection of open source hardware and software files related to Modulove hardware remixes of Hagiwo modules.

## Hardware

* [Sync LFO](SyncLFO/)

## Software

### Configure Visual Studio Code for your IDE

Required Extensions:

* [Arduino](https://github.com/Microsoft/vscode-arduino)
* [C/C++](https://github.com/Microsoft/vscode-cpptools)

### Configure the Arduino extension

Arduino Board Configuration: `Arduino Nano`

Select Programmer: `AVRISP mkii`

### Additional Intellisense configuration

```text
CMD + Shift + P > C/C++: Edit Configurations (JSON)
```

Within the `Arduino` configuraiton, add `-mmcu=atmega328` to the `compilerArgs` list:

```json
"compilerArgs": [
    ...
    "-mmcu=atmega328"
]
```

Next add your Arduino libraries path to the `includePath` list:

```json
"includePath": [
    ...
    "${HOME}/Arduino/libraries/**"
]
```

To make sure the Arduino extension does not overwrite these changes, update `.vscode/arduino.json` and add the following config:

```json
    "intelliSenseGen": "disable"
```

You're now ready to start hacking!
