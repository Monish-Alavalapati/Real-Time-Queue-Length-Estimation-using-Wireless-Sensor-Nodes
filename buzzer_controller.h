#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

// Pin definitions
const int buzzerPin = 27;  // Buzzer on GPIO27

// Threshold for queue alert
const int queueAlertThreshold = 5; // Queue length to trigger buzzer

// Function to initialize buzzer
void initializeBuzzer() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Ensure buzzer is off initially
}

// Function to control buzzer based on queue length
void updateBuzzer(int queueLength) {
  if (queueLength >= queueAlertThreshold) {
    // Intermittent beeping when queue length exceeds threshold
    static unsigned long lastBeepTime = 0;
    if (millis() - lastBeepTime > 1000) {
        digitalWrite(buzzerPin, HIGH);
        delay(100); // Beep duration
        digitalWrite(buzzerPin, LOW);
        lastBeepTime = millis();
    }
  } else {
    digitalWrite(buzzerPin, LOW); // Ensure buzzer is off if below threshold
  }
}

#endif // BUZZER_CONTROLLER_H
