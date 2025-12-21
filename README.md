# HUEnergyDisplay

![Based on ESP32-S3 board](https://img.shields.io/badge/hardware-ESP32--S3-blue?logo=espressif)
![With ESPHome as a firmare](https://img.shields.io/badge/firmware-esphome-blue?logo=esphome)
![Home Assistant Compatible](https://img.shields.io/badge/homassistant-compatible-blue?logo=homeassistant)

Custom 7 segment display for displaying the hungarian power or natural gas
transmission system.

The display has multiple 7 segment displays for different data arranged around
the map of the country:

<img align="left" src="hardware/pcb.png" width="400" alt="Design of the PCB">

* **Power:**
  * System load
  * Frequency
  * Power plant production
  * Solar production
  * Import/export to neighbouring contries, and total balance
  * Detailed power plant production on the TFT display
* **Gas:**
  * Total consumption (system load)
  * Storage container ratio
  * Production
  * Storage extraction
  * Import/export to neighbouring contries, and total balance

It also has a 2.25" TFT display for displaying status and other miscellaneous
data.

If you are looking for the easiest way to **start using your display
under Home Assistant:** ***[Home Assistant Quick Start Guide](HomeAssistant.md)***

## Hardware

The hardware of this project is a 300 mm x 210 mm sized bare PCB,
designed to fit into a picture frame, especially the IKEA RÖDALM.

More information about the hardware (PCB size, WiFi controller, display, etc.),
can be found in the **[hardware](hardware/README.md)** folder.

### ESPHome

The display uses an ESP32-S3 controller and ESPHome as it's firmware.

More information about the ESPHome firmware can be found in the
**[esphome](esphome/README.md)** folder.

## API

The display is using it's own private API, which collect and processes
the neccessary datas, and present it in an easy JSON format to the on-board
controller.

The API requires an API Key. By default, the displays are using the MD5 hash
of the MAC address of the controller as the API Key.
