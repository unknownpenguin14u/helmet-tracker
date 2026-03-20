#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <AccelStepper.h>
#include <Preferences.h>
#include "driver/i2s.h"
#include <math.h>
#include <IRremote.h>

// ---------------- PIN DEFINITIONS ----------------
#define I2S_WS 25
#define I2S_SD 33
#define I2S_SCK 26

#define STEPPER_DIR_PIN 5
#define STEPPER_STEP_PIN 18
#define STEPPER_ENABLE_PIN 19

#define IR_RECEIVE_PIN 27

// ---------------- I2S CONFIG ----------------
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 44100
#define BUFFER_LEN 512

// ---------------- MOTOR DEFAULTS ----------------
#define DEFAULT_MOTOR_SPEED 890.0
#define DEFAULT_MOTOR_ACCEL 990.0

// ---------------- SPL CALIBRATION ----------------
#define MIC_REF_DB 94.0
#define MIC_REF_DBFS -26.0

// ---------------- TIME WEIGHTING ----------------
#define TIME_WEIGHTING_FAST 0
#define TIME_WEIGHTING_SLOW 1
#define TIME_WEIGHTING_MODE TIME_WEIGHTING_FAST

// ---------------- BLE UUIDS ----------------
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define COMMAND_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define NOISE_CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

// ---------------- OBJECTS ----------------
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN);
Preferences preferences;
BLECharacteristic *pNoiseCharacteristic;
TaskHandle_t audioTaskHandle = NULL;

// ---------------- GLOBAL STATE ----------------
long totalSteps;
float motorSpeed;
float motorAccel;

bool deviceConnected = false;

bool isAutoMode = true;
bool isNoiseAutoCloseEnabled = true;
int closeThreshold = 75;
bool wasClosedByNoise = false;

unsigned long lastNoiseExceededTime = 0;
unsigned long lastNoiseDropTime = 0;

const unsigned long noiseDebounceDuration = 5000;
const unsigned long reopenDebounceDuration = 3000;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
float smoothedDB = 0;

volatile long commandedPosition = -1;
volatile bool commandIsFromAutomation = false;

// ---------------- A-WEIGHTING FILTER ----------------
float a0 = 0.2557411;
float a1 = -0.5114822;
float a2 = 0.2557411;
float b1 = -1.6906593;
float b2 = 0.7324808;
float aw_x1 = 0, aw_x2 = 0, aw_y1 = 0, aw_y2 = 0;

float AWeight(float input) {
  float output = a0 * input + a1 * aw_x1 + a2 * aw_x2 - b1 * aw_y1 - b2 * aw_y2;
  aw_x2 = aw_x1;
  aw_x1 = input;
  aw_y2 = aw_y1;
  aw_y1 = output;
  return output;
}

// ---------------- BLE CALLBACKS ----------------
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device Connected.");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device Disconnected. Restarting advertising...");
    // It's good practice to restart advertising on disconnect to allow reconnection.
    pServer->startAdvertising();
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String cmd = pCharacteristic->getValue();
    if (cmd.length() == 0) return;

    if (cmd.startsWith("goto:")) {
      int percentage = constrain(cmd.substring(5).toInt(), 0, 100);
      commandedPosition = map(percentage, 0, 100, totalSteps, 0);
      commandIsFromAutomation = false;
    }
    else if (cmd.startsWith("config:")) {
      String configStr = cmd.substring(7);
      isAutoMode = configStr.indexOf("auto=1") != -1;
      isNoiseAutoCloseEnabled = configStr.indexOf("noise=1") != -1;

      int threshIdx = configStr.indexOf("thresh=");
      if (threshIdx != -1)
        closeThreshold = configStr.substring(threshIdx + 7).toInt();
    }
    else if (cmd == "restart") {
      ESP.restart();
    }
  }
};

// ---------------- I2S SETUP ----------------
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

