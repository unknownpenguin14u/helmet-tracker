# DIY Hardware Guides

This document outlines the necessary components and wiring instructions to build the AcousticCurtain-Module hardware.

---

## Bill of Materials (Raw Materials)

Here is a list of the components you will need.

#### Core Mechanical Components (Required for all setups)
*   **NEMA 17 Stepper Motor:** A standard motor for DIY CNC and 3D printer projects.
*   **GT2 Timing Belt:** Typically sold by the meter (5 meters is a safe amount for most windows).
*   **GT2 Timing Pulley:** Must match the motor shaft diameter (usually 5mm). 20-tooth is a common size.
*   **GT2 Idler Pulley:** A free-spinning pulley for the opposite end of the belt loop.
*   **Curtain Rod/Rail:** The existing rail you wish to automate.
*   **Mounts & Fasteners:** You will need custom mounts to attach the motor and idler to your wall or window frame. These are often 3D printed.

#### Core Electronics (Required for all setups)
*   **12V 2A+ Power Supply:** A power adapter with a DC barrel jack to power the motor driver.
*   **Jumper Wires:** For connecting components on the breadboard or to the Arduino shield.

---

### Option 1: Wired Controller (Arduino)

This is the simplest setup, controlled directly from a computer via USB.
*   **Arduino Uno R3:** The microcontroller brain.
*   **CNC Shield v3:** An add-on board that makes connecting motor drivers to the Arduino easy.
*   **A4988 or DRV8825 Stepper Driver:** A small module that controls the motor. Must come with a heatsink.
*   **MAX4466 Microphone Amplifier:** A basic analog microphone for sound detection.

### Option 2: Wireless Controller (ESP8266)

*   **ESP8266 NodeMCU V3 (Lolin):** The Wi-Fi enabled microcontroller.
*   **DRV8825 Stepper Driver:** A module to control the motor. A4988 can also be used but DRV8825 is recommended. Must come with a heatsink.
*   **Breadboard:** A 400 or 800-point breadboard for wiring.
*   **MAX4466 Microphone Amplifier:** A basic analog microphone.

### Option 3: Wireless Controller (ESP32) - High-Fidelity Digital Audio

This is the **highly recommended** wireless setup, offering the most accurate sound level measurement by using a digital microphone.
*   **ESP32 DevKitC (or similar):** A powerful Wi-Fi and Bluetooth enabled microcontroller.
*   **DRV8825 Stepper Driver:** A module to control the motor. Must come with a heatsink.
*   **INMP441 Digital Microphone:** (**Recommended**) A high-fidelity digital I2S microphone. This provides a much cleaner signal than analog options.
*   **IR Receiver (TSOP38238 or similar):** Optional, for remote control.
*   **Breadboard:** A 400 or 800-point breadboard for wiring.

### Option 4: Wireless Controller (ESP32) - Basic Analog Audio

This is an alternative ESP32 setup using a more basic microphone. Accuracy will be lower than the INMP441 setup.
*   **ESP32 DevKitC (or similar):** A powerful Wi-Fi and Bluetooth enabled microcontroller.
*   **DRV8825 Stepper Driver:** A module to control the motor. Must come with a heatsink.
*   **MAX4466 Microphone Amplifier:** (**Alternative**) A basic analog microphone.
*   **IR Receiver (TSOP38238 or similar):** Optional, for remote control.
*   **Breadboard:** A 400 or 800-point breadboard for wiring.


---

### Recommended Upgrades

*   **Mechanical Limit Switches (x2):** Highly recommended for reliability on any wireless setup. Simple microswitches with a lever arm.

---

## Guide 1: Arduino Uno + CNC Shield (Wired Control)

This guide explains how to connect the Arduino, CNC shield, stepper motor, and other components to build the physical automatic curtain system.

#### Connection Diagram
*Diagram remains the same.*

#### Step-by-Step Wiring and Assembly
*Instructions remain the same.*

---

## Guide 2: ESP8266 NodeMCU V3 + DRV8825 + MAX4466 (Wireless)

