#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "DESKTOP-2B1D8FN 0272"; // Your WiFi network name
const char* password = "85eR51}6";         // Your WiFi password

// Pin Definitions
const int buzzerPin = 27;  // Buzzer on GPIO27

// Ultrasonic sensor pins
const int trigPin1 = 5;    // Entry sensor trigger
const int echoPin1 = 18;   // Entry sensor echo
const int trigPin2 = 19;   // Exit sensor trigger  
const int echoPin2 = 21;   // Exit sensor echo

// LED pins for each queue
const int numQueues = 5;
const int ledPins[numQueues] = {22, 23, 25, 26, 33};

// Thresholds
const int distanceThreshold = 50; // cm
const int queueAlertThreshold = 5; // Queue length to trigger buzzer/LED

// Variables for main queue being monitored
int queueLength = 0;
int totalEntries = 0;
int totalExits = 0;

bool lastStateEntry = false;
bool lastStateExit = false;

// Variables for all queues
int queueLengths[numQueues] = {0};
bool queueBusy[numQueues] = {false};
unsigned long lastLedToggleTime[numQueues] = {0};
bool ledState[numQueues] = {false};

// For calculating averages and maximums
int maxQueueLength = 0;
int queueSamples = 0;
int totalQueueSum = 0;
float avgQueueLength = 0;

// LED blink interval (milliseconds)
const int blinkInterval = 500;

// Web server
WebServer server(80);

// Function to measure distance from ultrasonic sensor
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

// Function to update LED states based on queue status
void updateLEDs() {
  // First queue (index 0) is the main queue we're monitoring with sensors
  queueLengths[0] = queueLength;
  queueBusy[0] = (queueLength >= queueAlertThreshold);
  
  // Update LED states
  unsigned long currentMillis = millis();
  
  for (int i = 0; i < numQueues; i++) {
    if (queueBusy[i]) {
      // Blink LED if queue is busy
      if (currentMillis - lastLedToggleTime[i] >= blinkInterval) {
        lastLedToggleTime[i] = currentMillis;
        ledState[i] = !ledState[i];
        digitalWrite(ledPins[i], ledState[i] ? HIGH : LOW);
      }
    } else {
      // LED stays off if queue is not busy
      digitalWrite(ledPins[i], LOW);
      ledState[i] = false;
    }
  }
}

// Function to find the shortest queue
int findShortestQueue() {
  int shortestQueueIndex = 0;
  int minLength = queueLengths[0];
  
  for (int i = 1; i < numQueues; i++) {
    if (queueLengths[i] < minLength) {
      minLength = queueLengths[i];
      shortestQueueIndex = i;
    }
  }
  
  return shortestQueueIndex;
}

