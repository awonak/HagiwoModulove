# Hagiwo/Modulove

This is a collection of open source alternative firmwares for Modulove's [Sync LFO](https://modulove.io/synclfo/) and [A-RYTH-MATIK](https://modulove.io/arythmatik/).

Documentation for each alt firmware is available at [https://awonak.github.io/HagiwoModulove/](https://awonak.github.io/HagiwoModulove/). This site also provides the ability to flash the firmware to your module directly from your web browser! No need to compile or upload from the Arduino IDE!

Original open source hardware/firmware and design can be found here:

Hagiwo

* [Euclidean Rhythms](https://note.com/solder_state/n/n433b32ea6dbc)
* [Sync LFO](https://note.com/solder_state/n/n4c600f2431c3)

Modulove

* [A-RYTH-MATIK](https://github.com/modulove/A-RYTH-MATIK)
* [Sync LFO](https://github.com/modulove/CATs-Eurosynth/tree/main/Modules/HAGIWO/Sync%20LFO)

## Firmware Development

### Clone the repo and dependencies

```shell
# Clone the HagiwoModulove repository
$ git clone https://github.com/awonak/HagiwoModulove.git
# Install libmodulove from GitHub using the arduino cli.
$ arduino-cli config init
$ arduino-cli config set library.enable_unsafe_install true
$ arduino-cli lib install --git-url https://github.com/awonak/libModulove.git 
# OR optionally clone libmodulove for local development
$ git clone https://github.com/awonak/libModulove.git
$ ln -s `pwd`/libmodulove ~/Arduino/libraries        
```

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
    "-mmcu=atmega328"
]
```

Next add your Arduino libraries path to the `includePath` list:

```json
"includePath": [
    "${HOME}/Arduino/libraries/**",
    "${HOME}/.arduino15/packages/arduino/hardware/avr/1.8.6/libraries/**"
]
```

To make sure the Arduino extension does not overwrite these changes, update `.vscode/arduino.json` and add the following config:

```json
    "intelliSenseGen": "disable"
```

### Additional User Settings for Arduino extension configurations.

Additional board config for an LGT8F328P Arduino Nano clone

```json
"arduino.additionalUrls": [
    "https://raw.githubusercontent.com/dbuezas/lgt8fx/master/package_lgt8fx_index.json",
],
```

You're now ready to start hacking!

### Third-party Arduino Libraries

* [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
* [EncoderButton](https://github.com/Stutchbury/EncoderButton)
* [AVR IO](https://github.com/avrdudes/avr-libc)
* [FastLED](https://github.com/FastLED/FastLED)
* [FlexiTimer2](https://github.com/wimleers/flexitimer2)
