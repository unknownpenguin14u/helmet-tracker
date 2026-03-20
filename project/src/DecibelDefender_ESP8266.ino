/*
  Decibel Defender - Standalone ESP8266 Controller (v6.0 - Limit Switches)

  This version adds support for physical limit switches for automatic homing on
  startup and provides a safety mechanism to prevent motor damage.
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
#define STEP_PIN   D1 // GPIO5
#define DIR_PIN    D2 // GPIO4
#define ENABLE_PIN D5 // GPIO14
#define MIC_PIN    A0

// --- New Limit Switch Pin Definitions ---
#define LIMIT_SWITCH_OPEN_PIN   D6 // GPIO12
#define LIMIT_SWITCH_CLOSE_PIN  D7 // GPIO13

// --- Core Components ---
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiManager wm;

// --- Fixed Motor Configuration ---
const long TOTAL_STEPS = 20000; // Total steps from open to close
const float HOMING_SPEED = 500.0; // Slower speed for homing
const float MAX_SPEED = 2000.0;
const float ACCELERATION = 1000.0;

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
const int NOISE_TRIGGER_DURATION_MS = 3000;


void setup() {
  Serial.begin(115200);
  EEPROM.begin(sizeof(Settings));
  loadSettings();

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // Start with motor disabled

  // --- Initialize Limit Switches ---
  pinMode(LIMIT_SWITCH_OPEN_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_CLOSE_PIN, INPUT_PULLUP);

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

  // --- Homing Sequence on Startup ---
  homeMotor();

  // --- Time & Server Setup ---
  timeClient.begin();
  timeClient.setTimeOffset(28800); // GMT+8 for Asia/Manila
  setupServerRoutes();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  MDNS.update();
  
  // Always check limit switches for safety
  checkLimitSwitches();
  
  stepper.run();

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
    // Always update settings from the request, as the dashboard sends them with every action.
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

  server.on("/", [](){ addCorsHeaders(); server.send(200, "text/plain", "Decibel Defender Standalone is online!"); });
  
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
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  if (currentMinute == 0) { // Check only at the top of the hour
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

  // Noise-based closing
  if (!isNightMode && currentSettings.noiseAutoCloseEnabled) {
    if (noiseLevel > currentSettings.noiseThreshold) {
      if (noiseExceedsThresholdStart == 0) {
        noiseExceedsThresholdStart = now;
      } else if (now - noiseExceedsThresholdStart >= NOISE_TRIGGER_DURATION_MS) {
        if (stepper.currentPosition() != TOTAL_STEPS) {
          moveCurtain("close", true);
        }
        noiseExceedsThresholdStart = 0; // Reset after triggering
      }
    } else {
      noiseExceedsThresholdStart = 0; // Reset if noise drops
    }
  } else {
     noiseExceedsThresholdStart = 0;
  }

  // Noise-based opening
  if (currentSettings.autoOpenEnabled && wasClosedByNoise && noiseLevel <= (currentSettings.noiseThreshold - THRESHOLD_DEADBAND) && stepper.currentPosition() == TOTAL_STEPS) {
    moveCurtain("open", false);
  }
}

// ======================================================
// MOTOR CONTROL & LIMIT SWITCHES
// ======================================================

void homeMotor() {
  Serial.println("Starting homing sequence...");
  digitalWrite(ENABLE_PIN, LOW); // Enable motor
  
  stepper.setMaxSpeed(HOMING_SPEED); // Use slower speed for homing
  stepper.setAcceleration(ACCELERATION / 2);

  // Move towards the open switch
  stepper.moveTo(-TOTAL_STEPS * 2); // Move a long way in the 'open' direction
  
  while (digitalRead(LIMIT_SWITCH_OPEN_PIN) == HIGH) {
    stepper.run();
    yield(); // Allow background processes to run
  }

  Serial.println("Open limit switch hit.");
  stepper.stop();
  stepper.setCurrentPosition(0); // This is our new zero position
  
  // Restore normal operating speed
  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);
  
  digitalWrite(ENABLE_PIN, HIGH); // Disable motor after homing
  Serial.println("Homing complete.");
}

void checkLimitSwitches() {
  // LOW means the switch is pressed
  bool openSwitchPressed = (digitalRead(LIMIT_SWITCH_OPEN_PIN) == LOW);
  bool closeSwitchPressed = (digitalRead(LIMIT_SWITCH_CLOSE_PIN) == LOW);

  if (openSwitchPressed) {
    if (stepper.currentPosition() != 0) {
      Serial.println("! Open limit hit unexpectedly. Resetting position to 0.");
      stepper.stop();
      stepper.setCurrentPosition(0);
    }
  }

  if (closeSwitchPressed) {
    if (stepper.currentPosition() != TOTAL_STEPS) {
      Serial.println("! Close limit hit unexpectedly. Resetting position to max.");
      stepper.stop();
      stepper.setCurrentPosition(TOTAL_STEPS);
    }
  }
}


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
    long targetPos = map(percentage, 0, 100, 0, TOTAL_STEPS);

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
  // Check if EEPROM is empty/invalid
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
  if (EEPROM.commit()) {
    Serial.println("Settings saved to EEPROM.");
  } else {
    Serial.println("ERROR: Failed to save settings to EEPROM.");
  }
}