// Function to update queue statistics
void updateQueueStats() {
  // Update maximum queue length
  if (queueLength > maxQueueLength) {
    maxQueueLength = queueLength;
  }
  
  // Update average queue length
  queueSamples++;
  totalQueueSum += queueLength;
  avgQueueLength = (float)totalQueueSum / queueSamples;
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
        :root {
          --primary: #3b82f6;
          --primary-dark: #2563eb;
          --secondary: #10b981;
          --danger: #ef4444;
          --warning: #f59e0b;
          --light: #f3f4f6;
          --dark: #1f2937;
          --gray: #6b7280;
          --success-bg: #ecfdf5;
          --warning-bg: #fffbeb;
          --danger-bg: #fee2e2;
        }
        body { 
          font-family: 'Segoe UI', Arial, sans-serif; 
          margin: 0; 
          padding: 0; 
          background: #f8fafc; 
          color: var(--dark);
          line-height: 1.6;
        }
        header { 
          background: linear-gradient(135deg, var(--primary), var(--primary-dark)); 
          color: white; 
          padding: 1.5rem; 
          text-align: center;
          box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
          position: relative;
        }
        header h1 {
          margin: 0;
          font-weight: 600;
          font-size: 2rem;
        }
        .mode-switch {
          position: absolute;
          top: 1rem;
          right: 1rem;
          background: rgba(255, 255, 255, 0.2);
          border-radius: 30px;
          padding: 5px 15px;
          color: white;
          font-size: 0.85rem;
          cursor: pointer;
          transition: all 0.3s ease;
          display: flex;
          align-items: center;
        }
        .mode-switch:hover {
          background: rgba(255, 255, 255, 0.3);
        }
        .mode-switch svg {
          margin-right: 5px;
        }
        .container { 
          max-width: 1000px; 
          margin: 2rem auto; 
          padding: 0 1rem;
        }
        .dashboard {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
          gap: 1.5rem;
          margin-bottom: 1.5rem;
        }
        .card { 
          background: white; 
          border-radius: 0.75rem; 
          box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.05);
          padding: 1.5rem;
          transition: transform 0.2s, box-shadow 0.2s;
        }
        .card:hover {
          transform: translateY(-3px);
          box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1);
        }
        .stat-card {
          text-align: center;
          position: relative;
          overflow: hidden;
        }
        .stat-card h3 { 
          font-weight: 500;
          color: var(--gray);
          margin: 0 0 0.5rem;
          font-size: 0.9rem;
          text-transform: uppercase;
          letter-spacing: 0.05em;
        }
        .stat-value {
          font-size: 3rem;
          font-weight: 700;
          margin: 0.5rem 0;
          color: var(--dark);
        }
        .queue-value {
          color: var(--primary-dark);
        }
        .entries-value {
          color: var(--secondary);
        }
        .exits-value {
          color: var(--warning);
        }
        .chart-container { 
          position: relative; 
          height: 350px;
          grid-column: 1 / -1;
        }
        .chart-card h3 {
          margin-top: 0;
          color: var(--dark);
          font-weight: 600;
          display: flex;
          justify-content: space-between;
          align-items: center;
        }
        .chart-toggle {
          display: flex;
          font-size: 0.8rem;
          background: var(--light);
          border-radius: 20px;
          overflow: hidden;
        }
        .chart-toggle button {
          border: none;
          background: transparent;
          padding: 4px 12px;
          cursor: pointer;
          font-size: 0.8rem;
          font-weight: 500;
        }
        .chart-toggle button.active {
          background: var(--primary);
          color: white;
        }
        .alert { 
          padding: 1rem 1.5rem; 
          background: #fee2e2; 
          color: var(--danger); 
          border-left: 4px solid var(--danger); 
          border-radius: 0.5rem; 
          margin-top: 1.5rem; 
          display: none;
          align-items: center;
          font-weight: 500;
        }
        .alert.show { 
          display: flex; 
        }
        .alert svg {
          margin-right: 0.75rem;
          flex-shrink: 0;
        }
        .status-indicator {
          display: inline-block;
          width: 12px;
          height: 12px;
          border-radius: 50%;
          margin-right: 8px;
        }
        .status-ok {
          background-color: var(--secondary);
        }
        .status-warning {
          background-color: var(--warning);
        }
        .status-alert {
          background-color: var(--danger);
        }
        .stats-small {
          display: flex;
          justify-content: space-around;
          margin-top: 0.75rem;
          padding-top: 0.75rem;
          border-top: 1px solid #eee;
          font-size: 0.9rem;
        }
        .stat-small {
          text-align: center;
        }
        .stat-small-value {
          font-weight: 600;
          font-size: 1.2rem;
          display: block;
        }
        .stat-small-label {
          color: var(--gray);
          font-size: 0.8rem;
        }
        .traffic-flow {
          display: flex;
          justify-content: center;
          align-items: center;
          margin-top: 0.5rem;
        }
        .flow-arrow {
          color: var(--primary);
          margin: 0 1rem;
          font-size: 1.5rem;
        }
        .all-queues-section {
          margin-top: 2rem;
          grid-column: 1 / -1;
        }
        .queues-grid {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
          gap: 1rem;
          margin-top: 1rem;
        }
        .queue-card {
          padding: 1rem;
          text-align: center;
          border-radius: 0.5rem;
          background: white;
          box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
        }
        .queue-card h4 {
          margin: 0 0 0.5rem;
          font-size: 1rem;
          color: var(--dark);
        }
        .queue-count {
          font-size: 1.8rem;
          font-weight: 700;
          margin: 0.5rem 0;
        }
        .queue-status {
          padding: 0.25rem 0.75rem;
          border-radius: 1rem;
          font-size: 0.75rem;
          font-weight: 600;
          display: inline-block;
        }
        .status-vacant {
          background: var(--success-bg);
          color: var(--secondary);
        }
        .status-busy {
          background: var(--warning-bg);
          color: var(--warning);
        }
        .status-full {
          background: var(--danger-bg);
          color: var(--danger);
        }
        .queue-recommendation {
          margin-top: 1.5rem;
          padding: 1rem 1.5rem;
          background: var(--success-bg);
          border-left: 4px solid var(--secondary);
          border-radius: 0.5rem;
          font-weight: 500;
          display: none;
        }
        .queue-recommendation.show {
          display: block;
        }
        .queue-recommendation .queue-number {
          font-weight: 700;
          color: var(--secondary);
        }
        .sensor-status {
          display: flex;
          justify-content: space-between;
          grid-column: 1 / -1;
        }
        .sensor-card {
          flex: 1;
          margin: 0 0.75rem;
          padding: 1rem;
          text-align: center;
        }
        .footer {
          text-align: center;
          padding: 1.5rem;
          color: var(--gray);
          font-size: 0.875rem;
          margin-top: 2rem;
          display: flex;
          justify-content: space-between;
          align-items: center;
        }
        .system-info {
          text-align: right;
        }
        @media (max-width: 768px) {
          .stat-value {
            font-size: 2.5rem;
          }
          .sensor-status {
            flex-direction: column;
          }
          .sensor-card {
            margin: 0.5rem 0;
          }
        }
      </style>
    </head>
    <body id="app-body">
      <header>
        <h1>Smart Queue Monitor</h1>
      </header>
      <div class="container">
        <div class="dashboard">
          <div class="card stat-card">
            <h3>Current Queue Length</h3>
            <div class="stat-value queue-value" id="queueLength">0</div>
            <div id="queueStatus">
              <span class="status-indicator status-ok" id="queueIndicator"></span>
              <span id="queueStatusText">Normal</span>
            </div>
            <div class="stats-small">
              <div class="stat-small">
                <span class="stat-small-value" id="avgQueueLength">0</span>
                <span class="stat-small-label">Avg. Length</span>
              </div>
              <div class="stat-small">
                <span class="stat-small-value" id="maxQueueLength">0</span>
                <span class="stat-small-label">Max Length</span>
              </div>
            </div>
          </div>
          <div class="card stat-card">
            <h3>Total Entries</h3>
            <div class="stat-value entries-value" id="totalEntries">0</div>
          </div>
          <div class="card stat-card">
            <h3>Total Exits</h3>
            <div class="stat-value exits-value" id="totalExits">0</div>
          </div>
          <div class="card chart-card chart-container">
            <h3>Queue Trend</h3>
            <canvas id="queueChart"></canvas>
          </div>
          
          <div class="card all-queues-section">
            <h3>All Queues Status</h3>
            <div class="queues-grid" id="queuesGrid">
              <!-- Queue cards will be dynamically inserted here -->
            </div>
            <div class="queue-recommendation" id="queueRecommendation">
              Suggested shortest queue: <span class="queue-number" id="shortestQueue">1</span>
            </div>
          </div>
        </div>
        
        <div class="alert" id="queueAlert">
          <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <circle cx="12" cy="12" r="10"></circle>
            <line x1="12" y1="8" x2="12" y2="12"></line>
            <line x1="12" y1="16" x2="12.01" y2="16"></line>
          </svg>
          <span>Queue alert: Length has exceeded the threshold of 5! Please redirect to queue <span id="redirectQueueNumber">-</span></span>
        </div>
      </div>
      
      <div class="footer">
        <div>Smart Queue Monitor v1.1 | Last updated: <span id="lastUpdated">-</span></div>
        <div class="system-info">
          <div>Sensor status: <span id="sensorStatus">OK</span></div>
        </div>
      </div>
      
      <script>
        const queueAlertThreshold = 5;
        const numQueues = 5;
        const ctx = document.getElementById('queueChart').getContext('2d');
        const queueChart = new Chart(ctx, {
          type: 'line',
          data: {
            labels: [],
            datasets: [{
              label: 'Queue Length',
              data: [],
              borderColor: '#3b82f6',
              backgroundColor: 'rgba(59, 130, 246, 0.1)',
              fill: true,
              tension: 0.4,
              pointRadius: 4,
              pointBackgroundColor: '#3b82f6'
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
              x: { 
                grid: {
                  display: false
                }
              },
              y: { 
                beginAtZero: true,
                suggestedMax: 10,
                ticks: {
                  stepSize: 1
                }
              }
            },
            plugins: {
              legend: {
                display: false
              },
              tooltip: {
                backgroundColor: 'rgba(31, 41, 55, 0.8)',
                padding: 10,
                cornerRadius: 6
              }
            }
          }
        });

        // Generate queue cards on page load
        function generateQueueCards() {
          const queuesGrid = document.getElementById('queuesGrid');
          queuesGrid.innerHTML = '';
          
          for (let i = 0; i < numQueues; i++) {
            const queueCard = document.createElement('div');
            queueCard.className = 'queue-card';
            queueCard.innerHTML = `
              <h4>Queue ${i + 1}</h4>
              <div class="queue-count" id="queue${i}Length">0</div>
              <div class="queue-status status-vacant" id="queue${i}Status">Vacant</div>
            `;
            queuesGrid.appendChild(queueCard);
          }
        }
        
        generateQueueCards();

        function updateQueueStatus(queueLength) {
          const indicator = document.getElementById('queueIndicator');
          const statusText = document.getElementById('queueStatusText');
          
          if (queueLength >= queueAlertThreshold) {
            indicator.className = 'status-indicator status-alert';
            statusText.textContent = 'Alert';
            statusText.style.color = '#ef4444';
          } else if (queueLength >= Math.floor(queueAlertThreshold * 0.6)) {
            indicator.className = 'status-indicator status-warning';
            statusText.textContent = 'Busy';
            statusText.style.color = '#f59e0b';
          } else {
            indicator.className = 'status-indicator status-ok';
            statusText.textContent = 'Normal';
            statusText.style.color = '#10b981';
          }
        }
        
        function updateAllQueuesStatus(queues) {
          let shortestQueue = 0;
          let minLength = queues[0];
          
          for (let i = 0; i < queues.length; i++) {
            const queueLengthEl = document.getElementById(queue${i}Length);
            const queueStatusEl = document.getElementById(queue${i}Status);
            
            if (queueLengthEl) queueLengthEl.textContent = queues[i];
            
            if (queueStatusEl) {
              if (queues[i] >= queueAlertThreshold) {
                queueStatusEl.className = 'queue-status status-full';
                queueStatusEl.textContent = 'Full';
              } else if (queues[i] >= Math.floor(queueAlertThreshold * 0.6)) {
                queueStatusEl.className = 'queue-status status-busy';
                queueStatusEl.textContent = 'Busy';
              } else {
                queueStatusEl.className = 'queue-status status-vacant';
                queueStatusEl.textContent = 'Vacant';
              }
            }
            
            if (queues[i] < minLength) {
              minLength = queues[i];
              shortestQueue = i;
            }
          }
          
          // Update recommendation
          const recommendation = document.getElementById('queueRecommendation');
          const shortestQueueEl = document.getElementById('shortestQueue');
          
          if (shortestQueueEl) shortestQueueEl.textContent = shortestQueue + 1; // +1 for 1-based numbering
          
          // Only show recommendation if main queue (0) is busy
          if (queues[0] >= Math.floor(queueAlertThreshold * 0.6) && shortestQueue !== 0) {
            recommendation.classList.add('show');
          } else {
            recommendation.classList.remove('show');
          }
          
          // Update redirect queue in alert
          document.getElementById('redirectQueueNumber').textContent = shortestQueue + 1;
        }

        async function fetchData() {
          try {
            const response = await fetch('/data');
            const data = await response.json();
            
            // Update main stats
            document.getElementById('queueLength').textContent = data.queueLength;
            document.getElementById('totalEntries').textContent = data.totalEntries;
            document.getElementById('totalExits').textContent = data.totalExits;
            
            // Update queue analytics
            document.getElementById('avgQueueLength').textContent = data.avgQueueLength.toFixed(1);
            document.getElementById('maxQueueLength').textContent = data.maxQueueLength;
            
            // Update status
            updateQueueStatus(data.queueLength);
            
            // Update all queues
            updateAllQueuesStatus(data.queueLengths);

            // Update chart
            const now = new Date();
            const timeStr = now.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', second:'2-digit'});
            
            queueChart.data.labels.push(timeStr);
            queueChart.data.datasets[0].data.push(data.queueLength);
            
            if (queueChart.data.labels.length > 20) {
              queueChart.data.labels.shift();
              queueChart.data.datasets[0].data.shift();
            }
            queueChart.update();

            // Update alert
            const alert = document.getElementById('queueAlert');
            if (data.queueLength >= queueAlertThreshold) {
              alert.classList.add('show');
            } else {
              alert.classList.remove('show');
            }
            
            // Update last updated time
            document.getElementById('lastUpdated').textContent = timeStr;
            
            // Update sensor status
            document.getElementById('sensorStatus').textContent = data.sensorStatus;
          } catch (error) {
            console.error('Error fetching data:', error);
            document.getElementById('sensorStatus').textContent = 'Connection Error';
          }
        }

        // Initial data fetch
        fetchData();
        
        // Set up periodic data fetching
        setInterval(fetchData, 2000);
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// Send dynamic JSON data
void handleData() {
  // For simulation purposes, generate random length for other queues
  // In a real system, these would come from sensors or other input
  for (int i = 1; i < numQueues; i++) {
    // For demo: slightly randomize other queue lengths
    if (random(100) < 20) { // 20% chance to change
      int change = random(3) - 1; // -1, 0, or 1
      queueLengths[i] = max(0, queueLengths[i] + change);
    }
    
    // Update busy status for LEDs
    queueBusy[i] = (queueLengths[i] >= queueAlertThreshold);
  }
  
  // Determine if sensors are working properly
  long distance1 = measureDistance(trigPin1, echoPin1);
  long distance2 = measureDistance(trigPin2, echoPin2);
  bool sensorOk = (distance1 < 9999 && distance2 < 9999);
  
  String json = "{";
  json += "\"queueLength\":" + String(queueLength) + ",";
  json += "\"totalEntries\":" + String(totalEntries) + ",";
  json += "\"totalExits\":" + String(totalExits) + ",";
  json += "\"maxQueueLength\":" + String(maxQueueLength) + ",";
  json += "\"avgQueueLength\":" + String(avgQueueLength) + ",";
  json += "\"queueLengths\":[";
  
  for (int i = 0; i < numQueues; i++) {
    json += String(queueLengths[i]);
    if (i < numQueues - 1) json += ",";
  }
  
  json += "],";
  json += "\"entrySensorDistance\":" + String(distance1) + ",";
  json += "\"exitSensorDistance\":" + String(distance2) + ",";
  json += "\"sensorStatus\":\"" + String(sensorOk ? "OK" : "Error") + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Smart Queue Monitor...");
  Serial.println("----------------------------------------");
  Serial.println("System Configuration:");
  Serial.println("* Number of queues: 5");
  Serial.println("* Queue alert threshold: " + String(queueAlertThreshold));
  
  // Setup pins
  Serial.println("\nPin Configuration:");
  
  // Ultrasonic sensor pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  Serial.println("* Entry Sensor: Trigger Pin=" + String(trigPin1) + ", Echo Pin=" + String(echoPin1));
  Serial.println("* Exit Sensor:  Trigger Pin=" + String(trigPin2) + ", Echo Pin=" + String(echoPin2));
  
  // Setup buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  Serial.println("* Buzzer Pin: " + String(buzzerPin));
  
  // Setup LED pins
  Serial.println("\nLED Queue Indicators:");
  for (int i = 0; i < numQueues; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
    Serial.println("* Queue " + String(i+1) + " LED: Pin=" + String(ledPins[i]));
  }

  // Connect to WiFi
  Serial.println("\nConnecting to WiFi network: " + String(ssid));
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
  Serial.println("----------------------------------------");
  Serial.println("Smart Queue Monitor is running!");
  Serial.println("----------------------------------------");
}

void loop() {
  server.handleClient();

  // Entry detection
  long distance1 = measureDistance(trigPin1, echoPin1);
  bool entryDetected = (distance1 > 0 && distance1 < distanceThreshold);
  
  if (entryDetected && !lastStateEntry) {
    queueLength++;
    totalEntries++;
    Serial.println("[ENTRY] Person entered queue. Current length: " + String(queueLength));
    updateQueueStats();
  }
  lastStateEntry = entryDetected;

  // Exit detection
  long distance2 = measureDistance(trigPin2, echoPin2);
  bool exitDetected = (distance2 > 0 && distance2 < distanceThreshold);
  
  if (exitDetected && !lastStateExit && queueLength > 0) {
    queueLength--;
    totalExits++;
    Serial.println("[EXIT] Person left queue. Current length: " + String(queueLength));
    updateQueueStats();
  }
  lastStateExit = exitDetected;

  // Queue redirection logic
  if (queueLength >= queueAlertThreshold) {
    int shortestQueueIndex = findShortestQueue();
    
    // Buzzer activation
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    
    if (shortestQueueIndex != 0 && queueLengths[shortestQueueIndex] < queueAlertThreshold) {
      Serial.println("[ALERT] Queue length threshold exceeded! Redirecting to Queue " + String(shortestQueueIndex + 1));
      Serial.println("[REDIRECT] Please direct customers to Queue " + String(shortestQueueIndex + 1) + " (Length: " + String(queueLengths[shortestQueueIndex]) + ")");
    } else {
      Serial.println("[ALERT] Queue length threshold exceeded! All queues are busy.");
    }
  }
  
  // Update LED status for all queues
  updateLEDs();
  
  // Print queue status to serial monitor every 10 seconds for debugging
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000) {
    lastStatusTime = millis();
    
    Serial.println("\n----------------------------------------");
    Serial.println("QUEUE STATUS UPDATE:");
    for (int i = 0; i < numQueues; i++) {
      String status = queueBusy[i] ? "BUSY (LED Blinking)" : "VACANT (LED Off)";
      Serial.println("Queue " + String(i+1) + ": Length=" + String(queueLengths[i]) + ", Status=" + status);
    }
    
    // Print sensor readings
    long d1 = measureDistance(trigPin1, echoPin1);
    long d2 = measureDistance(trigPin2, echoPin2);
    Serial.println("\nSENSOR READINGS:");
    Serial.println("Entry Sensor: " + String(d1) + " cm");
    Serial.println("Exit Sensor: " + String(d2) + " cm");
    Serial.println("----------------------------------------\n");
  }
  
  delay(200);  // Small delay to stabilize readings
}
