#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "DESKTOP-2B1D8FN 0272"; // Your WiFi network name
const char* password = "85eR51}6";         // Your WiFi password

// Pin Definitions
const int buzzerPin = 27;  // Buzzer on GPIO27
const int trigPin1 = 5;
const int echoPin1 = 18;
const int trigPin2 = 19;
const int echoPin2 = 21;

// Thresholds
const int distanceThreshold = 50; // cm
const int queueAlertThreshold = 5; // Queue length to trigger buzzer

// Variables
int queueLength = 0;
int totalEntries = 0;
int totalExits = 0;

bool lastStateEntry = false;
bool lastStateExit = false;

// Web server
WebServer server(80);

// Measure distance function
long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 50000); // 50 ms timeout
  if (duration == 0) {
    return 9999; // Indicate an error or out-of-range reading
  }
  long distance = duration * 0.034 / 2;
  return distance;
}

// Serve HTML page
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Smart Queue Monitor</title>
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background: #f4f4f9; color: #333; }
        header { background: #2563eb; color: white; padding: 10px 20px; text-align: center; }
        .container { max-width: 800px; margin: 20px auto; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); }
        .card { margin-bottom: 20px; padding: 20px; border: 1px solid #ddd; border-radius: 8px; }
        .card h3 { margin: 0 0 10px; }
        .chart-container { position: relative; height: 300px; }
        .alert { padding: 10px; background: #ffcccc; color: #cc0000; border: 1px solid #cc0000; border-radius: 5px; margin-top: 20px; display: none; }
        .alert.show { display: block; }
      </style>
    </head>
    <body>
      <header>
        <h1>Smart Queue Monitor</h1>
      </header>
      <div class="container">
        <div class="card">
          <h3>Queue Length: <span id="queueLength">0</span></h3>
          <h3>Total Entries: <span id="totalEntries">0</span></h3>
          <h3>Total Exits: <span id="totalExits">0</span></h3>
        </div>
        <div class="card chart-container">
          <canvas id="queueChart"></canvas>
        </div>
        <div class="alert" id="queueAlert">
          <strong>Alert:</strong> Queue has exceeded the threshold!
        </div>
      </div>
      <script>
        const queueAlertThreshold = 5;
        const ctx = document.getElementById('queueChart').getContext('2d');
        const queueChart = new Chart(ctx, {
          type: 'line',
          data: {
            labels: [],
            datasets: [{
              label: 'Queue Length',
              data: [],
              borderColor: '#2563eb',
              backgroundColor: 'rgba(37, 99, 235, 0.2)',
              fill: true
            }]
          },
          options: {
            responsive: true,
            scales: {
              x: { beginAtZero: true },
              y: { beginAtZero: true }
            }
          }
        });

        async function fetchData() {
          try {
            const response = await fetch('/data');
            const data = await response.json();
            document.getElementById('queueLength').textContent = data.queueLength;
            document.getElementById('totalEntries').textContent = data.totalEntries;
            document.getElementById('totalExits').textContent = data.totalExits;

            const now = new Date();
            queueChart.data.labels.push(now.toLocaleTimeString());
            queueChart.data.datasets[0].data.push(data.queueLength);
            if (queueChart.data.labels.length > 20) {
              queueChart.data.labels.shift();
              queueChart.data.datasets[0].data.shift();
            }
            queueChart.update();

            const alert = document.getElementById('queueAlert');
            if (data.queueLength >= queueAlertThreshold) {
              alert.classList.add('show');
            } else {
              alert.classList.remove('show');
            }
          } catch (error) {
            console.error('Error fetching data:', error);
          }
        }

        setInterval(fetchData, 2000);
        fetchData();
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// Send dynamic JSON data
void handleData() {
  String json = "{";
  json += "\"queueLength\":" + String(queueLength) + ",";
  json += "\"totalEntries\":" + String(totalEntries) + ",";
  json += "\"totalExits\":" + String(totalExits);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Smart Queue Monitor...");

  // Setup pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize mDNS
  if (MDNS.begin("queuemonitor")) {
    Serial.println("mDNS responder started. Access at http://queuemonitor.local");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // Define server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();

  // Entry detection
  long distance1 = measureDistance(trigPin1, echoPin1);
  bool entryDetected = (distance1 > 0 && distance1 < distanceThreshold);
  if (entryDetected && !lastStateEntry) {
    queueLength++;
    totalEntries++;
    Serial.println("Entry detected!");
  }
  lastStateEntry = entryDetected;

  // Exit detection
  long distance2 = measureDistance(trigPin2, echoPin2);
  bool exitDetected = (distance2 > 0 && distance2 < distanceThreshold);
  if (exitDetected && !lastStateExit && queueLength > 0) {
    queueLength--;
    totalExits++;
    Serial.println("Exit detected!");
  }
  lastStateExit = exitDetected;

  // Buzzer control
  if (queueLength >= queueAlertThreshold) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
  }

  delay(200);
}
