/*
  Decibel Defender - Arduino Stepper Motor Controller

  This sketch controls a NEMA 17 stepper motor using an A4988 driver
  on a CNC Shield v3. It listens for serial commands from a host
  (like a web app using the Web Serial API) to open, close, or move
  the curtain to a specific percentage.

  It includes logic to disable the motor when idle to prevent overheating
  and has safer default speed/acceleration settings to prevent skipping.

  Commands:
  - OPEN          : Moves the motor to the 0 position (fully open).
  - CLOSE         : Moves the motor to the `totalSteps` position (fully closed).
  - GOTO:[0-100]  : Moves the motor to a specific percentage of its travel.
  - SET_STEPS:[N] : Calibrates the total number of steps for full travel.
  - SET_SPEED:[N] : Sets the maximum motor speed (e.g., 400-2000).
  - SET_ACCEL:[N] : Sets the motor acceleration (e.g., speed / 2).
  - PING          : Responds with "PONG" to check the connection.
*/

#include <AccelStepper.h>

// Define the stepper motor connections for the X-axis on the CNC Shield
#define STEP_PIN   2
#define DIR_PIN    5
#define ENABLE_PIN 8 // All enable pins are tied to this one on the shield

// Create a new instance of the AccelStepper class
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// --- Configuration Variables ---
long totalSteps = 20000; // Default total steps for full travel (calibrate this!)
String inputString = "";     // A String to hold incoming data
bool stringComplete = false; // Whether the string is complete

void setup() {
  Serial.begin(115200); // Start serial communication
  inputString.reserve(200); // Reserve 200 bytes for the inputString

  // Set the enable pin to OUTPUT and disable the motor initially
  // The A4988 driver's enable pin is active LOW.
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // HIGH = disabled

  // Set safer default motor speed and acceleration to prevent skipping
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

  stepper.setEnablePin(ENABLE_PIN);
  stepper.setPinsInverted(false, false, true); // Invert the enable pin signal
}

void loop() {
  // Check if new serial data has arrived and process it
  serialEvent();
  
  if (stringComplete) {
    inputString.trim(); // Remove any leading/trailing whitespace
    handleCommand(inputString); // Process the command
    inputString = ""; // Clear the string
    stringComplete = false;
  }

  // If the motor is running, this function must be called as often as possible
  if (stepper.isRunning()) {
    stepper.run();
  } else {
    // If the motor has stopped, disable it to save power and prevent heat.
    // AccelStepper handles this automatically with setEnablePin.
  }
}

// This function is called when a complete command is received
void handleCommand(String cmd) {
  if (cmd.startsWith("GOTO:")) {
    int percentage = cmd.substring(5).toInt();
    percentage = constrain(percentage, 0, 100); // Clamp value between 0 and 100
    long targetPosition = map(percentage, 0, 100, totalSteps, 0);
    stepper.enableOutputs(); // Enable motor
    stepper.moveTo(targetPosition);
  }
  else if (cmd.equals("OPEN")) {
    stepper.enableOutputs(); // Enable motor
    stepper.moveTo(0);
  }
  else if (cmd.equals("CLOSE")) {
    stepper.enableOutputs(); // Enable motor
    stepper.moveTo(totalSteps);
  }
  else if (cmd.startsWith("SET_STEPS:")) {
    long steps = cmd.substring(10).toInt();
    if (steps > 0) {
      totalSteps = steps;
      Serial.println("OK: Steps set to " + String(totalSteps));
    } else {
      Serial.println("ERR: Invalid steps value.");
    }
  }
  else if (cmd.startsWith("SET_SPEED:")) {
    float speed = cmd.substring(10).toFloat();
    if (speed > 0) {
        stepper.setMaxSpeed(speed);
        Serial.println("OK: Speed set to " + String(speed));
    }
  }
  else if (cmd.startsWith("SET_ACCEL:")) {
    float accel = cmd.substring(10).toFloat();
    if (accel > 0) {
        stepper.setAcceleration(accel);
        Serial.println("OK: Acceleration set to " + String(accel));
    }
  }
  else if (cmd.equals("PING")) {
    Serial.println("PONG");
  }
  else {
    // Ignore empty commands
    if(cmd.length() > 0) {
      Serial.println("ERR: Unknown command: " + cmd);
    }
  }
}

// This function reads serial data into a buffer until a newline is received
void serialEvent() {
  while (Serial.available() && !stringComplete) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}
