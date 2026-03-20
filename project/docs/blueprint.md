# **App Name**: Decibel Defender

## Core Features:

- Noise Detection: Use a sound sensor connected to the Arduino Uno to detect ambient noise levels.
- Decibel Conversion: Convert the sensor's analog readings into decibel levels using a calibration curve.
- Threshold Trigger: Automatically trigger the curtain closing mechanism when the detected noise level exceeds 60 decibels.
- Curtain Control: Control the Nema 17 stepper motor connected to the CNC shield V3 to open and close the curtains.
- Calibration Mode: Enable users to calibrate the sound sensor for more accurate decibel readings.
- Visual Indicator: Show a visual representation of noise level to allow the user to see what the noise levels are even if they are not quite high enough to trigger the curtain closing. This should be accomplished via the existing LED, which should light up more brightly the closer to the threshold you are, and flash when the curtains close. 

## Style Guidelines:

- Primary color: Dark, muted blue (#34495E) to convey a sense of calmness and protection.
- Background color: Very dark gray (#2C3E50), close in hue to the primary color, for a dark, restful theme.
- Accent color: Bright, contrasting orange (#E67E22) to draw attention to important indicators and calibration options.
- Body and headline font: 'Inter' (sans-serif) for a clean, modern, and readable interface.
- Use simple, minimalist icons to represent settings, calibration, and noise level indicators.
- Prioritize a clear, uncluttered interface with essential information prominently displayed.
- Subtle animations, such as a smooth curtain closing effect, can enhance the user experience without being distracting.