# HUEnergyDisplay - Home Assistant Quick Start Guide

## Installing the firmware

If you got your display with the controller and firmware preinstalled,
you can skip to the next step.

## Connect the display to WiFi

### Using Improv Wi-Fi

- With your phone (or other BLE capable device), go to [improv-wifi.com](https://www.improv-wifi.com/)
- At `Improv via BLE` click `Connect device to Wi-Fi`
- Select the device, and click `Pair`
- Enter your Wi-Fi network's credentials (SSID and password) and click `Save`
- The device will now connect to the selected Wi-Fi network

### Connecting to the Wi-Fi AP provided by the device

## Add the display to Home Assistant

After connecting the display to your WiFi network, click on this button
to open the integrations page in Home Assistant:

[![Open your Home Assistant instance and show your integrations.](https://my.home-assistant.io/badges/integrations.svg)](https://my.home-assistant.io/redirect/integrations/)

Home Assistant should automatically recognize the device from the network.

If not, you can manually add itt using the ESPHome integration:

[![Open your Home Assistant instance and start setting up a new integration.](https://my.home-assistant.io/badges/config_flow_start.svg)](https://my.home-assistant.io/redirect/config_flow_start/?domain=esphome)

## Using the device in Home Assistant
