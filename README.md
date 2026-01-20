# Description

This repository contains driver for WS2812 LEDs, running on ATtiny13A (9.6 MHz internal RC).

# Pinout

PB3 - data input pin for ws2812
PB4 - used to switch modes. Debounce is implemented using the watchdog timer.

# Modes

The firmware supports 9 LED modes using wave-like brightness patterns:
- 3 modes: single-color waves (Red, Green, Blue)
- 6 modes: dual-color waves, combining a main color with a secondary shade

# Dependencies

Using tinyLED C++ header only [library](https://github.com/GyverLibs/microLED/) from AlexGyver
