# AcousticCurtain-Module - ESP32 Firmware (Improved Version)

This folder contains an improved and more stable version of the ESP32 firmware. It is the recommended firmware for long-term, reliable operation.

This version includes several key fixes and enhancements over the original:
-   **Motor Heat Management:** The stepper motor is automatically disabled after a 2-second idle period to prevent overheating and reduce power consumption.
-   **I2S Stability:** Uses the `I2S_COMM_FORMAT_I2S_MSB` communication format, which is more stable for the INMP441 microphone.
-   **Noise Value Clamping:** The calculated dB(A) value is clamped between a realistic range (30-120 dB) to prevent sensor errors from causing erratic behavior.
-   **Reduced Motor Spam:** The automation logic now checks the curtain's current position before sending a move command, preventing redundant operations.
-   **Increased Task Memory:** The stack size for the audio processing task has been increased from 8KB to 12KB to prevent potential crashes during high load.
-   **Live Serial Debugging:** Prints live dB(A) levels, thresholds, and motor status to the serial monitor every second for easier debugging and calibration.

## Prerequisites

1.  **Arduino IDE / Arduinodroid:** Download and install your preferred development environment.
2.  **ESP32 Board Support:** Follow the official Expressif guide to add ESP32 board support to your IDE: [Installing the ESP32 Board in Arduino IDE](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html). In the Boards Manager, search for and install `esp32`. Select "ESP32 Dev Module" or a board that matches your hardware.
3.  **Install Libraries:** Open your IDE's Library Manager and install the following:
    -   `AccelStepper` by Mike McCauley
    -   `IRremoteESP32` by `bgrg` (Note: The library to install is `IRremoteESP32`, but the header file you must use in your code is `#include <IRremote.h>`)

    **Note on BLE Libraries:** The required Bluetooth Low Energy (BLE) libraries (`BLEDevice.h`, `BLEServer.h`, etc.) and `Preferences.h` are included automatically when you install the ESP32 board support package.

## How to Upload

1.  **Open the `.ino` file:** Open `decibel_defender_esp32_improved.ino` in your IDE.
2.  **Connect Your Board:** Connect your ESP32 to your computer or phone.
3.  **Select Board and Port:** In your IDE, go to **Tools > Board** and select "ESP32 Dev Module" (or your specific board). Then go to **Tools > Port** and select the correct serial port for your device.
4.  **Upload:** Click the "Upload" button.

## How to Connect

After uploading, the device will start advertising as "AcousticCurtain-Module" via Bluetooth. Open the web dashboard and use the "Connect" button to pair with the device. For debugging, you can open the Serial Monitor at a baud rate of **115200**.
