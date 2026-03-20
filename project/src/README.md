
# AcousticCurtain-Module: DIY Smart Curtain

AcousticCurtain-Module is a DIY home automation project that creates an intelligent, noise-responsive curtain system. Using a web-based dashboard, you can control your curtains and set a threshold. When the ambient noise exceeds your limit, the system automatically closes the curtains to help dampen the sound. When it's quiet again, they can automatically reopen.

This project combines a modern web interface with physical hardware, making it a great example of a full-stack IoT application.

## Core Functions

### Automation
*   **Noise-Responsive Control:** Automatically closes the curtains when the ambient noise level surpasses a user-defined threshold for a sustained period.
*   **Smart Re-opening:** Automatically re-opens the curtains when the noise level falls significantly below the threshold (a 15 dB deadband), preventing erratic open/close cycles. This only occurs if the curtains were last closed by a noise event.

### Manual Control
*   **Direct Position Control:** Use "Open" and "Close" buttons for full movement or a slider for precise positioning.

### Hardware Integration & Calibration
*   **Bluetooth Connectivity:** The system uses a direct Bluetooth connection for maximum reliability.
*   **Motor Calibration:** The firmware allows you to define the total number of steps the motor needs to travel to move the curtains from fully open to fully closed.

---

## Connectivity

This project uses a simple and robust connection system.

### Bluetooth Connection
This is the standard way to use the dashboard.
*   **How it works:** The web browser uses the **Web Bluetooth API** to connect directly to the ESP32.
*   **Requirement:** Your computer/phone must have Bluetooth and be physically near the curtain controller.
*   **Note:** This feature is only available on the **ESP32** version of the firmware, as the ESP8266 does not have Bluetooth hardware.

---

## How to Use the Dashboard (and Access it Offline)

The dashboard is designed to work fully offline, but it requires a quick, one-time setup to enable this feature. Here’s how you do it:

### Step 1: Install the Dashboard as an App

To use the dashboard without an internet connection, you first need to **install it** on your phone or computer. This turns the website into an app on your device.

*   **On a Computer (Chrome or Edge):**
    1.  Open the dashboard in your browser.
    2.  Look for an **install icon** in the address bar (it might look like a computer with a down arrow) and click it.
    3.  Confirm the installation.

*   **On an Android Phone (Chrome):**
    1.  Open the dashboard in Chrome.
    2.  Tap the three-dot menu and select **"Install app"** or **"Add to Home screen"**.

An app icon will now be on your desktop or phone's home screen. **From now on, use this icon to launch the dashboard.**

### Step 2: Using the App Offline

Once installed, the app will always open, even if you have no Wi-Fi or internet connection.

*   **To control the curtains offline**, open the installed app and use the **"Connect"** button. This allows the dashboard to talk directly to the hardware without needing any network.

---

**What if I don't install it?**

If you just use the website in a browser tab and you lose your internet connection, you will be redirected to the "You are Offline" page. You won't be able to control the curtains until your connection is back. **This is why installing the app is the recommended way to use the dashboard.**

---

## How to Run the Application

This project uses a direct browser-to-hardware connection via the Web Serial API. You only need to run the main Next.js web application.

1.  **Open a terminal** and make sure you are in the root directory of the project.
2.  **Install dependencies** (you only need to do this once):
    ```bash
    npm install
    ```
3.  **Start the web server:**
    ```bash
    npm run dev
    ```
4.  Open your web browser and navigate to **http://localhost:3000** (or the address shown in the terminal).

---

## DIY Hardware Guides

For instructions on how to build the physical curtain system, including wiring diagrams for the Arduino, ESP8266, and optional limit switches, please see the dedicated hardware guide in the root of the project: `HARDWARE_GUIDES.md`.