// ---------------- AUDIO TASK ----------------
void audioTask(void *parameter) {
  int32_t i2sBuffer[BUFFER_LEN];
  static float dcOffset = 0;
  static unsigned long lastNotify = 0;

  while (true) {
    size_t bytesRead;
    i2s_read(I2S_PORT, &i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);

    int samples = bytesRead / 4;
    double sum = 0;

    for (int i = 0; i < samples; i++) {
      int32_t sample = i2sBuffer[i] >> 8;
      float normalized = sample / 8388608.0f;

      dcOffset = 0.999f * dcOffset + 0.001f * normalized;
      normalized -= dcOffset;

      float weighted = AWeight(normalized);
      sum += weighted * weighted;
    }

    float rms = sqrt(sum / samples);
    if (rms < 0.000001f) rms = 0.000001f;

    float dBFS = 20.0f * log10(rms);
    float dBA = dBFS - MIC_REF_DBFS + MIC_REF_DB;

    float tau = (TIME_WEIGHTING_MODE == TIME_WEIGHTING_FAST) ? 0.125f : 1.0f;
    float dt = (float)BUFFER_LEN / SAMPLE_RATE;
    float alpha = 1.0f - expf(-dt / tau);

    portENTER_CRITICAL(&mux);
    smoothedDB = smoothedDB + alpha * (dBA - smoothedDB);
    portEXIT_CRITICAL(&mux);

    if (deviceConnected && millis() - lastNotify > 200) {
      float dbValue;
      portENTER_CRITICAL(&mux);
      dbValue = smoothedDB;
      portEXIT_CRITICAL(&mux);
      
      pNoiseCharacteristic->setValue(dbValue);
      pNoiseCharacteristic->notify();
      lastNotify = millis();
    }
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Serial.println("Starting ISO-Module Firmware...");

  preferences.begin("iso-module", false);
  motorSpeed = preferences.getFloat("motorSpeed", DEFAULT_MOTOR_SPEED);
  motorAccel = preferences.getFloat("motorAccel", DEFAULT_MOTOR_ACCEL);
  totalSteps = preferences.getLong("totalSteps", 3000);

  stepper.setEnablePin(STEPPER_ENABLE_PIN);
  stepper.setPinsInverted(false, false, true);
  stepper.setMaxSpeed(motorSpeed);
  stepper.setAcceleration(motorAccel);
  stepper.setCurrentPosition(0);
  stepper.disableOutputs();

  BLEDevice::init("ISO-Module");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic* pCommandCharacteristic = pService->createCharacteristic(COMMAND_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCommandCharacteristic->setCallbacks(new CommandCallbacks());

  pNoiseCharacteristic = pService->createCharacteristic(NOISE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pNoiseCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  
  // This is the critical fix. The service UUID must be in the advertisement
  // for the web app to find the device.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  
  BLEDevice::startAdvertising();
  Serial.println("Advertising started...");

  IrReceiver.begin(IR_RECEIVE_PIN, false);
  setupI2S();
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 8192, NULL, 1, &audioTaskHandle, 0);
}

// ---------------- MAIN LOOP ----------------
void loop() {
  if (commandedPosition >= 0) {
    if (!stepper.isRunning())
      stepper.enableOutputs();
    stepper.moveTo(commandedPosition);
    wasClosedByNoise = commandIsFromAutomation;
    commandedPosition = -1;
  }

  stepper.run();

  float currentDB;
  portENTER_CRITICAL(&mux);
  currentDB = smoothedDB;
  portEXIT_CRITICAL(&mux);

  if (stepper.distanceToGo() == 0 && IrReceiver.decode()) {
    commandIsFromAutomation = false;
    switch (IrReceiver.decodedIRData.command) {
      case 0x8: commandedPosition = 0; break;
      case 0x5A: commandedPosition = totalSteps; break;
      case 0x1C: commandedPosition = stepper.currentPosition(); break;
    }
    IrReceiver.resume();
  }

  if (isAutoMode && isNoiseAutoCloseEnabled) {
    if (currentDB > closeThreshold) {
      if (lastNoiseExceededTime == 0)
        lastNoiseExceededTime = millis();

      if (millis() - lastNoiseExceededTime >= noiseDebounceDuration) {
        commandedPosition = totalSteps;
        commandIsFromAutomation = true;
        lastNoiseExceededTime = 0;
      }
    }
    else {
      lastNoiseExceededTime = 0;
    }

    if (currentDB < (closeThreshold - 15) && wasClosedByNoise) {
      if (lastNoiseDropTime == 0)
        lastNoiseDropTime = millis();

      if (millis() - lastNoiseDropTime >= reopenDebounceDuration) {
        commandedPosition = 0;
        commandIsFromAutomation = true;
        lastNoiseDropTime = 0;
      }
    }
    else {
      lastNoiseDropTime = 0;
    }
  }

  if (stepper.distanceToGo() == 0) {
    stepper.disableOutputs();
  }
  
  // Reconnect logic is now handled by the onDisconnect callback.
}