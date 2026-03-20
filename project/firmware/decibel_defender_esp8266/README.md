# AcousticCurtain-Module - ESP8266 Firmware (Wi-Fi Version)

This folder contains the Arduino firmware for a Wi-Fi-only version of the AcousticCurtain-Module project, designed to run on an **ESP8266 NodeMCU V3** board.

**Note:** This version **does not use Bluetooth**. It requires a Wi-Fi connection to be controlled from the dashboard via a local web server. The main project has been simplified to be Bluetooth-only, so this firmware is for users who prefer a Wi-Fi based setup.

## Features

-   **Wi-Fi Connectivity:** Uses `WiFiManager` to create a captive portal for easy, one-time setup of your home Wi-Fi network. No hardcoded credentials needed.
-   **Web Server:** Hosts a local web server that listens for commands from the main dashboard application.
-   **mDNS Service Discovery:** Broadcasts its presence on the network as `acousticcurtain.local`, so you don't need to find its IP address.
-   **Full Automation Logic:** All noise-based and time-based automation is handled directly on the device.
-   **Smooth Motor Control:** Uses `AccelStepper` for smooth acceleration and deceleration of the curtain motor.
-   **NTP Time Sync:** Automatically fetches the current time from the internet to keep schedules accurate.
-   **Limit Switch Support (Optional):** Includes an automatic homing sequence to calibrate the curtain's open and close positions if limit switches are installed.

## Prerequisites

1.  **Arduino IDE:** Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
2.  **ESP8266 Board Support:** Follow [this guide](https://arduino-esp8266.readthedocs.io/en/latest/installing.html) to add ESP8266 board support to your Arduino IDE. Select "NodeMCU 1.0 (ESP-12E Module)" in the Boards Manager.
3.  **Install Libraries:** Open the Arduino IDE and go to **Sketch > Include Library > Manage Libraries...**. Install the following libraries:
    -   `WiFiManager` by tzapu
    -   `AccelStepper` by Mike McCauley
    -   `NTPClient` by Fabrice Weinberg
    -   `ArduinoJson` by Benoit Blanchon (version 6 is recommended)

## How to Upload

1.  **Open the `.ino` file:** Open `decibel_defender_esp8266.ino` in the Arduino IDE.
2.  **Configure (if needed):**
    -   If you are **not** using limit switches, change `#define USE_LIMIT_SWITCHES true` to `false` and manually set the `totalSteps` variable after measuring your curtain's travel distance.
    -   Adjust the `timeClient` timezone offset if you are not in UTC+8.
3.  **Connect Your Board:** Connect your ESP8266 NodeMCU to your computer via USB.
4.  **Select Board and Port:** In the Arduino IDE, go to **Tools > Board** and select "NodeMCU 1.0 (ESP-12E Module)". Then go to **Tools > Port** and select the correct serial port for your device.
5.  **Upload:** Click the "Upload" button (the right-pointing arrow).

## First-Time Setup (Wi-Fi)

1.  After uploading, the ESP8266 will create a Wi-Fi access point named **"AcousticCurtain-Setup"**.
2.  Connect to this network with your phone or computer. A captive portal should automatically open.
3.  Click "Configure WiFi," select your home network, enter the password, and click "Save."
4.  The device will restart and connect to your home network. You can now access the dashboard and connect to it using the `http://acousticcurtain.local` address.
