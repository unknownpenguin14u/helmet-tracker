#include <AccelStepper.h>

// --- Configuration: PLEASE ADJUST THESE VALUES ---

// Define the pins for your CNC Shield (for the X-axis)
// You might need to change these if you use Y or Z axis connections.
const int STEP_PIN = 2;
const int DIR_PIN = 5;

// Steps per revolution for your NEMA 17 motor (usually 200)
const int STEPS_PER_REVOLUTION = 200;

// !! IMPORTANT !! Calibrate this value for your setup.
// This is the total number of steps to move the curtain from fully closed to fully open.
const long TOTAL_STEPS_FOR_CURTAIN = 20000; // <-- EXAMPLE VALUE: Adjust this!

// Set the speed and acceleration for the motor.
const float MAX_SPEED = 800.0;
const float ACCELERATION = 400.0;

// --- End of Configuration ---

// Initialize the stepper library
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  // Start the serial communication at 9600 baud.
  // This must match the rate in our local bridge server.
  Serial.begin(9600);

  // Configure the stepper motor settings
  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  // Set initial position to 0 (fully closed)
  stepper.setCurrentPosition(0);

  Serial.println("Arduino Curtain Controller Initialized.");
  Serial.println("Waiting for commands (e.g., OPEN, CLOSE, GOTO:50)...");
}

void loop() {
  // Check if there's data available to read from the serial port.
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove any whitespace

    Serial.print("Received command: ");
    Serial.println(command);

    if (command.startsWith("GOTO:")) {
      // Handles commands like "GOTO:75"
      int percentage = command.substring(5).toInt();
      // Clamp the value between 0 and 100
      percentage = max(0, min(100, percentage));
      long targetPosition = map(percentage, 0, 100, 0, TOTAL_STEPS_FOR_CURTAIN);
      stepper.moveTo(targetPosition);
      Serial.print("Moving to ");
      Serial.print(percentage);
      Serial.println("%");
    } else if (command == "OPEN") {
      stepper.moveTo(TOTAL_STEPS_FOR_CURTAIN); // 100% open
       Serial.println("Opening curtain fully.");
    } else if (command == "CLOSE") {
      stepper.moveTo(0); // 0% open (closed)
      Serial.println("Closing curtain fully.");
    } else {
      Serial.println("Unknown command.");
    }
  }

  // This is crucial! It tells the stepper to move if it needs to.
  // It's non-blocking, so it runs continuously.
  stepper.run();
}
