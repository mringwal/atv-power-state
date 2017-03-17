# Apple TV Power State Listener

This library allows to generate an event when a human starts to interact with an Apple TV and when it is put to sleep.

It was created to control a projector and a motorized screen without additional remote: now, the projector turns on and the screen comes down automatically when I press the Siri button.

## How does it work
The library assumes that the Apple TV is connected via an USB cable and uses [libimobildevice](http://www.libimobiledevice.org) to access the system log.

Based on particular events in the system log, the library detects if the Siri button was pressed (as indicator for an active human) and when the Apple TV goes to sleep - either automatically or forced by holding the HOME button for 2 seconds.

## Usage
The library allows for easy integration into custom home automation projects. A simple 'atv-listener' tool can be used for testing.

For integration, just add atv.c and atv.h to your project and link against libimobiledevice. Then, register for Apple TV Power State updates via atv_init(..).

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
