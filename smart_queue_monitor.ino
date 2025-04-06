#include "network_config.h"
#include "ultrasonic_sensor.h"
#include "web_server.h"
#include "buzzer_controller.h"
#include "queue_monitor.h"

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Smart Queue Monitor...");

  // Initialize hardware components
  initializeSensors();
  initializeBuzzer();
  
  // Connect to WiFi
  if (!connectToWiFi()) {
    return; // Exit setup if WiFi connection fails
  }
  
  // Setup mDNS
  setupMDNS();
  
  // Setup web server
  setupServer();
}

void loop() {
  // Handle client requests
  server.handleClient();
  
  // Read sensor data
  long distance1 = measureDistance(trigPin1, echoPin1);
  long distance2 = measureDistance(trigPin2, echoPin2);
  
  // Update queue based on sensor readings
  updateQueue(distance1, distance2);
  
  // Update buzzer state
  updateBuzzer(queueLength);
  
  // Small delay for stability
  delay(200);
}