This guide explains how to wire an ESP8266 NodeMCU V3, a DRV8825 stepper driver, and a MAX4466 microphone on a breadboard. This setup is required for wireless control.

#### ESP8266 Wiring Diagram
*Diagram remains the same.*

#### ESP8266 Step-by-Step Wiring
*Instructions remain the same.*

---

## Guide 3: Adding Limit Switches (Optional Upgrade)

This guide explains how to add two mechanical limit switches to the wireless setups for automatic homing and safety.

#### Limit Switch Wiring Diagram (ESP8266 & ESP32)
*Diagrams and instructions remain the same.*

---

## Guide 4: ESP32 + MAX4466 (Basic Analog Control)

This guide provides the specific wiring instructions for using an **ESP32 DevKitC** board with a **MAX4466 Analog Microphone**. This is a basic setup; for higher accuracy, see Guide 5.

#### ESP32 Analog Wiring Diagram
*Diagram and instructions from the previous "Guide 4" are moved here, with the title updated.*

```mermaid
graph TD
    subgraph "Power"
        PowerSupply[12V 2A+ Power Supply] -- "Barrel Jack" --> Breadboard_Power_Rails((+<br>-));
    end

    subgraph "Microcontroller (ESP32 DevKitC)"
        ESP32[ESP32 DevKitC]
        ESP32_3V3[3V3]
        ESP32_GND[GND]
        ESP32_GPIO18[GPIO18 (STEP)]
        ESP32_GPIO5[GPIO5 (DIR)]
        ESP32_GPIO19[GPIO19 (ENABLE)]
        ESP32_GPIO34[GPIO34 (MIC)]

        ESP32 -- "USB Power" --> Your_Computer[Computer or 5V Adapter]
        ESP32 --- ESP32_3V3 & ESP32_GND & ESP32_GPIO18 & ESP32_GPIO5 & ESP32_GPIO19 & ESP32_GPIO34
    end
    
    subgraph "Motor Driver (DRV8825)"
        DRV8825[DRV8825 Module</br>+ Heatsink]
        DRV8825_GND[GND]
        DRV8825_VMOT[VMOT]
        DRV8825_STEP[STEP]
        DRV8825_DIR[DIR]
        DRV8825_ENABLE[~ENABLE]
        DRV8825_RESET[~RESET]
        DRV8825_SLEEP[~SLEEP]
        DRV8825_MOTOR_A[A1 & A2]
        DRV8825_MOTOR_B[B1 & B2]

        DRV8825 --- DRV8825_GND & DRV8825_VMOT & DRV8825_STEP & DRV8825_DIR & DRV8825_ENABLE & DRV8825_RESET & DRV8825_SLEEP & DRV8825_MOTOR_A & DRV8825_MOTOR_B
    end

    subgraph "Sound Sensor & Motor"
        NEMA17[NEMA 17 Motor]
        NEMA17_COIL_A[Coil A]
        NEMA17_COIL_B[Coil B]
        
        Mic[MAX4466 Microphone]
        Mic_VCC[VCC]
        Mic_GND[GND]
        Mic_OUT[OUT]

        NEMA17 --- NEMA17_COIL_A & NEMA17_COIL_B
        Mic --- Mic_VCC & Mic_GND & Mic_OUT
    end
    
    %% Motor Connections
    ESP32_GPIO18 -- "STEP Signal" --> DRV8825_STEP;
    ESP32_GPIO5 -- "DIR Signal" --> DRV8825_DIR;
    ESP32_GPIO19 -- "ENABLE Signal" --> DRV8825_ENABLE;
    
    %% Analog Microphone Connections
    ESP32_3V3 -- "3.3V Power" --> Mic_VCC;
    ESP32_GND -- "Ground" --> Mic_GND;
    ESP32_GPIO34 -- "Analog Signal" --> Mic_OUT;

    %% Power Connections
    Breadboard_Power_Rails -- "12V (+)" --> DRV8825_VMOT;
    Breadboard_Power_Rails -- "GND (-)" --> DRV8825_GND;
    Breadboard_Power_Rails -- "GND (-)" --> ESP32_GND;

    %% Motor Connections
    DRV8825_MOTOR_A -- "Motor Wires" --> NEMA17_COIL_A;
    DRV8825_MOTOR_B -- "Motor Wires" --> NEMA17_COIL_B;

    %% Driver Logic
    linkStyle 9 stroke-width:2px,fill:none,stroke:red;
    DRV8825_RESET -- "Jumper Wire" --> DRV8825_SLEEP;

    %% Styling
    style ESP32 fill:#E7352C,stroke:#fff,color:#fff
    style DRV8825 fill:#A033FF,stroke:#fff,color:#fff
    style NEMA17 fill:#555,stroke:#fff,color:#fff
    style PowerSupply fill:#c22,stroke:#fff,color:#fff
    style Mic fill:#4CAF50,stroke:#fff,color:#fff
```

