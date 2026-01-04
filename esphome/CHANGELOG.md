# Changelog for ESPHome firmware

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-01-04

### Added

- Added settings for different solar modes
  - Now you can choose, how the `System Load`, `Production` and `Solar` displays
    display the Industrial and Prosumer solar data

### Changed

- Changed the solar value source from estimated to ultra-short-term forecast
  - The availability of the estimated values is not reliable, sometimes it
    doesn't updated for more then an hour.
- Changed the CPU frequency to 240 MHz

### Fixed

- Fixed an issue which caused the display to restart without reason

## [1.0.1] - 2025-12-21

### Breaking change

- The device will use the MD5 hash of the MAC address as the API Key,
  instead of a separately provided API Key

### Changed

- The default state of the displays is ON, so they will be on in the factory
  configuration
- Time is only displayed on the TFT display if it's correct (synchronized)
- Moved the error icons on the statusbar one position to the right

### Fixed

- Fixed an issue, where changin the display data to `gas`, didn't change
  the TFT display to the correct page

## [1.0.0] - 2025-12-17

- Initial version
