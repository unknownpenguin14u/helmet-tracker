/*
  Decibel Defender - Standalone ESP8266 Controller (v5.3 - Automation Fix)

  This version fixes the automation logic to be more reliable, especially for
  time-based events, and correctly implements the 30-second delay for noise triggers.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <AccelStepper.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>

// --- Pin Definitions ---
#define STEP_PIN   D1
#define DIR_PIN    D2
#define ENABLE_PIN D3
#define MIC_PIN    A0

// --- Core Components ---
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiManager wm;

// --- Fixed Motor Configuration ---
const long TOTAL_STEPS = 20000;
const float MAX_SPEED = 890;
const float ACCELERATION = 900;

// --- Settings Struct ---
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
const unsigned long NOISE_TRIGGER_DURATION_MS = 30000; // Require 30 seconds of sustained noise

void setup() {
  Serial.begin(115200);
  EEPROM.begin(sizeof(Settings));
  loadSettings();

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // Keep driver disabled initially

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);
  
  // --- Wi-Fi & mDNS Setup ---
  wm.setAPCallback(configModeCallback);
  if (!wm.autoConnect("DecibelDefender-Setup")) {
    Serial.println("Failed to connect. Restarting...");
    ESP.restart();
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("decibel-defender")) {
    Serial.println("mDNS responder started. Hostname: decibel-defender.local");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // --- Time & Server Setup ---
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  setupServerRoutes();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTP server started");

  // --- Homing on Startup ---
  Serial.println("Homing curtain to open position on startup...");
  digitalWrite(ENABLE_PIN, LOW); // Enable motor driver
  stepper.setCurrentPosition(1); // Set a non-zero start position to ensure movement
  stepper.moveTo(0); // Command motor to move to the 'open' position (0)
}

void loop() {
  server.handleClient();
  MDNS.update();
  stepper.run(); // Call on every loop to process motor steps

  // Disable motor once it reaches its target
  if (!stepper.isRunning()) {
    digitalWrite(ENABLE_PIN, HIGH);
  }

  unsigned long now = millis();
  if (now - lastNoiseCheck > NOISE_CHECK_INTERVAL) {
    lastNoiseCheck = now;
    updateNoiseLevel();
    runAutomationChecks(now);
  }
  
  if (now - lastTimeCheck > TIME_CHECK_INTERVAL) {
    lastTimeCheck = now;
    timeClient.update();
  }
}

// ======================================================
// CALLBACKS & HANDLERS
// ======================================================
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.print("Connect to AP: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void handleRequestStart() {
    addCorsHeaders();
    updateSettingsFromRequest(); 
}

void handleResetWifi() {
  handleRequestStart();
  server.send(200, "text/plain", "OK. Resetting Wi-Fi and restarting.");
  delay(1000);
  wm.resetSettings();
  ESP.restart();
}

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

  server.on("/", [](){ addCorsHeaders(); server.send(200, "text/html", "<html><body><h1>Decibel Defender is Online</h1><p>Control the device via the <a href='http://localhost:3000'>dashboard</a>.</p></body></html>"); });
  
  server.on("/open", [](){ 
    handleRequestStart();
    moveCurtain("open", false); 
    server.send(200, "text/plain", "OK"); 
  });
  
  server.on("/close", [](){ 
    handleRequestStart();
    moveCurtain("close", false); 
    server.send(200, "text/plain", "OK"); 
  });
  
  server.on("/goto", [](){ 
    handleRequestStart();
    if(server.hasArg("percent")) goToPercent(server.arg("percent").toInt()); 
    server.send(200, "text/plain", "OK"); 
  });
  
  server.on("/set", [](){
      handleRequestStart();
      server.send(200, "text/plain", "Settings updated.");
  });

  server.on("/reset-wifi", handleResetWifi);
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
  // Exit if automation is globally disabled
  if (!currentSettings.autoMode) {
    return;
  }
  
  // Prevent rapid open/close cycles
  if (now - lastMovementTimestamp < MOVEMENT_COOLDOWN_MS) {
    return;
  }

  int currentHour = timeClient.getHours();
  
  // Determine if it should be night based on schedule
  bool isCurrentlyNight;
  if (currentSettings.sunriseHour < currentSettings.sunsetHour) {
      // Standard day/night cycle (e.g., sunrise 6, sunset 18)
      isCurrentlyNight = (currentHour >= currentSettings.sunsetHour || currentHour < currentSettings.sunriseHour);
  } else {
      // Inverted cycle where day spans across midnight
      isCurrentlyNight = (currentHour >= currentSettings.sunsetHour && currentHour < currentSettings.sunriseHour);
  }

  // --- Time-Based Automation ---
  // If the state should be "night" but it's currently day, trigger close action.
  if (isCurrentlyNight && !isNightMode) {
    isNightMode = true; // Set state immediately to prevent race conditions
    if (currentSettings.sunsetCloseEnabled && stepper.currentPosition() != TOTAL_STEPS) {
        Serial.println("Automation: Entering Night Mode, closing curtains.");
        moveCurtain("close", false);
        return; // Prioritize time-based action, then exit
    }
  }
  // If the state should be "day" but it's currently night, trigger open action.
  else if (!isCurrentlyNight && isNightMode) {
    isNightMode = false; // Set state immediately
    if (currentSettings.sunriseOpenEnabled && stepper.currentPosition() != 0) {
        Serial.println("Automation: Exiting Night Mode, opening curtains.");
        moveCurtain("open", false);
        return; // Prioritize time-based action, then exit
    }
  }

  // --- Noise-Based Automation ---
  // Noise automation only runs if it's not night mode and is enabled.
  if (isNightMode || !currentSettings.noiseAutoCloseEnabled) {
    noiseExceedsThresholdStart = 0; // Reset noise timer when disabled
    return;
  }
  
  // LOGIC TO CLOSE CURTAINS
  if (noiseLevel > currentSettings.noiseThreshold) {
    if (noiseExceedsThresholdStart == 0) {
      noiseExceedsThresholdStart = now; // Start timer on first detection
      Serial.println("Automation: Noise threshold exceeded, starting 30s timer...");
    } else {
      if (now - noiseExceedsThresholdStart >= NOISE_TRIGGER_DURATION_MS) {
        if (stepper.currentPosition() != TOTAL_STEPS) {
          Serial.println("Automation: Closing due to sustained noise.");
          moveCurtain("close", true);
        }
        noiseExceedsThresholdStart = 0; // Reset timer after triggering
      }
    }
  } else {
    if (noiseExceedsThresholdStart > 0) {
        Serial.println("Automation: Noise dropped below threshold, resetting timer.");
    }
    noiseExceedsThresholdStart = 0; // Reset timer if noise drops
  }

  // LOGIC TO RE-OPEN CURTAINS
  int openThreshold = currentSettings.noiseThreshold - THRESHOLD_DEADBAND;
  if (currentSettings.autoOpenEnabled && wasClosedByNoise && noiseLevel <= openThreshold) {
    if (stepper.currentPosition() == TOTAL_STEPS) {
      Serial.println("Automation: Opening due to quiet environment.");
      moveCurtain("open", false);
    }
  }
}

// ======================================================
// MOTOR CONTROL
// ======================================================
void moveCurtain(String direction, bool isNoiseTrigger) {
    unsigned long now = millis();
    if (now - lastMovementTimestamp < MOVEMENT_COOLDOWN_MS) {
      Serial.println("Movement cooldown active, command ignored.");
      return;
    }
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
    if (now - lastMovementTimestamp < MOVEMENT_COOLDOWN_MS) {
      Serial.println("Movement cooldown active, command ignored.");
      return;
    }
    lastMovementTimestamp = now;

    percentage = constrain(percentage, 0, 100);
    long targetPos = map(percentage, 0, 100, TOTAL_STEPS, 0);

    digitalWrite(ENABLE_PIN, LOW);
    stepper.moveTo(targetPos);
}

// ======================================================
// SETTINGS & EEPROM
// ======================================================
void updateSettingsFromRequest() {
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
  }
}

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

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
