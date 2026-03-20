/**
 * @file DecibelDefender.ino
 * @brief Firmware for an automated curtain system controlled via serial commands.
 *
 * This Arduino sketch controls a NEMA 17 stepper motor using an A4988 driver
 * on a CNC Shield v3. It listens for serial commands to open, close, or move
 * the curtain to a specific percentage.
 *
 * It features the ability to set the total curtain travel distance (in steps)
 * via a serial command for easy calibration. It also disables the motor
 * when idle to prevent overheating and save power.
 *
 * Pre-requisites:
 * - Arduino IDE with AccelStepper library installed.
 *   (Go to Sketch > Include Library > Manage Libraries... and search for "AccelStepper")
 *
 * Hardware:
 * - Arduino Uno
 * - CNC Shield v3
 * - A4988 Stepper Driver (on X-axis slot)
 * - NEMA 17 Stepper Motor
 *
 * Serial Commands:
 * - "OPEN"        : Fully opens the curtain (moves to position 0).
 * - "CLOSE"       : Fully closes the curtain (moves to TOTAL_STEPS).
 * - "GOTO:50"     : Moves the curtain to a specific percentage (e.g., 50%).
 * - "STOP"        : Immediately stops any current movement.
 * - "RESET"       : Resets the motor's current position to be the new origin (0).
 * - "SET_STEPS:N" : Sets the total steps for the curtain travel to N. (e.g., "SET_STEPS:20000").
 *                   This value is stored in volatile memory and will be reset on power loss.
 * - "ping"        : A test command to check the connection. Responds with "pong".
 */

#include <AccelStepper.h>

// --- Pin Definitions for CNC Shield v3 ---
const int STEP_PIN = 2;         // X-axis step pin
const int DIR_PIN = 5;          // X-axis direction pin
const int ENABLE_PIN = 8;       // Stepper enable pin (active LOW)

// --- Motor Configuration ---
const int MOTOR_INTERFACE_TYPE = 1; // AccelStepper driver type for A4988
const float MAX_SPEED = 2000.0;     // Steps per second
const float ACCELERATION = 1000.0;  // Steps per second squared

// --- State Variables ---
long totalStepsForCurtain = 20000; // Default total steps for full travel
String inputString = "";           // A String to hold incoming data
bool stringComplete = false;       // Whether the string is complete

// Create a new instance of the AccelStepper class
AccelStepper stepper(MOTOR_INTERFACE_TYPE, STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(9600);
  inputString.reserve(50); // Reserve memory for the input string

  pinMode(ENABLE_PIN, OUTPUT);
  disableMotor(); // Start with motor disabled

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);
  
  Serial.println("Decibel Defender Initialized.");
}

void loop() {
  // Check for new serial commands if the motor is not busy
  if (stringComplete) {
    processCommand();
    inputString = "";
    stringComplete = false;
  }

  // If the motor is running, let it run. Otherwise, disable it.
  if (stepper.isRunning()) {
    stepper.run();
  } else {
    disableMotor();
  }
}

/**
 * @brief Reads incoming serial data character by character.
 * Sets `stringComplete` to true when a newline character is received.
 */
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

/**
 * @brief Processes the received serial command.
 */
void processCommand() {
  inputString.trim(); // Remove any whitespace

  if (inputString.equalsIgnoreCase("OPEN")) {
    moveCurtainToPosition(0);
  } else if (inputString.equalsIgnoreCase("CLOSE")) {
    moveCurtainToPosition(totalStepsForCurtain);
  } else if (inputString.startsWith("GOTO:")) {
    int percentage = inputString.substring(5).toInt();
    percentage = constrain(percentage, 0, 100);
    long targetPos = map(percentage, 0, 100, 0, totalStepsForCurtain);
    moveCurtainToPosition(targetPos);
  } else if (inputString.equalsIgnoreCase("STOP")) {
    stopMotor();
  } else if (inputString.equalsIgnoreCase("RESET")) {
    resetMotorOrigin();
  } else if (inputString.startsWith("SET_STEPS:")) {
    long newSteps = inputString.substring(10).toInt();
    setTotalSteps(newSteps);
  } else if (inputString.equalsIgnoreCase("ping")) {
    Serial.println("pong");
  } else {
    Serial.print("Unknown command: ");
    Serial.println(inputString);
  }
}

/**
 * @brief Enables power to the stepper motor driver.
 */
void enableMotor() {
  digitalWrite(ENABLE_PIN, LOW); // Active LOW
}

/**
 * @brief Disables power to the stepper motor driver to save power and prevent heat.
 */
void disableMotor() {
  digitalWrite(ENABLE_PIN, HIGH); // Active LOW
}

/**
 * @brief Moves the curtain to a specific target step position.
 * @param targetPosition The absolute position (in steps) to move to.
 */
void moveCurtainToPosition(long targetPosition) {
  enableMotor();
  stepper.moveTo(targetPosition);
  Serial.print("Moving to position: ");
  Serial.println(targetPosition);
}

/**
 * @brief Immediately stops the motor and clears its movement queue.
 */
void stopMotor() {
  stepper.stop();
  stepper.runToPosition(); // Clear the buffer
  disableMotor();
  Serial.println("Motor stopped.");
}

/**
 * @brief Resets the motor's current position to be the new zero point.
 */
void resetMotorOrigin() {
  stepper.setCurrentPosition(0);
  Serial.println("Motor origin reset to 0.");
}

/**
 * @brief Updates the total steps for the curtain travel distance.
 * @param newTotalSteps The new total number of steps.
 */
void setTotalSteps(long newTotalSteps) {
  if (newTotalSteps > 0) {
    totalStepsForCurtain = newTotalSteps;
    Serial.print("Total steps set to: ");
    Serial.println(totalStepsForCurtain);
  } else {
    Serial.println("Invalid number of steps.");
  }
}
