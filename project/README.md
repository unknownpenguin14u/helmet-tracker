# AcousticCurtain-Module: DIY Smart Curtain

AcousticCurtain-Module is a DIY home automation project that creates an intelligent, noise-responsive curtain system. Using a web-based dashboard, you can monitor live ambient noise levels and set a threshold. When the noise exceeds your limit, the system automatically closes the curtains to help dampen the sound. When it's quiet again, they can automatically reopen.

This project combines a modern web interface (built as a Progressive Web App for offline use) with physical hardware, making it a great example of a full-stack IoT application.

## Core Functions

### Automation
*   **Noise-Responsive Control:** Automatically closes the curtains when the ambient noise level surpasses a user-defined threshold for a sustained period.
*   **Smart Re-opening:** Automatically re-opens the curtains when the noise level falls significantly below the threshold (a 15 dB deadband), preventing erratic open/close cycles. This only occurs if the curtains were last closed by a noise event.

### Manual Control & Monitoring
*   **Direct Position Control:** Use "Open" and "Close" buttons for full movement, a slider for precise positioning, or nudge buttons for fine-tuning.
*   **Speed Adjustment:** Tune the motor's speed and acceleration directly from the dashboard for smoother or faster operation.

### Hardware Integration & Calibration
*   **Bluetooth Connectivity:** The system uses a direct Bluetooth connection for maximum reliability. The dashboard can connect directly to the ESP32 using Web Bluetooth.
*   **Motor Calibration:** A settings panel allows you to define the total number of steps the motor needs to travel to move the curtains from fully open to fully closed.

---

## Tech Stack & Core Dependencies

This project is built with a modern web stack. Here are the key technologies and libraries used to create the dashboard:

*   **Framework:** [Next.js](https://nextjs.org/) (v15) with the App Router.
*   **Language:** [TypeScript](https://www.typescriptlang.org/).
*   **Styling:** [Tailwind CSS](https://tailwindcss.com/) for utility-first styling.
*   **UI Components:** [ShadCN/UI](https://ui.shadcn.com/), which uses Radix UI primitives for accessible components and Lucide for icons.
*   **State Management:** React Hooks and Context API for local and global state.
*   **Forms:** [React Hook Form](https://react-hook-form.com/) with [Zod](https://zod.dev/) for validation.
*   **Offline & PWA:** The dashboard is a Progressive Web App (PWA) enabled by `@ducanh2912/next-pwa`, allowing it to be installed and used offline.
*   **Hardware Communication:**
    *   **Bluetooth:** The [Web Bluetooth API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API) for direct, offline communication.
*   **Generative AI:** [Genkit](https://firebase.google.com/docs/genkit) for integrating AI features.

### All Dependencies

For a complete list of all packages, you can refer to the `package.json` file. The main dependencies include:

*   `next`, `react`, `react-dom`
*   `tailwindcss`, `autoprefixer`, `postcss`
*   `@radix-ui/*` (for UI primitives)
*   `lucide-react` (for icons)
*   `react-hook-form`, `zod`
*   `@ducanh2912/next-pwa`
*   `genkit`, `@genkit-ai/google-genai`
*   `firebase`

---

## Theoretical Framework

The AcousticCurtain-Module project is built upon several key engineering and design principles that define its architecture and behavior.

### 1. System Architecture: Bluetooth Low Energy (BLE)

The project is designed for resilience and simplicity, ensuring you never lose control of your curtains.

*   **Primary Connection: Bluetooth Low Energy (BLE):** The dashboard connects directly to the ESP32 using Web Bluetooth. This provides a robust, offline control method that doesn't rely on any network infrastructure, only that your computer/phone is near the device.
*   **Custom UI over Platform-Specific Tools:** While platforms like **Arduino IoT Cloud** or **Blynk** offer faster ways to create simple dashboards, this project intentionally uses a custom Next.js web application. This approach provides unlimited flexibility in UI/UX design, allowing for a polished, professional, and highly user-friendly interface.

### 2. Control Systems Theory: A Hybrid Approach

The system employs a hybrid control loop model to manage the curtain's state.

*   **Open-Loop Control (for Manual Commands):** When a user clicks "Open," "Close," or sets the slider to a specific percentage, the system operates in an open loop. It sends a command to the motor (`stepper.moveTo(position)`) and trusts that the motor will reach the target without needing to verify the final physical position. This is efficient and sufficient for this application, as stepper motors are known for their precision.
*   **Closed-Loop Control (for Automation):** The noise-responsive automation acts as a closed-loop system.
    *   **Sensor:** The MAX4466 microphone continuously measures the ambient noise (the process variable).
    *   **Controller:** The ESP32 firmware compares the measured noise level against the user-defined `noiseThreshold` (the setpoint).
    *   **Actuator:** If the noise exceeds the setpoint for a given duration, the controller triggers the stepper motor to close the curtains, thereby influencing the environment. The "deadband" for re-opening prevents oscillation, a common issue in control systems where the controller over-corrects.

### 3. Human-Computer Interaction (HCI): Simplicity and Direct Manipulation

The dashboard design prioritizes clarity and an intuitive user experience.

*   **Principle of Direct Manipulation:** The user can directly manipulate the primary objects of the system. The curtain position is controlled via a slider that visually maps to its physical state, and automation rules are toggled with simple switches.
*   **System Feedback:** The dashboard provides immediate and continuous feedback. The connection status is always visible, assuring the user that the system is operating as expected.

---

## How to Use the Dashboard (and Access it Offline)

That's an excellent question. The dashboard is designed to work fully offline, but it requires a quick, one-time setup to enable this feature. Here’s how you do it:

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

For instructions on how to build the physical curtain system, including wiring diagrams and firmware setup, please see the dedicated hardware guide:

**[Click here to view the Hardware Guides](./HARDWARE_GUIDES.md)**
