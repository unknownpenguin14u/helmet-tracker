/*
  ISO-Module - Smart Curtain Controller (ESP32 Firmware)
  
  This firmware runs on an ESP32 DevKitC and provides all the automation
  and control logic for the smart curtain system. It connects to Wi-Fi,
  hosts a web server for commands, and includes a Bluetooth Low Energy (BLE)
  fallback for offline control.

  - Uses WiFiManager for easy on-boarding.
  - Hosts a web server to accept commands from the dashboard.
  - Uses AccelStepper for smooth motor control.
  - Includes a BLE service as a fallback connection method.
  - Handles all automation logic locally on the device.
*/

#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// =================================================================
//                      CONFIGURATION
// =================================================================

// --- HARDWARE PINS (for ESP32 DevKitC) ---
const int STEP_PIN = 18;
const int DIR_PIN = 5;
const int ENABLE_PIN = 19;
const int MIC_PIN = 34;
const int OPEN_SWITCH_PIN = 16;
const int CLOSE_SWITCH_PIN = 17;

// --- MOTOR SETTINGS ---
const int MOTOR_INTERFACE_TYPE = 1;
long totalSteps = 24000; // Default steps for full travel. Calibrated if homing is used.
int motorSpeed = 2000;    // Default max speed in steps/sec
int motorAccel = 1000;    // Default acceleration in steps/sec^2

// --- WI-FI & NETWORK ---
const char* MDNS_HOSTNAME = "iso-module";

// --- TIME & SCHEDULING ---
const long NTP_OFFSET = 8 * 3600; // UTC+8 offset in seconds
const char* NTP_SERVER = "pool.ntp.org";

// --- BLUETOOTH LOW ENERGY (BLE) ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- AUTOMATION LOGIC ---
int noiseThreshold = 75;      // dB(A) level to trigger closing
bool autoMode = true;
bool sunriseOpen = true;
bool sunsetClose = true;
bool noiseAutoClose = true;
int sunriseHour = 6;
int sunsetHour = 18;
bool wasClosedByNoise = false;
bool nightLock = false;

// --- LIMIT SWITCHES ---
#define USE_LIMIT_SWITCHES true // Set to false if you are not using limit switches
const unsigned long HOMING_TIMEOUT_MS = 30000; // 30 seconds timeout for homing

// --- STATE MACHINE ---
enum State {
  STATE_STARTUP,
  STATE_WIFI_CONNECT,
  STATE_NETWORK_INIT,
  STATE_HOMING,
  STATE_READY,
  STATE_ERROR
};
State currentState = STATE_STARTUP;

// =================================================================
//                      GLOBAL OBJECTS
// =================================================================

AccelStepper stepper = AccelStepper(MOTOR_INTERFACE_TYPE, STEP_PIN, DIR_PIN);
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET);
BLECharacteristic *pCharacteristic;

// Forward declaration
void executeCommand(String cmd);


// =================================================================
//              BLUETOOTH & WEB SERVER CALLBACKS
// =================================================================

// --- BLE Characteristic Callback ---
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String cmd = "";
        for (int i = 0; i < value.length(); i++) {
          cmd += value[i];
        }
        executeCommand(cmd);
      }
    }
};

// --- Web Server Handlers ---
void handleRoot() {
  server.send(200, "text/plain", "ISO-Module is online.");
}

