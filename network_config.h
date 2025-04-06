#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <WiFi.h>
#include <ESPmDNS.h>

// WiFi credentials
const char* ssid = "DESKTOP-2B1D8FN 0272"; // Your WiFi network name
const char* password = "85eR51}6";         // Your WiFi password

// Function to connect to WiFi
bool connectToWiFi() {
  WiFi.mode(WIFI_STA); // Set WiFi mode to Station explicitly
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi [");
  Serial.print(ssid);
  Serial.print("]...");

  int connect_timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    connect_timeout++;
    if(connect_timeout > 30) { // ~15 second timeout
        Serial.println("\nFailed to connect to WiFi!");
        return false;
    }
  }
  
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

// Function to setup mDNS
bool setupMDNS() {
  // Small delay before starting mDNS
  delay(1000);
  
  Serial.print("Setting up mDNS responder with hostname 'queuemonitor'...");
  if (MDNS.begin("queuemonitor")) { // Hostname is "queuemonitor", access via queuemonitor.local
    Serial.println("\nmDNS responder started successfully.");
    Serial.println("You can now access the dashboard at: http://queuemonitor.local");
    MDNS.addService("http", "tcp", 80); // Announce the web server service
    return true;
  } else {
    Serial.println("\nError setting up mDNS responder!");
    return false;
  }
}

#endif // NETWORK_CONFIG_H