#### ESP32 Step-by-Step Wiring (Analog)
*Instructions from the previous "Guide 4" are moved here.*

---

## Guide 5: ESP32 + INMP441 (High-Fidelity Digital Control)

This guide provides the wiring for the **recommended** high-accuracy setup, using an **ESP32 DevKitC** with an **INMP441 Digital I2S Microphone**. This method bypasses the ESP32's noisy ADC for a much cleaner audio signal.

#### ESP32 High-Fidelity Wiring Diagram

```mermaid
graph TD
    subgraph "Power"
        PowerSupply[12V 2A+ Power Supply] -- "Barrel Jack" --> Breadboard_Power_Rails((+<br>-));
    end

    subgraph "Microcontroller (ESP32 DevKitC)"
        ESP32[ESP32 DevKitC]
        ESP32_3V3[3V3]
        ESP32_GND[GND]
        ESP32_GPIO18[GPIO18 (STEP)]
        ESP32_GPIO5[GPIO5 (DIR)]
        ESP32_GPIO19[GPIO19 (ENABLE)]
        ESP32_GPIO26[GPIO26 (I2S SCK)]
        ESP32_GPIO25[GPIO25 (I2S WS)]
        ESP32_GPIO33[GPIO33 (I2S SD)]
        ESP32_GPIO27[GPIO27 (IR IN)]

        ESP32 -- "USB Power" --> Your_Computer[Computer or 5V Adapter]
        ESP32 --- ESP32_3V3 & ESP32_GND & ESP32_GPIO18 & ESP32_GPIO5 & ESP32_GPIO19 & ESP32_GPIO26 & ESP32_GPIO25 & ESP32_GPIO33 & ESP32_GPIO27
    end
    
    subgraph "Motor Driver (DRV8825)"
        DRV8825[DRV8825 Module</br>+ Heatsink]
        DRV8825_GND[GND]
        DRV8825_VMOT[VMOT]
        DRV8825_STEP[STEP]
        DRV8825_DIR[DIR]
        DRV8825_ENABLE[~ENABLE]
        DRV8825_RESET[~RESET]
        DRV8825_SLEEP[~SLEEP]

        DRV8825 --- DRV8825_GND & DRV8825_VMOT & DRV8825_STEP & DRV8825_DIR & DRV8825_ENABLE & DRV8825_RESET & DRV8825_SLEEP
    end

    subgraph "Sensors & Motor"
        NEMA17[NEMA 17 Motor]
        Mic[INMP441 Digital Mic]
        Mic_VCC[VCC]
        Mic_GND[GND]
        Mic_SCK[SCK]
        Mic_WS[WS]
        Mic_SD[SD]
        Mic_LR[L/R]
        
        IR[IR Receiver]
        IR_VCC[VCC]
        IR_GND[GND]
        IR_OUT[OUT]

        Mic --- Mic_VCC & Mic_GND & Mic_SCK & Mic_WS & Mic_SD & Mic_LR
        IR --- IR_VCC & IR_GND & IR_OUT
    end
    
    %% Motor Connections
    ESP32_GPIO18 -- "STEP Signal" --> DRV8825_STEP;
    ESP32_GPIO5 -- "DIR Signal" --> DRV8825_DIR;
    ESP32_GPIO19 -- "ENABLE Signal" --> DRV8825_ENABLE;
    
    %% Digital I2S Microphone Connections
    ESP32_GPIO26 -- "Serial Clock" --> Mic_SCK;
    ESP32_GPIO25 -- "Word Select" --> Mic_WS;
    ESP32_GPIO33 -- "Serial Data" --> Mic_SD;
    ESP32_3V3 -- "3.3V Power" --> Mic_VCC;
    ESP32_GND -- "Ground" --> Mic_GND;
    ESP32_GND -- "Select Left Channel" --> Mic_LR;

    %% IR Receiver Connections
    ESP32_GPIO27 -- "IR Signal" --> IR_OUT;
    ESP32_3V3 -- "3.3V Power" --> IR_VCC;
    ESP32_GND -- "Ground" --> IR_GND;

    %% Power Connections
    Breadboard_Power_Rails -- "12V (+)" --> DRV8825_VMOT;
    Breadboard_Power_Rails -- "GND (-)" --> DRV8825_GND;
    Breadboard_Power_Rails -- "GND (-)" --> ESP32_GND;

    %% Motor Connections
    DRV8825 -- "4 Wires" --> NEMA17;

    %% Driver Logic
    linkStyle 10 stroke-width:2px,fill:none,stroke:red;
    DRV8825_RESET -- "Jumper Wire" --> DRV8825_SLEEP;

    %% Styling
    style ESP32 fill:#E7352C,stroke:#fff,color:#fff
    style DRV8825 fill:#A033FF,stroke:#fff,color:#fff
    style NEMA17 fill:#555,stroke:#fff,color:#fff
    style PowerSupply fill:#c22,stroke:#fff,color:#fff
    style Mic fill:#1E90FF,stroke:#fff,color:#fff
    style IR fill:#FF8C00,stroke:#fff,color:#fff
```

