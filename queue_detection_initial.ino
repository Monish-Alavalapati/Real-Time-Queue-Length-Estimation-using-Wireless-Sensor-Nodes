#include <WiFi.h>
#include <ESPmDNS.h> 
#include <WebServer.h>
#include <mDNS.h>

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


// Serve HTML page with enhanced design (HTML/CSS/JS remains the same as provided)
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
          --primary: #2563eb;
          --primary-dark: #1d4ed8;
          --secondary: #4b5563;
          --success: #10b981;
          --danger: #ef4444;
          --warning: #f59e0b;
          --bg: #f3f4f6;
          --card: #ffffff;
          --text: #1f2937;
          --text-light: #6b7280;
          --border: #e5e7eb;
          --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
        }
        
        * {
          margin: 0;
          padding: 0;
          box-sizing: border-box;
        }
        
        body {
          font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif;
          background: var(--bg);
          color: var(--text);
          line-height: 1.5;
        }
        
        .container {
          max-width: 1200px;
          margin: 0 auto;
          padding: 20px;
        }
        
        header {
          background: var(--card);
          border-bottom: 1px solid var(--border);
          padding: 15px 0;
          box-shadow: var(--shadow);
          position: sticky;
          top: 0;
          z-index: 10;
        }
        
        .header-content {
          display: flex;
          align-items: center;
          justify-content: space-between;
        }
        
        .logo {
          display: flex;
          align-items: center;
          gap: 10px;
          font-size: 1.5rem;
          font-weight: 600;
          color: var(--primary);
        }
        
        .status {
          display: flex;
          align-items: center;
          gap: 8px;
          font-weight: 500;
        }
        
        .status-dot {
          width: 10px;
          height: 10px;
          border-radius: 50%;
          background: var(--success);
          animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
          0% { opacity: 1; }
          50% { opacity: 0.5; }
          100% { opacity: 1; }
        }
        
        .dashboard {
          margin-top: 20px;
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
          gap: 20px;
        }
        
        .card {
          background: var(--card);
          border-radius: 12px;
          box-shadow: var(--shadow);
          padding: 20px;
          transition: transform 0.2s, box-shadow 0.2s;
          overflow: hidden;
        }
        
        .card:hover {
          transform: translateY(-3px);
          box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
        }
        
        .card-header {
          display: flex;
          align-items: center;
          justify-content: space-between;
          margin-bottom: 15px;
        }
        
        .card-title {
          font-size: 1.1rem;
          font-weight: 600;
          color: var(--secondary);
        }
        
        .card-icon {
          font-size: 1.5rem;
          opacity: 0.7;
        }
        
        .card-value {
          font-size: 2.5rem;
          font-weight: 700;
          margin-bottom: 8px;
          color: var(--primary);
        }
        
        .card-trend {
          display: flex;
          align-items: center;
          gap: 5px;
          font-size: 0.9rem;
        }
        
        .trend-up {
          color: var(--success);
        }
        
        .trend-down {
          color: var(--danger);
        }
        
        .chart-container {
          grid-column: 1 / -1;
          height: 350px;
        }
        
        .alert {
          background: #fee2e2;
          color: var(--danger);
          border-left: 4px solid var(--danger);
          padding: 10px 15px;
          margin-top: 20px;
          border-radius: 6px;
          font-weight: 500;
          display: none;
        }

        .alert.show {
          display: block;
          animation: fadeIn 0.3s;
        }
        
        @keyframes fadeIn {
          from { opacity: 0; }
          to { opacity: 1; }
        }
        
        .gauge-container {
          position: relative;
          width: 100%;
          padding-top: 30px;
          display: flex;
          flex-direction: column;
          align-items: center;
        }
        
        .gauge {
          width: 140px;
          height: 70px;
          border-radius: 140px 140px 0 0;
          background: #eef2ff;
          position: relative;
          overflow: hidden;
          margin-bottom: 20px;
        }
        
        .gauge-fill {
          position: absolute;
          bottom: 0;
          left: 0;
          right: 0;
          background: linear-gradient(to top, var(--success), var(--primary));
          height: 0%;
          transition: height 0.5s;
        }
        
        .gauge-center {
          position: absolute;
          bottom: -20px;
          left: 50%;
          width: 20px;
          height: 20px;
          background: var(--card);
          border-radius: 50%;
          transform: translateX(-50%);
          z-index: 2;
        }
        
        .gauge-text {
          font-size: 1.2rem;
          font-weight: 600;
        }
        
        .insight {
          font-size: 0.9rem;
          color: var(--text-light);
          margin-top: 5px;
        }
        
        @media (max-width: 768px) {
          .dashboard {
            grid-template-columns: 1fr;
          }
          
          .logo {
            font-size: 1.2rem;
          }
          
          .card-value {
            font-size: 2rem;
          }
        }
      </style>
    </head>
    <body>
      <header>
        <div class="container header-content">
          <div class="logo">
            <span>📊</span> Smart Queue Monitor
          </div>
          <div class="status">
            <span class="status-dot"></span>
            <span id="connection-status">Connecting...</span>
          </div>
        </div>
      </header>
      
      <main class="container">
        <div class="dashboard">
          <div class="card">
            <div class="card-header">
              <div class="card-title">Current Queue</div>
              <div class="card-icon">👥</div>
            </div>
            <div class="card-value" id="queueLength">0</div>
            <div class="card-trend" id="queueTrend">
              <span id="queueTrendIcon">—</span>
              <span id="queueTrendText">Stable</span>
            </div>
          </div>
          
          <div class="card">
            <div class="card-header">
              <div class="card-title">Total Entries</div>
              <div class="card-icon">✅</div>
            </div>
            <div class="card-value" id="totalEntries">0</div>
            <div class="insight">People who entered today</div>
          </div>
          
          <div class="card">
            <div class="card-header">
              <div class="card-title">Total Exits</div>
              <div class="card-icon">🚪</div>
            </div>
            <div class="card-value" id="totalExits">0</div>
            <div class="insight">People who exited today</div>
          </div>
          
          <div class="card">
            <div class="card-header">
              <div class="card-title">Queue Status</div>
              <div class="card-icon">📈</div>
            </div>
            <div class="gauge-container">
              <div class="gauge">
                <div class="gauge-fill" id="gauge-fill"></div>
                <div class="gauge-center"></div>
              </div>
              <div class="gauge-text" id="gauge-text">Low</div>
            </div>
          </div>
          
          <div class="card chart-container">
            <div class="card-header">
              <div class="card-title">Queue History</div>
            </div>
            <canvas id="queueChart"></canvas>
          </div>
        </div>
        
        <div class="alert" id="queueAlert">
          <strong>Alert:</strong> Queue has exceeded threshold! Staff assistance recommended.
        </div>
      </main>
      
      <script>
        // Queue history chart
        const ctx = document.getElementById('queueChart').getContext('2d');
        const queueData = {
          labels: [],
          datasets: [{
            label: 'Queue Length',
            backgroundColor: 'rgba(37, 99, 235, 0.1)',
            borderColor: '#2563eb',
            borderWidth: 2,
            pointRadius: 3,
            pointBackgroundColor: '#1d4ed8',
            pointBorderColor: '#ffffff',
            pointHoverRadius: 5,
            tension: 0.2,
            data: [],
            fill: true
          }]
        };

        const config = {
          type: 'line',
          data: queueData,
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: {
                display: false
              },
              tooltip: {
                mode: 'index',
                intersect: false,
              }
            },
            scales: {
              x: { 
                grid: {
                  display: false,
                },
                ticks: {
                  maxRotation: 0,
                  autoSkip: true,
                  maxTicksLimit: 8 // Limit the number of visible time labels
                }
              },
              y: { 
                beginAtZero: true,
                grid: {
                  drawBorder: false,
                },
                ticks: {
                  precision: 0
                }
              }
            },
            interaction: {
              intersect: false,
              mode: 'index',
            },
            animation: {
              duration: 300 // Faster update animation
            }
          }
        };

        const queueChart = new Chart(ctx, config);
        
        // Variables to track trends
        let prevQueueLength = 0;
        // Make queueAlertThreshold accessible to JS if needed, otherwise it's defined in C++
        const queueAlertThresholdJS = 5; // Mirror C++ value if needed for JS logic
        
        // Update the gauge based on queue length
        function updateGauge(currentQueueLength) {
          const gaugeElement = document.getElementById('gauge-fill');
          const gaugeText = document.getElementById('gauge-text');
          let percentage = 0;
          let status = 'Low';
          const maxExpectedQueue = 10; // Define a reasonable max for gauge scaling
          
          if (currentQueueLength === 0) {
            percentage = 0;
            status = 'Empty';
          } else if (currentQueueLength < 3) {
            percentage = (currentQueueLength / maxExpectedQueue) * 100 * 0.8; // Scale gently
            status = 'Low';
          } else if (currentQueueLength < queueAlertThresholdJS) {
            percentage = 30 + (currentQueueLength / maxExpectedQueue) * 100 * 0.4;
            status = 'Moderate';
          } else { // At or above threshold
            percentage = 60 + Math.min(40, (currentQueueLength / maxExpectedQueue) * 100 * 0.3); // Cap increase
            status = 'High';
             // Make gauge color more warning-like if high
            gaugeElement.style.background = 'linear-gradient(to top, var(--warning), var(--danger))';
          }

          // Revert color if not high
          if (status !== 'High') {
              gaugeElement.style.background = 'linear-gradient(to top, var(--success), var(--primary))';
          }
          
          gaugeElement.style.height = Math.min(100, percentage) + '%'; // Cap at 100%
          gaugeText.textContent = status;
        }
        
        // Show alert when queue exceeds threshold
        function checkQueueAlert(currentQueueLength) {
          const alertElement = document.getElementById('queueAlert');
          if (currentQueueLength >= queueAlertThresholdJS) { // Use JS variable
            alertElement.classList.add('show');
          } else {
            alertElement.classList.remove('show');
          }
        }
        
        // Update queue trend indicator
        function updateQueueTrend(current, previous) {
          const trendIcon = document.getElementById('queueTrendIcon');
          const trendText = document.getElementById('queueTrendText');
          const trendContainer = trendIcon.parentElement; // Get the container div
          
          if (current > previous) {
            trendIcon.textContent = '↑';
            trendText.textContent = 'Increasing';
            trendContainer.className = 'card-trend trend-up';
          } else if (current < previous) {
            trendIcon.textContent = '↓';
            trendText.textContent = 'Decreasing';
            trendContainer.className = 'card-trend trend-down';
          } else {
            trendIcon.textContent = '—';
            trendText.textContent = 'Stable';
            trendContainer.className = 'card-trend'; // Reset class
          }
        }
        
        // Fetch and update data
        async function fetchData() {
          try {
            const response = await fetch('/data');
             if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();
            
            // Update stats
            document.getElementById('queueLength').textContent = data.queueLength;
            document.getElementById('totalEntries').textContent = data.totalEntries;
            document.getElementById('totalExits').textContent = data.totalExits;
            
            // Update the chart
            const now = new Date();
            const timeLabel = now.toLocaleTimeString(); // Use locale-specific time format

            // Only add data if queue length changed or labels are empty
            if (data.queueLength !== prevQueueLength || queueChart.data.labels.length === 0) {
                queueChart.data.labels.push(timeLabel);
                queueChart.data.datasets[0].data.push(data.queueLength);
            
                // Keep only the most recent 20 data points
                const maxDataPoints = 20;
                if (queueChart.data.labels.length > maxDataPoints) {
                  queueChart.data.labels.shift();
                  queueChart.data.datasets[0].data.shift();
                }
                
                queueChart.update();
            }
            
            // Update gauge
            updateGauge(data.queueLength);
            
            // Check for alerts
            checkQueueAlert(data.queueLength);
            
            // Update trend (only if queue length changed)
            if (data.queueLength !== prevQueueLength) {
                updateQueueTrend(data.queueLength, prevQueueLength);
                prevQueueLength = data.queueLength; // Update previous value only on change
            }
            
            // Update connection status
            document.getElementById('connection-status').textContent = 'Live';
            document.querySelector('.status-dot').style.background = 'var(--success)';

          } catch (error) {
            console.error('Error fetching data:', error);
            document.getElementById('connection-status').textContent = 'Offline';
            document.querySelector('.status-dot').style.background = 'var(--danger)';
            // Optionally update trend/gauge to show error state
            updateQueueTrend(-1, prevQueueLength); // Indicate error or unknown state
          }
        }
        
        // Initial data fetch
        fetchData();
        
        // Fetch data every 2 seconds (reduced frequency)
        setInterval(fetchData, 2000); // Fetch every 2000ms = 2 seconds
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
  digitalWrite(buzzerPin, LOW); // Ensure buzzer is off initially

  // Connect to WiFi using DHCP
  WiFi.mode(WIFI_STA); // Set WiFi mode to Station explicitly
  WiFi.begin(ssid, password);

  // <<< CHANGED: Removed static IP configuration. Using DHCP now. >>>
  // IPAddress staticIP(192, 168, 1, 200);
  // IPAddress gateway(192, 168, 137, 198);
  // IPAddress subnet(255, 255, 255, 0);
  // WiFi.config(staticIP, gateway, subnet);

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
        // Optional: Enter a deep sleep or blink an LED to indicate failure
        return; // Stop setup if WiFi fails
    }
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Print the IP assigned by DHCP

  // <<< CHANGED: Added a small delay before starting mDNS >>>
  delay(1000); 

  // Initialize mDNS
  Serial.print("Setting up mDNS responder with hostname 'queuemonitor'...");
  if (MDNS.begin("queuemonitor")) { // Hostname is "queuemonitor", access via queuemonitor.local
    Serial.println("\nmDNS responder started successfully.");
    Serial.println("You can now access the dashboard at: http://queuemonitor.local);
    MDNS.addService("http", "tcp", 80); // Announce the web server service
  } else {
    Serial.println("\nError setting up mDNS responder!");
  }

  // Server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);

  // Handle Not Found
   server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient(); // Handle incoming web requests
  //MDNS.update(); // Keep mDNS service updated (Good practice, though sometimes optional)

  // Sensor reading variables
  long distance1 = 9999; // Initialize with out-of-range value
  long distance2 = 9999; // Initialize with out-of-range value

  // --- Entry Detection ---
  distance1 = measureDistance(trigPin1, echoPin1);
  // Add basic filtering: check if reading is valid (not timeout/error value and within a reasonable max range)
  bool entryDetected = (distance1 > 0 && distance1 < distanceThreshold && distance1 != 9999); 

  if (entryDetected && !lastStateEntry) {
    queueLength++;
    totalEntries++;
    Serial.print("Entry Detected! Queue: "); Serial.println(queueLength);
    // Optional: Short beep on entry
    // digitalWrite(buzzerPin, HIGH);
    // delay(50);
    // digitalWrite(buzzerPin, LOW);
  }
  lastStateEntry = entryDetected;

  // --- Exit Detection ---
  // Add a small delay between sensor reads if they interfere
  // delay(10); 
  distance2 = measureDistance(trigPin2, echoPin2);
  // Add basic filtering: check if reading is valid
  bool exitDetected = (distance2 > 0 && distance2 < distanceThreshold && distance2 != 9999); 

  // Only decrement if someone is actually in the queue
  if (exitDetected && !lastStateExit && queueLength > 0) { 
    queueLength--;
    totalExits++;
     Serial.print("Exit Detected! Queue: "); Serial.println(queueLength);
     // Optional: Different short beep on exit
     // digitalWrite(buzzerPin, HIGH);
     // delay(20);
     // digitalWrite(buzzerPin, LOW);
     // delay(20);
     // digitalWrite(buzzerPin, HIGH);
     // delay(20);
     // digitalWrite(buzzerPin, LOW);
  }
  // Ensure queue length doesn't go negative due to sensor glitches
  if (queueLength < 0) {
      queueLength = 0;
  }
  lastStateExit = exitDetected;

  // --- Buzzer Control ---
  if (queueLength >= queueAlertThreshold) {
    // Maybe make the alert beep intermittent instead of constant?
    // Example: Beep for 100ms every second
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

  // Adjust loop delay - shorter delay for more responsive sensor readings,
  // but ensure it's long enough to prevent spurious double counts.
  // The `pulseIn` timeout already adds some delay.
  delay(200); // Reduced delay from 500ms to 200ms - adjust as needed
}
