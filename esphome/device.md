---
title: HUEnergyDisplay
date-published: 2025-12-19
type: misc
standard: global
board: esp32
project-url: https://github.com/denes44/HUEnergyDisplay/tree/main/esphome
made-for-esphome: True
difficulty: 1
---

Custom 7 segment display for displaying the hungarian power or natural gas
transmission system.

The hardware of this project is a 210 mm x 300 mm sized bare PCB,
designed to fit into a picture frame, especially the IKEA RÖDALM.

The display has multiple 7 segment displays for different data arranged around
the map of the country, like: system load, frequency (for power), import/export
for the neighbouring countries, production, etc.

It also has a 2.25" TFT display for displaying status and other miscellaneous
data.

The 7 segment displays are driven by MAX7219 ICs (11 ICs for 12 displays in
total).

## GPIO Pinout

| Pin    | Function         |
| ------ | ---------------- |
| GPIO0  | Boot button      |
| GPIO1  | MAX7219 CLK      |
| GPIO12 | MAX7219 MOSI     |
| GPIO13 | MAX7219 CS       |
| GPIO5-10 | 2.25" TFT display |
| GPIO4  | Ambient light sensor |
| GPIO48 | Status Led (currently not used) |

## Basic Configuration

The [Latest configuration](https://github.com/denes44/HUEnergyDisplay/tree/main/esphome)
can be found on the project's GitHub repo.
