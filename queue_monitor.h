#ifndef QUEUE_MONITOR_H
#define QUEUE_MONITOR_H

// Queue monitoring variables
int queueLength = 0;
int totalEntries = 0;
int totalExits = 0;

bool lastStateEntry = false;
bool lastStateExit = false;

// Function to update queue based on sensor readings
void updateQueue(long distance1, long distance2) {
  // --- Entry Detection ---
  bool entryDetected = (distance1 > 0 && distance1 < distanceThreshold && distance1 != 9999);

  if (entryDetected && !lastStateEntry) {
    queueLength++;
    totalEntries++;
    Serial.print("Entry Detected! Queue: "); Serial.println(queueLength);
    // Optional: Short beep on entry could be added here if needed
  }
  lastStateEntry = entryDetected;

  // --- Exit Detection ---
  bool exitDetected = (distance2 > 0 && distance2 < distanceThreshold && distance2 != 9999);

  // Only decrement if someone is actually in the queue
  if (exitDetected && !lastStateExit && queueLength > 0) {
    queueLength--;
    totalExits++;
    Serial.print("Exit Detected! Queue: "); Serial.println(queueLength);
    // Optional: Different short beep on exit could be added here if needed
  }
  
  // Ensure queue length doesn't go negative due to sensor glitches
  if (queueLength < 0) {
    queueLength = 0;
  }
  lastStateExit = exitDetected;
}

#endif // QUEUE_MONITOR_H
