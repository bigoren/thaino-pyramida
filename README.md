# Thaino Pyramida

## Setup

- Esp32 Master
- Esp32 sensors x n
- Teensy audio x 1
- Teensy Leds x 2

This code is for the teensy leds.

Esp32 sensors -> (wifi) -> esp32 master -> ??? -> teensy audio / teensy leds

## Concepts

- Trigger - represents a sensor event. 4 bits, up to 16 events
- State - controls which song + animations are playing


## States

### Idle

This is the state after 40 seconds of no inputs from the sensors


## Open Questions

- Why is there a delay(100) at the end?
- Do we want to start a new animation only when the previous one ends?
