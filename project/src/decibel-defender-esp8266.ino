
/*
  Decibel Defender - Standalone ESP8266 Controller (v5 - Final with WiFi Reset)

  This sketch includes the final features:
  - WiFiManager for plug-and-play setup.
  - A /reset-wifi endpoint to allow changing networks from the dashboard.
  - All automation logic and settings are managed on the device.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <AccelStepper.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <WiFiManager.h> // Include the WiFiManager library

// --- Pin Definitions ---
#define STEP_PIN   D1  // GPIO5
#define DIR_PIN    D2  // GPIO4
#define ENABLE_PIN D5  // GPIO14 (Active LOW)
#define MIC_PIN    A0  // Analog pin for MAX4466

// --- Core Components ---
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiManager wm;

// --- Fixed Motor Configuration ---
const long TOTAL_STEPS = 20000;
const float MAX_SPEED = 2000.0;
const float ACCELERATION = 1000.0;

// --- Settings Struct & EEPROM ---
struct Settings {
  int noiseThreshold;
  bool autoMode;
  bool sunriseOpenEnabled;
  bool sunsetCloseEnabled;
  int sunriseHour;
  int sunsetHour;
  bool autoOpenEnabled;
  bool noiseAutoCloseEnabled;
};

Settings currentSettings;

// --- Global State Variables ---
int noiseLevel = 40;
bool wasClosedByNoise = false;
bool isNightMode = false;

unsigned long lastNoiseCheck = 0;
unsigned long lastTimeCheck = 0;
unsigned long lastMovementTimestamp = 0;
unsigned long noiseExceedsThresholdStart = 0;

const long NOISE_CHECK_INTERVAL = 1000;
const long TIME_CHECK_INTERVAL = 60000;
const int THRESHOLD_DEADBAND = 15;
const int MOVEMENT_COOLDOWN_MS = 10000;
const int NOISE_TRIGGER_DURATION_MS = 3000;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(sizeof(Settings));
  loadSettings();

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  wm.setAPCallback(configModeCallback);
  
  Serial.println("\nStarting Wi-Fi Manager...");
  if (!wm.autoConnect("DecibelDefender-Setup")) {
    Serial.println("Failed to connect and hit timeout. Restarting...");
    ESP.restart();
  }
  
  Serial.println("\nWiFi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(28800); // PHT is UTC+8

  setupServerRoutes();
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  if (stepper.isRunning()) {
    stepper.run();
  } else {
    digitalWrite(ENABLE_PIN, HIGH);
  }

  unsigned long now = millis();
  if (now - lastNoiseCheck > NOISE_CHECK_INTERVAL) {
    lastNoiseCheck = now;
    updateNoiseLevel();
    if (currentSettings.autoMode) {
      runAutomationChecks(now);
    }
  }
  
  if (now - lastTimeCheck > TIME_CHECK_INTERVAL) {
    lastTimeCheck = now;
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.update();
    }
  }
}

// ======================================================
// WIFI MANAGER CALLBACK & RESET
// ======================================================
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.print("Connect to AP: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void handleResetWifi() {
  addCorsHeaders();
  server.send(200, "text/plain", "OK. Resetting Wi-Fi and restarting.");
  delay(1000); // Give the server time to send the response
  wm.resetSettings();
  ESP.restart();
}


// ======================================================
// AUTOMATION LOGIC
// ======================================================
void updateNoiseLevel() {
  int sensorValue = analogRead(MIC_PIN);
  noiseLevel = map(sensorValue, 0, 1023, 30, 120); 
  noiseLevel = constrain(noiseLevel, 30, 120);
}

void runAutomationChecks(unsigned long now) {
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  if (currentMinute == 0) {
    if (currentSettings.sunsetCloseEnabled && currentHour == currentSettings.sunsetHour) {
      if (stepper.currentPosition() != TOTAL_STEPS) {
        moveCurtain("close", false);
        isNightMode = true;
      }
    }
    else if (currentSettings.sunriseOpenEnabled && currentHour == currentSettings.sunriseHour) {
      if (stepper.currentPosition() != 0) {
        moveCurtain("open", false);
        isNightMode = false;
      }
    }
  }

  if (!isNightMode && currentSettings.noiseAutoCloseEnabled) {
    if (noiseLevel > currentSettings.noiseThreshold) {
      if (noiseExceedsThresholdStart == 0) {
        noiseExceedsThresholdStart = now;
      } else if (now - noiseExceedsThresholdStart >= NOISE_TRIGGER_DURATION_MS) {
        if (stepper.currentPosition() != TOTAL_STEPS) {
          moveCurtain("close", true);
        }
        noiseExceedsThresholdStart = 0;
      }
    } else {
      noiseExceedsThresholdStart = 0;
    }
  } else {
     noiseExceedsThresholdStart = 0;
  }

  if (currentSettings.autoOpenEnabled && wasClosedByNoise && noiseLevel <= (currentSettings.noiseThreshold - THRESHOLD_DEADBAND) && stepper.currentPosition() == TOTAL_STEPS) {
    moveCurtain("open", false);
  }
}

// ======================================================
// MOTOR CONTROL
// ======================================================
void moveCurtain(String direction, bool isNoiseTrigger) {
    unsigned long now = millis();
    if (now - lastMovementTimestamp < MOVEMENT_COOLDOWN_MS) return;
    lastMovementTimestamp = now;

    digitalWrite(ENABLE_PIN, LOW);

    if (direction == "open") {
        stepper.moveTo(0);
        wasClosedByNoise = false;
    } else if (direction == "close") {
        stepper.moveTo(TOTAL_STEPS);
        if (isNoiseTrigger) wasClosedByNoise = true;
    }
}

void goToPercent(int percentage) {
    unsigned long now = millis();
    if (now - lastMovementTimestamp < MOVEMENT_COOLDOWN_MS) return;
    lastMovementTimestamp = now;

    percentage = constrain(percentage, 0, 100);
    long targetPos = map(percentage, 100, 0, 0, TOTAL_STEPS);

    digitalWrite(ENABLE_PIN, LOW);
    stepper.moveTo(targetPos);
}

// ======================================================
// WEB SERVER HANDLERS
// ======================================================
void setupServerRoutes() {
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Max-Age", "10000");
      server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
      server.send(204);
    } else {
      addCorsHeaders();
      server.send(404, "text/plain", "Not Found");
    }
  });
  
  server.on("/", [](){ addCorsHeaders(); server.send(200, "text/plain", "Decibel Defender Standalone is online!"); });
  server.on("/open", [](){ addCorsHeaders(); moveCurtain("open", false); server.send(200, "text/plain", "OK"); });
  server.on("/close", [](){ addCorsHeaders(); moveCurtain("close", false); server.send(200, "text/plain", "OK"); });
  server.on("/goto", [](){ addCorsHeaders(); if(server.hasArg("percent")) goToPercent(server.arg("percent").toInt()); server.send(200, "text/plain", "OK"); });
  server.on("/set", handleSetSettings);
  server.on("/reset-wifi", handleResetWifi); // New route for resetting Wi-Fi
}

void handleSetSettings() {
  addCorsHeaders();
  bool requiresSave = false;

  if (server.hasArg("noiseThreshold")) { currentSettings.noiseThreshold = server.arg("noiseThreshold").toInt(); requiresSave = true; }
  if (server.hasArg("autoMode")) { currentSettings.autoMode = server.arg("autoMode") == "true"; requiresSave = true; }
  if (server.hasArg("sunriseOpen")) { currentSettings.sunriseOpenEnabled = server.arg("sunriseOpen") == "true"; requiresSave = true; }
  if (server.hasArg("sunsetClose")) { currentSettings.sunsetCloseEnabled = server.arg("sunsetClose") == "true"; requiresSave = true; }
  if (server.hasArg("sunriseHour")) { currentSettings.sunriseHour = server.arg("sunriseHour").toInt(); requiresSave = true; }
  if (server.hasArg("sunsetHour")) { currentSettings.sunsetHour = server.arg("sunsetHour").toInt(); requiresSave = true; }
  if (server.hasArg("autoOpen")) { currentSettings.autoOpenEnabled = server.arg("autoOpen") == "true"; requiresSave = true; }
  if (server.hasArg("noiseAutoClose")) { currentSettings.noiseAutoCloseEnabled = server.arg("noiseAutoClose") == "true"; requiresSave = true; }
  
  if (requiresSave) {
    saveSettings();
    server.send(200, "text/plain", "Settings updated and saved.");
  } else {
    server.send(400, "text/plain", "No valid settings provided.");
  }
}

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

// ======================================================
// EEPROM (MEMORY) MANAGEMENT
// ======================================================
void loadSettings() {
  EEPROM.get(0, currentSettings);
  if (currentSettings.noiseThreshold < 40 || currentSettings.noiseThreshold > 120 || EEPROM.read(0) == 0xFF) {
    Serial.println("No valid settings found or EEPROM is empty. Loading defaults.");
    currentSettings.noiseThreshold = 75;
    currentSettings.autoMode = true;
    currentSettings.sunriseOpenEnabled = true;
    currentSettings.sunsetCloseEnabled = true;
    currentSettings.sunriseHour = 6;
    currentSettings.sunsetHour = 18;
    currentSettings.autoOpenEnabled = true;
    currentSettings.noiseAutoCloseEnabled = true;
    saveSettings();
  }
  Serial.println("Settings loaded from EEPROM.");
}

void saveSettings() {
  EEPROM.put(0, currentSettings);
  EEPROM.commit();
  Serial.println("Settings saved to EEPROM.");
}
