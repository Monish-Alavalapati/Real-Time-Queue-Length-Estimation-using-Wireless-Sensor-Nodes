#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

// Pin Definitions for Ultrasonic Sensors
const int trigPin1 = 5;   // Entry sensor trigger pin
const int echoPin1 = 18;  // Entry sensor echo pin
const int trigPin2 = 19;  // Exit sensor trigger pin
const int echoPin2 = 21;  // Exit sensor echo pin

// Threshold for detection (in cm)
const int distanceThreshold = 50;

// Function to measure distance using ultrasonic sensor
long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Increased timeout for pulseIn to potentially handle longer distances or slower responses
  long duration = pulseIn(echoPin, HIGH, 50000); // 50 ms timeout
  
  // Check if timeout occurred (pulseIn returns 0 on timeout)
  if (duration == 0) {
      // Handle timeout, e.g., return a large value or a specific error code
      return 9999; // Indicate an error or out-of-range reading
  }
  
  long distance = duration * 0.034 / 2;
  return distance;
}

// Function to initialize ultrasonic sensor pins
void initializeSensors() {
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
}

#endif // ULTRASONIC_SENSOR_H
