/*
  Decibel Defender - Arduino Stepper Motor Controller
  
  This sketch controls a NEMA 17 stepper motor using an Arduino Uno,
  a CNC Shield v3, and an A4988 driver. It listens for simple serial
  commands to open, close, or move the curtain to a specific percentage.

  It also reads from a MAX4466 microphone to provide ambient noise level data.

  Serial Commands:
  - OPEN: Moves the curtain to the 100% open position.
  - CLOSE: Moves the curtain to the 0% closed position.
  - GOTO:[0-100]: Moves the curtain to a specific percentage. (e.g., "GOTO:50")
  - SET_STEPS:[steps]: Sets the total steps for the full curtain travel, for calibration. (e.g., "SET_STEPS:20000")
  - GET_NOISE: Reads the ambient noise level and prints it to the serial port.
*/

#include <AccelStepper.h>

// --- Pin Definitions ---
// Stepper motor connection on CNC Shield's X-axis
#define STEP_PIN 2
#define DIR_PIN 5

// Motor driver enable pin (active LOW)
// This pin is used to disable the motor driver when not in use to prevent overheating.
#define ENABLE_PIN 8

// Analog pin for the MAX4466 microphone
#define MIC_PIN A0

// --- Stepper Motor Configuration ---
// The A4988 driver is set to 1/16 microstepping by default on the CNC shield.
// A standard NEMA 17 motor has 200 steps per revolution.
// 200 steps * 16 microsteps = 3200 steps per revolution.
const int STEPS_PER_REVOLUTION = 3200;

// This value is CRITICAL for calibration. It's the total number of steps
// the motor needs to turn to move the curtain from fully closed to fully open.
// This can be updated on-the-fly with the "SET_STEPS" command.
long totalStepsForCurtain = 20000;

// Create an instance of the AccelStepper library
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// --- State Management ---
enum MotorState {
  IDLE,
  MOVING
};
MotorState motorState = IDLE;

// --- Serial Communication ---
String inputString = "";         // A String to hold incoming data
bool stringComplete = false;  // Whether the string is complete

void setup() {
  Serial.begin(9600);
  inputString.reserve(50); // Pre-allocate memory for the string

  // Configure motor control pins
  pinMode(ENABLE_PIN, OUTPUT);

  // By default, the motor driver is disabled. Power is cut off, motor is cool and can be moved by hand.
  disableMotor();

  // Configure the AccelStepper library
  stepper.setMaxSpeed(800);    // Steps per second
  stepper.setAcceleration(400); // Steps per second per second
}

void loop() {
  // Check for incoming serial commands
  if (stringComplete) {
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }

  // If the motor is supposed to be moving, run the stepper
  if (motorState == MOVING) {
    // If the motor has reached its target position
    if (stepper.distanceToGo() == 0) {
      motorState = IDLE;
      // Disable motor to prevent overheating and save power
      disableMotor();
    } else {
      // Continue moving towards the target
      stepper.run();
    }
  }
}

// --- Serial Event Handler ---
// This function is called whenever data is received from the serial port.
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

// --- Command Processing ---
void processCommand(String cmd) {
  cmd.trim(); // Remove any whitespace

  if (cmd.equalsIgnoreCase("OPEN")) {
    moveCurtainTo(100);
  } else if (cmd.equalsIgnoreCase("CLOSE")) {
    moveCurtainTo(0);
  } else if (cmd.startsWith("GOTO:")) {
    int percentage = cmd.substring(5).toInt();
    moveCurtainTo(percentage);
  } else if (cmd.startsWith("SET_STEPS:")) {
    long newTotalSteps = cmd.substring(10).toInt();
    if (newTotalSteps > 0) {
      totalStepsForCurtain = newTotalSteps;
      Serial.print("OK: Total steps set to ");
      Serial.println(totalStepsForCurtain);
    }
  } else if (cmd.equalsIgnoreCase("GET_NOISE")) {
    int noise = getNoiseLevel();
    Serial.print("NOISE:");
    Serial.println(noise);
  }
}

// --- Motor Control Functions ---

// Enables the A4988 driver. Must be called before any movement.
void enableMotor() {
  digitalWrite(ENABLE_PIN, LOW); // LOW means enabled
  delay(10); // Short delay to ensure driver is ready
}

// Disables the A4988 driver. Motor will be cool and free-wheeling.
void disableMotor() {
  digitalWrite(ENABLE_PIN, HIGH); // HIGH means disabled
}

// Calculates the target step position based on a percentage and commands the motor to move.
void moveCurtainTo(int percentage) {
  // Constrain percentage to be between 0 and 100
  percentage = constrain(percentage, 0, 100);

  // Calculate the target step position
  long targetPosition = map(percentage, 0, 100, 0, totalStepsForCurtain);

  // Only move if the target is different from the current position
  if (targetPosition != stepper.currentPosition()) {
    enableMotor(); // Power up the motor
    motorState = MOVING;
    stepper.moveTo(targetPosition);
  }
}

// --- Sensor Reading Function ---

// Reads the microphone and maps the analog value to a pseudo-dB scale
int getNoiseLevel() {
  const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
  unsigned int sample;
  
  unsigned long startMillis = millis(); // Start of sample window
  unsigned int peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(MIC_PIN);
    if (sample < 1024) { // toss out spurious readings
      if (sample > signalMax) {
        signalMax = sample;  // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  
  // Map the peak-to-peak amplitude to a more intuitive 0-100 dB-like scale.
  // This is not a true dB value but provides a useful relative metric.
  // The values (e.g., 10, 800) may need tuning based on your microphone's sensitivity.
  int noise = map(peakToPeak, 10, 800, 30, 100);
  return constrain(noise, 30, 100); // Constrain to a reasonable range
}
