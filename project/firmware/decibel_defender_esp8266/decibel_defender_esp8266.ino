#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// --- Pin Definitions ---
#define STEP_PIN D1
#define DIR_PIN D2
#define ENABLE_PIN D5
#define MIC_PIN A0
#define OPEN_SWITCH_PIN D6
#define CLOSE_SWITCH_PIN D7

// --- Firmware Configuration ---
#define USE_LIMIT_SWITCHES false // Set to true if you have limit switches installed

// --- Stepper Configuration ---
#define MOTOR_INTERFACE_TYPE 1
#define INVERT_DIRECTION false // Set to true if your motor runs backward

// Sane defaults for speed and acceleration.
const long max_speed = 400;
const long acceleration = 200;

long totalSteps = 20000; // Default steps, will be calibrated by homing if switches are used

// Create AccelStepper instance
AccelStepper stepper = AccelStepper(MOTOR_INTERFACE_TYPE, STEP_PIN, DIR_PIN);

// --- Web Server & Network Clients ---
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
// Change timezone offset as needed (in seconds). E.g., PST is UTC-8 -> -8 * 3600 = -28800
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); 

// --- State Management ---
// Configuration state (set by dashboard)
struct DeviceConfig {
    int noiseThreshold;
    bool isAutoMode;
    bool isNoiseAutoCloseEnabled;
    bool isSunriseOpenEnabled;
    bool isSunsetCloseEnabled;
    int sunriseHour;
    int sunsetHour;
    int noiseDeadband;
    int noiseTriggerDuration;
};
DeviceConfig config = {500, true, true, true, true, 6, 18, 100, 10}; // Default noise threshold is raw ADC value

// Runtime state (internal)
struct RuntimeState {
    bool wasClosedByNoise;
    bool isNightLock;
};
RuntimeState state = {false, false};

// --- Timing & Counters ---
unsigned long lastNoiseCheckMillis = 0;
unsigned long lastScheduleCheckMillis = 0;
int lastScheduleCheckHour = -1;
int noiseHighCounter = 0;

// --- Forward Declarations ---
void gotoPercentage(int percent);

// --- Motor Control ---
void gotoPercentage(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    long targetPos = map(percent, 0, 100, 0, totalSteps);
    stepper.moveTo(targetPos);
    Serial.print("Moving to ");
    Serial.print(percent);
    Serial.println("%");
}

void homingSequence() {
    if (!USE_LIMIT_SWITCHES) return;

    Serial.println("Starting homing sequence...");
    stepper.setMaxSpeed(200); // Move slowly for homing
    stepper.setAcceleration(100);

    // 1. Move towards close switch
    stepper.moveTo(-100000);
    while (digitalRead(CLOSE_SWITCH_PIN) == HIGH) {
        stepper.run();
    }
    stepper.setCurrentPosition(0);
    Serial.println("Close limit reached. Position set to 0.");
    
    // 2. Move towards open switch to find total steps
    stepper.moveTo(100000);
    while (digitalRead(OPEN_SWITCH_PIN) == HIGH) {
        stepper.run();
    }
    totalSteps = stepper.currentPosition();
    Serial.print("Open limit reached. Total steps calibrated to: ");
    Serial.println(totalSteps);

    // Reset to normal speed
    stepper.setMaxSpeed(max_speed);
    stepper.setAcceleration(acceleration);
    
    // Go back to closed position
    gotoPercentage(0);
}

// --- Noise Sensing ---
int readNoiseLevel() {
    // Provides a raw ADC reading from 0-1023
    return analogRead(MIC_PIN);
}

// --- Web Server Handlers ---
void handleRoot() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "ISO-Module is online.");
}

void handleGoTo() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("percent")) {
        int percent = server.arg("percent").toInt();
        gotoPercentage(percent);
        state.wasClosedByNoise = false;
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing 'percent' parameter");
    }
}

void handleGetConfig() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<256> doc;
    doc["noiseThreshold"] = config.noiseThreshold;
    doc["isAutoMode"] = config.isAutoMode;
    // ... add other config fields ...
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleSetConfig() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("plain") == false) {
        server.send(400, "text/plain", "Body not received");
        return;
    }
    
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));

    config.noiseThreshold = doc["noiseThreshold"] | config.noiseThreshold;
    config.isAutoMode = doc["isAutoMode"] | config.isAutoMode;
    config.isNoiseAutoCloseEnabled = doc["isNoiseAutoCloseEnabled"] | config.isNoiseAutoCloseEnabled;
    config.isSunriseOpenEnabled = doc["isSunriseOpenEnabled"] | config.isSunriseOpenEnabled;
    config.isSunsetCloseEnabled = doc["isSunsetCloseEnabled"] | config.isSunsetCloseEnabled;
    config.sunriseHour = doc["sunriseHour"] | config.sunriseHour;
    config.sunsetHour = doc["sunsetHour"] | config.sunsetHour;

    Serial.println("Configuration updated from dashboard.");
    server.send(200, "text/plain", "OK");
}

