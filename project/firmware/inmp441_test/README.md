# INMP441 Test Firmware

This is a simple diagnostic sketch to verify that your INMP441 I2S digital microphone is wired correctly and functioning.

It initializes the I2S peripheral on the ESP32 and reads the raw audio samples, printing the average magnitude to the Serial Monitor.

## Wiring

This test requires only the ESP32 and the INMP441 microphone.

#### Wiring Diagram

```mermaid
graph TD
    subgraph "Microcontroller (ESP32 DevKitC)"
        ESP32[ESP32 DevKitC]
        ESP32_3V3[3V3]
        ESP32_GND[GND]
        ESP32_GPIO14[GPIO14 (I2S SCK)]
        ESP32_GPIO15[GPIO15 (I2S WS)]
        ESP32_GPIO33[GPIO33 (I2S SD)]

        ESP32 -- "USB Power" --> Your_Computer[Computer]
        ESP32 --- ESP32_3V3 & ESP32_GND & ESP32_GPIO14 & ESP32_GPIO15 & ESP32_GPIO33
    end
    
    subgraph "Sound Sensor (Digital)"
        Mic[INMP441 Digital Mic]
        Mic_VCC[VCC]
        Mic_GND[GND]
        Mic_SCK[SCK]
        Mic_WS[WS]
        Mic_SD[SD]
        Mic_LR[L/R]

        Mic --- Mic_VCC & Mic_GND & Mic_SCK & Mic_WS & Mic_SD & Mic_LR
    end
    
    %% Digital I2S Microphone Connections
    ESP32_GPIO14 -- "Serial Clock" --> Mic_SCK;
    ESP32_GPIO15 -- "Word Select" --> Mic_WS;
    ESP32_GPIO33 -- "Serial Data" --> Mic_SD;
    ESP32_3V3 -- "3.3V Power" --> Mic_VCC;
    ESP32_GND -- "Ground" --> Mic_GND;
    ESP32_GND -- "Select Left Channel" --> Mic_LR;

    %% Styling
    style ESP32 fill:#E7352C,stroke:#fff,color:#fff
    style Mic fill:#1E90FF,stroke:#fff,color:#fff
```

#### Step-by-Step

1.  **Mount Components:** Place the ESP32 DevKitC onto a breadboard.
2.  **Connect the INMP441 Digital Microphone:**
    *   `ESP32 Pin GPIO14` -> `SCK` on microphone
    *   `ESP32 Pin GPIO15` -> `WS` on microphone
    *   `ESP32 Pin GPIO33` -> `SD` on microphone
    *   `ESP32 Pin 3V3` -> `VCC` on microphone
    *   `ESP32 Pin GND` -> `GND` on microphone
    *   **`ESP32 Pin GND`** -> `L/R` on microphone (Connect L/R to Ground to select the Left audio channel).
3.  **Power the ESP32:** Connect the ESP32 to your computer via USB.


## How to Use

1.  Wire the microphone according to the diagram above.
2.  Open `inmp441_test.ino` in the Arduino IDE.
3.  Upload this sketch to your ESP32.
4.  Open the Arduino IDE's Serial Monitor (or any serial terminal).
5.  Set the baud rate to **115200**.

## Expected Output

If the microphone is working correctly, you will see a continuous stream of non-zero integer values being printed to the serial monitor. The values will fluctuate as the noise level in the room changes. A quiet room will produce small numbers, while clapping or talking will cause large numbers to appear.

```
--- INMP441 FIX TEST START ---
Mic Ready...
154
123
289
1567
8734
...
```

## Troubleshooting

*   **If you see only `0`:** This usually means the `SD` (Serial Data) pin is not connected correctly. Check the wire between `ESP32 Pin GPIO33` and the `SD` pin on the microphone.
*   **If you see only `4294967295` (which is the same as `-1`):** This often indicates a problem with the clock (`SCK`) or word select (`WS`) pins. The ESP32 is not receiving the timing signals it needs to read the data. Double-check your wiring for `SCK` and `WS`.
*   **If you get a "Failed to install I2S driver" error:** This is a software problem. Make sure you have the correct ESP32 board support package installed in your Arduino IDE.
