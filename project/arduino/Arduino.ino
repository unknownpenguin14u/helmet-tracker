/*
  Decibel Defender - Automatic Curtain Controller
  
  This sketch controls a stepper motor to open and close a curtain based on 
  commands received over the serial port.
  
  It is designed to work with the Decibel Defender web application, which can
  connect directly to the Arduino using the Web Serial API.

  Hardware:
  - Arduino Uno (or compatible)
  - CNC Shield v3
  - A4988 Stepper Driver (on the X-axis)
  - NEMA 17 Stepper Motor
  - 12V Power Supply for the CNC Shield
*/

#include <AccelStepper.h>

// =================================================================
// == Configuration
// =================================================================

// The AccelStepper library uses the CNC Shield's default pins for the X-axis.
// Step pin: D2
// Direction pin: D5
const int STEP_PIN = 2;
const int DIR_PIN = 5;

// This is the "Enable" pin on the CNC Shield. 
// When this pin is HIGH, the driver is disabled, letting the motor cool down.
// When LOW, the driver is enabled and holds the motor's position.
const int ENABLE_PIN = 8; 

// --- Motor Movement Settings ---
const float MOTOR_ACCELERATION = 800.0;
const float MOTOR_MAX_SPEED = 1000.0;

// --- Curtain Calibration ---
// This is the default total number of steps for the motor to travel from
// fully closed to fully open. This can be updated on-the-fly by sending
// the command "SET_STEPS:<number>" from the serial monitor or web UI.
long totalStepsForCurtain = 20000; 

// =================================================================
// == Global Variables
// =================================================================

// Create an instance of the AccelStepper library.
// The "1" signifies that we are using a driver with STEP and DIR pins.
AccelStepper stepper(1, STEP_PIN, DIR_PIN);

// Enum to manage the motor's state.
enum MotorState {
  IDLE,
  MOVING,
  STOP_REQUESTED
};
MotorState motorState = IDLE;

// String to store incoming serial data.
String serialData;
bool isSerialDataComplete = false;

// =================================================================
// == Motor Control Functions
// =================================================================

// Enables the stepper driver, allowing it to move and hold position.
// This is called right before a move starts.
void enableMotor() {
  digitalWrite(ENABLE_PIN, LOW); // LOW = Enabled
  delay(10); // Small delay to ensure the driver is ready
}

// Disables the stepper driver. The motor will stop holding its position
// and cool down. This is called when the motor is idle.
void disableMotor() {
  digitalWrite(ENABLE_PIN, HIGH); // HIGH = Disabled
}

// Moves the curtain to a target position.
// This function is non-blocking. The main loop will handle the movement.
void goToPosition(long newPosition) {
  // Ensure the target position is within the valid range (0 to totalStepsForCurtain)
  newPosition = constrain(newPosition, 0, totalStepsForCurtain);

  enableMotor(); // Power up the motor before moving.
  stepper.moveTo(newPosition);
  motorState = MOVING;
  
  Serial.print("OK: Moving to ");
  Serial.println(newPosition);
}

// =================================================================
// == Setup
// =================================================================

void setup() {
  // Start the serial communication at 9600 baud.
  Serial.begin(9600);
  Serial.println("Arduino Initialized. Decibel Defender Ready.");

  // Configure the enable pin as an output.
  pinMode(ENABLE_PIN, OUTPUT);
  
  // Set motor parameters.
  stepper.setMaxSpeed(MOTOR_MAX_SPEED);
  stepper.setAcceleration(MOTOR_ACCELERATION);

  // Set the initial state of the motor. We assume it starts fully open (at max steps).
  // This allows us to immediately send a "CLOSE" command without needing to home first.
  stepper.setCurrentPosition(totalStepsForCurtain);
  
  // Start with the motor disabled to keep it cool.
  disableMotor();
}

// =================================================================
// == Main Loop
// =================================================================

void loop() {
  // Check for and process any incoming serial commands.
  handleSerialCommands();

  // If the motor is supposed to be moving, run the stepper motor control.
  if (motorState == MOVING) {
    // If a stop was requested, stop the motor immediately.
    if (motorState == STOP_REQUESTED) {
      stepper.stop();
      motorState = IDLE;
      disableMotor();
      Serial.println("OK: Motor stopped.");
      return;
    }
    
    // If the motor has reached its target position...
    if (stepper.distanceToGo() == 0) {
      motorState = IDLE;
      disableMotor(); // Disable motor to let it cool down.
      Serial.println("OK: Move complete.");
      Serial.print("Current Position: ");
      Serial.println(stepper.currentPosition());
    } else {
      // Otherwise, continue running the motor.
      stepper.run();
    }
  }
}

// =================================================================
// == Serial Command Handling
// =================================================================

void handleSerialCommands() {
  // Read any available serial data.
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    serialData += inChar;
    // A newline character indicates the end of a command.
    if (inChar == '\n') {
      isSerialDataComplete = true;
    }
  }

  // If a complete command has been received...
  if (isSerialDataComplete) {
    serialData.trim(); // Remove any whitespace.

    if (serialData.length() > 0) {
      Serial.print("Received Command: ");
      Serial.println(serialData);
      
      // --- Process Commands ---

      if (serialData.equalsIgnoreCase("OPEN")) {
        goToPosition(totalStepsForCurtain);
      }
      else if (serialData.equalsIgnoreCase("CLOSE")) {
        goToPosition(0);
      }
      else if (serialData.equalsIgnoreCase("STOP")) {
        motorState = STOP_REQUESTED;
      }
      else if (serialData.startsWith("GOTO:")) {
        // Extracts the percentage value after "GOTO:"
        int percentage = serialData.substring(5).toInt();
        percentage = constrain(percentage, 0, 100); // Ensure percentage is between 0 and 100
        long targetPosition = map(percentage, 0, 100, 0, totalStepsForCurtain);
        goToPosition(targetPosition);
      }
      else if (serialData.startsWith("SET_STEPS:")) {
        // Extracts the new total steps value and updates the setting.
        long newTotalSteps = serialData.substring(10).toInt();
        if (newTotalSteps > 0) {
          totalStepsForCurtain = newTotalSteps;
          Serial.print("OK: Total steps updated to ");
          Serial.println(totalStepsForCurtain);
          // Optional: You might want to update current position if curtain state is known
          // For simplicity, we just update the limit. A re-calibration might be needed.
        } else {
          Serial.println("ERROR: Invalid steps value.");
        }
      }
      else {
        Serial.println("ERROR: Unknown command.");
      }
    }

    // Reset for the next command.
    serialData = "";
    isSerialDataComplete = false;
  }
}
