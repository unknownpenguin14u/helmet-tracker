# AcousticCurtain-Module - Firmware

This directory contains the firmware source code for the microcontrollers that power the AcousticCurtain-Module hardware.

## Available Firmware

-   **`decibel_defender_esp32/`**: The recommended firmware for the standalone, wireless version of the project running on an **ESP32**. It includes all automation logic, a web server, and Wi-Fi management. See the `README.md` inside this directory for full instructions.

-   **`decibel_defender_esp32_improved/`**: A new version of the ESP32 firmware that includes several stability and performance improvements, such as better motor heat management, more stable audio processing, and enhanced debugging output. This is the recommended version for long-term use.

-   **`decibel_defender_esp8266/`**: Firmware for the wireless version of the project running on an **ESP8266**. See the `README.md` inside this directory for full instructions.

-   **`inmp441_test/`**: A simple diagnostic sketch to test if your **INMP441 digital microphone** is wired correctly.

For instructions on how to build the physical hardware, please refer to the `HARDWARE_GUIDES.md` file in the root of the project.