void handleResetWifi() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Resetting Wi-Fi and restarting...");
    delay(1000);
    WiFiManager wm;
    wm.resetSettings();
    ESP.reset();
}

void addCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

// --- Main Setup ---
void setup() {
    Serial.begin(115200);
    delay(1000);

    // Limit switch setup
    if (USE_LIMIT_SWITCHES) {
        pinMode(OPEN_SWITCH_PIN, INPUT_PULLUP);
        pinMode(CLOSE_SWITCH_PIN, INPUT_PULLUP);
    }

    stepper.setEnablePin(ENABLE_PIN);
    stepper.setPinsInverted(INVERT_DIRECTION, false, true);
    stepper.setMaxSpeed(max_speed);
    stepper.setAcceleration(acceleration);
    stepper.disableOutputs();

    homingSequence(); // Calibrate if switches are enabled

    // --- Wi-Fi Setup ---
    WiFi.mode(WIFI_STA);
    WiFiManager wm;
    wm.setConfigPortalTimeout(120);

    if (wm.autoConnect("ISO-Module-Setup")) {
        Serial.println("Connected to Wi-Fi.");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin("iso-module", WiFi.localIP())) {
            Serial.println("mDNS responder started.");
        }

        timeClient.begin();

        // --- Web Server Routes ---
        server.on("/", HTTP_GET, handleRoot);
        server.on("/goto", HTTP_GET, handleGoTo);
        server.on("/config", HTTP_GET, handleGetConfig);
        server.on("/config", HTTP_POST, handleSetConfig);
        server.on("/reset-wifi", HTTP_GET, handleResetWifi);
        
        server.on("/", HTTP_OPTIONS, addCorsHeaders);
        server.on("/goto", HTTP_OPTIONS, addCorsHeaders);
        server.on("/config", HTTP_OPTIONS, addCorsHeaders);
        server.on("/reset-wifi", HTTP_OPTIONS, addCorsHeaders);

        server.begin();
        Serial.println("HTTP server started.");
    } else {
        Serial.println("Wi-Fi configuration failed.");
    }
}

// --- Main Loop ---
void loop() {
    server.handleClient();
    MDNS.update();

    if (stepper.run()) {
        stepper.enableOutputs();
    } else {
        stepper.disableOutputs();
    }

    // --- Automation Logic ---
    if (config.isAutoMode && !state.isNightLock) {
        if (millis() - lastNoiseCheckMillis >= 1000) {
            lastNoiseCheckMillis = millis();
            int currentNoise = readNoiseLevel();

            if (config.isNoiseAutoCloseEnabled && currentNoise > config.noiseThreshold) {
                noiseHighCounter++;
                if (noiseHighCounter >= config.noiseTriggerDuration && stepper.currentPosition() > 0) {
                    gotoPercentage(0);
                    state.wasClosedByNoise = true;
                    noiseHighCounter = 0;
                }
            } else {
                noiseHighCounter = 0;
            }

            if (config.isNoiseAutoCloseEnabled && state.wasClosedByNoise && currentNoise < (config.noiseThreshold - config.noiseDeadband)) {
                 if (stepper.currentPosition() == 0) {
                    gotoPercentage(100);
                    state.wasClosedByNoise = false;
                 }
            }
        }
    }

    if (millis() - lastScheduleCheckMillis >= 60000) {
        lastScheduleCheckMillis = millis();
        if (config.isAutoMode && WiFi.status() == WL_CONNECTED) {
            timeClient.update();
            int currentHour = timeClient.getHours();
            
            if (currentHour != lastScheduleCheckHour) {
                if (config.isSunriseOpenEnabled && currentHour == config.sunriseHour) {
                    if (stepper.currentPosition() < totalSteps) {
                        gotoPercentage(100);
                        state.wasClosedByNoise = false;
                        state.isNightLock = false;
                    }
                }
                if (config.isSunsetCloseEnabled && currentHour == config.sunsetHour) {
                    if (stepper.currentPosition() > 0) {
                        gotoPercentage(0);
                        state.wasClosedByNoise = false;
                        state.isNightLock = true;
                    }
                }
                lastScheduleCheckHour = currentHour;
            }
        }
    }
}