#### ESP32 Step-by-Step Wiring (Digital)

1.  **Mount Components:** Place the ESP32 DevKitC and the DRV8825 driver onto a breadboard. **Attach the heatsink** to the DRV8825 chip.

2.  **Power Connections (Critical):**
    *   Connect your **12V power supply** to the breadboard's power rails.
    *   Connect `VMOT` on the DRV8825 to the **12V positive (+)** rail.
    *   Connect `GND` (next to VMOT) on the DRV8825 to the **GND (-)** rail.
    *   Connect a `GND` pin from the **ESP32** to the same **GND (-)** rail. This common ground is essential.

3.  **Motor Control Signal Connections (ESP32 to DRV8255):**
    *   `ESP32 Pin GPIO18` -> `STEP` on DRV8825
    *   `ESP32 Pin GPIO5` -> `DIR` on DRV8825
    *   `ESP32 Pin GPIO19` -> `ENABLE` on DRV8825

4.  **Connect the INMP441 Digital Microphone:**
    *   `ESP32 Pin GPIO26` -> `SCK` on microphone
    *   `ESP32 Pin GPIO25` -> `WS` on microphone
    *   `ESP32 Pin GPIO33` -> `SD` on microphone
    *   `ESP32 Pin 3V3` -> `VCC` on microphone
    *   `ESP32 Pin GND` -> `GND` on microphone
    *   **`ESP32 Pin GND`** -> `L/R` on microphone (Connect L/R to Ground to select the Left audio channel).

5.  **Connect the IR Receiver (Optional):**
    *   `ESP32 Pin GPIO27` -> `OUT` or `Signal` on IR receiver
    *   `ESP32 Pin 3V3` -> `VCC` or `+` on IR receiver
    *   `ESP32 Pin GND` -> `GND` or `-` on IR receiver

6.  **Enable the Driver:**
    *   Use a small jumper wire on the breadboard to link the `~RESET` and `~SLEEP` pins on the DRV8825 together.

7.  **Connect the NEMA 17 Motor & Tune VREF:**
    *   Follow the instructions in Guide 1 (Step 5 & 6) to connect the motor and, most importantly, **tune the driver current (VREF)**.

---

## Troubleshooting Common Issues

*This section remains the same.*

---
## Firmware Installation

*This section remains the same.*