void handleCommand() {
  String command = server.arg("command");
  if (command.length() > 0) {
    executeCommand(command);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleGoto() {
    if (server.hasArg("percent")) {
        int percent = server.arg("percent").toInt();
        percent = 100 - percent; // Invert percentage
        long targetPosition = map(percent, 0, 100, 0, totalSteps);
        stepper.moveTo(targetPosition);
        server.send(200, "text/plain", "Moving to " + String(percent) + "%");
    } else {
        server.send(400, "text/plain", "Missing 'percent' parameter");
    }
}


void handleResetWifi() {
    server.send(200, "text/plain", "Resetting Wi-Fi and restarting...");
    delay(1000);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
}

// =================================================================
//                   HELPER FUNCTIONS
// =================================================================

void executeCommand(String cmd) {
  if (cmd.startsWith("goto:")) {
    int percent = cmd.substring(5).toInt();
    long targetPosition = map(percent, 0, 100, 0, totalSteps);
    stepper.moveTo(targetPosition);
  } else if (cmd == "open") {
    stepper.moveTo(0);
  } else if (cmd == "close") {
    stepper.moveTo(totalSteps);
  }
}

// =================================================================
//                        SETUP & LOOP
// =================================================================

void setup() {
  Serial.begin(115200);

  // Configure motor driver pins
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW); // Enable driver
  stepper.setEnablePin(ENABLE_PIN);
  stepper.setPinsInverted(false, false, true);
  stepper.setMaxSpeed(motorSpeed);
  stepper.setAcceleration(motorAccel);

  // Configure limit switch pins
  #if USE_LIMIT_SWITCHES
    pinMode(OPEN_SWITCH_PIN, INPUT_PULLUP);
    pinMode(CLOSE_SWITCH_PIN, INPUT_PULLUP);
  #endif
  
  currentState = STATE_WIFI_CONNECT;
}

void loop() {
  switch (currentState) {
    case STATE_WIFI_CONNECT:
      {
        WiFiManager wifiManager;
        wifiManager.setConnectTimeout(30);
        if (wifiManager.autoConnect("ISO-Module-Setup")) {
          currentState = STATE_NETWORK_INIT;
        } else {
          // If connection fails, restart and try again.
          ESP.restart();
        }
      }
      break;

    case STATE_NETWORK_INIT:
      {
        // --- Initialize BLE ---
        BLEDevice::init("ISO-Module");
        BLEServer *pServer = BLEDevice::createServer();
        BLEService *pService = pServer->createService(SERVICE_UUID);
        pCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID,
                                          BLECharacteristic::PROPERTY_READ |
                                          BLECharacteristic::PROPERTY_WRITE
                                        );
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
        pService->start();
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->start();

        // --- Initialize Web Server & mDNS ---
        server.on("/", handleRoot);
        server.on("/command", handleCommand);
        server.on("/goto", handleGoto);
        server.on("/reset-wifi", handleResetWifi);
        server.begin();
        
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService("http", "tcp", 80);
        }

        // --- Initialize Time Client ---
        timeClient.begin();
        
        currentState = STATE_HOMING;
      }
      break;

    case STATE_HOMING:
      #if USE_LIMIT_SWITCHES
        // See function implementation below
        homeMotor();
      #endif
      currentState = STATE_READY;
      break;

    case STATE_READY:
      timeClient.update();
      server.handleClient();
      stepper.run();

      // Simple power saving: disable motor if it's not moving
      if (!stepper.isRunning()) {
        digitalWrite(ENABLE_PIN, HIGH); // Disable driver
      } else {
        digitalWrite(ENABLE_PIN, LOW); // Enable driver
      }
      break;

    case STATE_ERROR:
      // Halt on error
      delay(1000);
      break;

    default:
      // Should not happen
      break;
  }
}

// =================================================================
//                   MOTOR HOMING SEQUENCE
// =================================================================

#if USE_LIMIT_SWITCHES
void homeMotor() {
  unsigned long startTime = millis();

  // --- Step 1: Move towards CLOSE switch ---
  stepper.setMaxSpeed(motorSpeed / 2); // Home slowly
  stepper.moveTo(-50000); // Move a long way to find the switch

  while (digitalRead(CLOSE_SWITCH_PIN) == HIGH) {
    stepper.run();
    if (millis() - startTime > HOMING_TIMEOUT_MS) {
      // Timeout reached, fallback to default steps
      currentState = STATE_READY;
      return;
    }
  }

  // --- Step 2: Found CLOSE switch, set as position 0 ---
  stepper.setCurrentPosition(0);
  stepper.moveTo(50000); // Prepare to move towards OPEN switch
  
  // --- Step 3: Move towards OPEN switch to find total steps ---
  startTime = millis();
  while (digitalRead(OPEN_SWITCH_PIN) == HIGH) {
    stepper.run();
    if (millis() - startTime > HOMING_TIMEOUT_MS) {
       // Timeout, use default value but at least we are homed to zero.
       currentState = STATE_READY;
       return;
    }
  }

  // --- Step 4: Found OPEN switch, record total steps and finish ---
  totalSteps = stepper.currentPosition();
  stepper.setCurrentPosition(totalSteps); // Set current position to max
  stepper.moveTo(totalSteps); // Move back to the open position
  while(stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  // Homing complete, reset to normal speed
  stepper.setMaxSpeed(motorSpeed);
}
#endif
