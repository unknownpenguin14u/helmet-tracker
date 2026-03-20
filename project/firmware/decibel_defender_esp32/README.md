# AcousticCurtain-Module - ESP32 Firmware

This folder contains the Arduino firmware for the wireless version of the AcousticCurtain-Module project, designed to run on an **ESP32** board (such as the ESP32 DevKitC). This version is Bluetooth-only.

This firmware uses advanced signal processing (I2S digital audio input) to calculate an accurate dB(A) sound pressure level.

## Features

-   **High-Accuracy Sound Measurement:** Uses the ESP32's I2S peripheral with a digital INMP441 microphone for a clean audio signal, which is then processed with a Fast Fourier Transform (FFT) and A-weighting for an accurate dB(A) reading.
-   **Bluetooth Low Energy (BLE) Control:** Connect directly from the web dashboard to control the curtains. No Wi-Fi or internet needed.
-   **IR Remote Control (Optional):** Control the curtains with a standard infrared remote.
-   **Noise-Responsive Automation:** All noise-based automation is handled directly on the device, ensuring it runs reliably even if the dashboard is closed.
-   **Smooth Motor Control:** Uses `AccelStepper` for smooth acceleration and deceleration, running on a dedicated CPU core to prevent stuttering.
-   **Position Persistence:** Automatically saves the curtain's last position to memory. If the device reboots, it will restore the curtain to where it was.

## Prerequisites

1.  **Arduino IDE / Arduinodroid:** Download and install your preferred development environment.
2.  **ESP32 Board Support:** Follow the official Expressif guide to add ESP32 board support to your IDE: [Installing the ESP32 Board in Arduino IDE](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html). In the Boards Manager, search for and install `esp32`. Select "ESP32 Dev Module" or a board that matches your hardware.
3.  **Install Libraries:** Open your IDE's Library Manager and install the following:
    -   `AccelStepper` by Mike McCauley
    -   `ArduinoFFT` by Enrique Condes (Note: the library is named `arduinoFFT`, but the class in the code is `ArduinoFFT` with a capital 'A')
    -   `IRremoteESP32` by `bgrg` (Note: The library to install is `IRremoteESP32`, but for compatibility reasons, the header file you must use in your code is `#include <IRremote.h>`)

    **Note on BLE Libraries:** The required Bluetooth Low Energy (BLE) libraries (`BLEDevice.h`, `BLEServer.h`, etc.) are included automatically when you install the ESP32 board support package. You do not need to install `ArduinoBLE` or any other BLE library separately. The `Preferences.h` library for saving memory is also built-in.

## How to Upload

1.  **Open the `.ino` file:** Open `decibel_defender_esp32.ino` in your IDE.
2.  **Connect Your Board:** Connect your ESP32 to your computer or phone.
3.  **Select Board and Port:** In your IDE, go to **Tools > Board** and select "ESP32 Dev Module" (or your specific board). Then go to **Tools > Port** and select the correct serial port for your device.
4.  **Upload:** Click the "Upload" button.

## How to Connect

After uploading, the device will start advertising as "AcousticCurtain-Module" via Bluetooth. Open the web dashboard and use the "Connect" button to pair with the device.
