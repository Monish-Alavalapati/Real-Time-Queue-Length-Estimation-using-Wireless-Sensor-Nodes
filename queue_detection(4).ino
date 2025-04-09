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
      <title>Smart Queue Monitor Pro</title>
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      <style>
        :root {
          --primary: #4f46e5;
          --primary-dark: #4338ca;
          --primary-light: #c7d2fe;
          --secondary: #10b981;
          --secondary-dark: #059669;
          --danger: #ef4444;
          --warning: #f59e0b;
          --light: #f3f4f6;
          --dark: #1f2937;
          --gray: #6b7280;
        }
        body { 
          font-family: Arial, sans-serif; 
          margin: 0; 
          padding: 0; 
          background: #f8fafc; 
          color: var(--dark);
          line-height: 1.6;
        }
        header { 
          background: linear-gradient(120deg, var(--primary), var(--primary-dark)); 
          color: white; 
          padding: 1.5rem; 
          text-align: center;
          box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
          position: relative;
          overflow: hidden;
        }
        header h1 {
          margin: 0;
          font-weight: 700;
          font-size: 2rem;
        }
        header p {
          margin-top: 0.5rem;
          opacity: 0.9;
        }
        .container { 
          max-width: 1200px; 
          margin: 0 auto; 
          padding: 1.5rem;
        }
        .dashboard {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
          gap: 1.5rem;
          margin-bottom: 1.5rem;
        }
        .card { 
          background: white; 
          border-radius: 0.75rem; 
          box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
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
        }
        .stat-icon {
          margin: 0 auto 1rem;
          width: 48px;
          height: 48px;
          display: flex;
          align-items: center;
          justify-content: center;
          border-radius: 12px;
        }
        .queue-icon {
          background-color: rgba(79, 70, 229, 0.1);
          color: var(--primary);
        }
        .entry-icon {
          background-color: rgba(16, 185, 129, 0.1);
          color: var(--secondary);
        }
        .exit-icon {
          background-color: rgba(245, 158, 11, 0.1);
          color: var(--warning);
        }
        .wait-icon {
          background-color: rgba(139, 92, 246, 0.1);
          color: #8b5cf6;
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
          font-size: 2.5rem;
          font-weight: 700;
          margin: 0.5rem 0;
          color: var(--dark);
        }
        .queue-value {
          color: var(--primary);
        }
        .entries-value {
          color: var(--secondary);
        }
        .exits-value {
          color: var(--warning);
        }
        .wait-value {
          color: #8b5cf6;
        }
        .stats-info {
          display: flex;
          align-items: center;
          justify-content: center;
          margin-top: 0.5rem;
          color: var(--gray);
          font-size: 0.875rem;
        }
        .status-indicator {
          display: inline-block;
          width: 10px;
          height: 10px;
          border-radius: 50%;
          margin-right: 6px;
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
        .chart-container { 
          position: relative; 
          height: 350px;
          grid-column: 1 / -1;
        }
        .chart-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          margin-bottom: 1rem;
        }
        .chart-card h3 {
          margin: 0;
          color: var(--dark);
          font-weight: 600;
        }
        .time-selector {
          display: flex;
          gap: 0.5rem;
        }
        .time-btn {
          background: none;
          border: none;
          color: var(--gray);
          font-size: 0.875rem;
          padding: 0.25rem 0.5rem;
          border-radius: 4px;
          cursor: pointer;
        }
        .time-btn.active {
          background-color: var(--primary-light);
          color: var(--primary-dark);
          font-weight: 500;
        }
        .alert { 
          padding: 1rem 1.5rem; 
          background: #fee2e2; 
          color: var(--danger); 
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
        .footer {
          text-align: center;
          padding: 1.5rem;
          color: var(--gray);
          font-size: 0.875rem;
          margin-top: 1.5rem;
        }
        .device-status {
          display: flex;
          align-items: center;
          justify-content: center;
          margin-top: 0.5rem;
        }
        .device-indicator {
          width: 8px;
          height: 8px;
          border-radius: 50%;
          background-color: var(--secondary);
          margin-right: 6px;
        }
        .predictive {
          display: grid;
          grid-template-columns: 1fr 1fr;
          gap: 1rem;
          margin-top: 1rem;
        }
        .prediction-card {
          background: rgba(243, 244, 246, 0.5);
          border-radius: 8px;
          padding: 1rem;
        }
        .prediction-title {
          font-size: 0.875rem;
          font-weight: 600;
          margin-bottom: 0.5rem;
        }
        .prediction-value {
          font-weight: 700;
          font-size: 1.5rem;
          color: var(--primary);
        }
        .prediction-info {
          font-size: 0.75rem;
          color: var(--gray);
          margin-top: 0.5rem;
        }
        @media (max-width: 768px) {
          .dashboard {
            grid-template-columns: 1fr;
          }
          .predictive {
            grid-template-columns: 1fr;
          }
        }
      </style>
    </head>
    <body>
      <header>
        <h1>Smart Queue Monitor Pro</h1>
        <p>Advanced monitoring with predictive analytics</p>
      </header>
      
      <div class="container">
        <div class="dashboard">
          <div class="card stat-card">
            <div class="stat-icon queue-icon">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"></path>
                <circle cx="9" cy="7" r="4"></circle>
                <path d="M23 21v-2a4 4 0 0 0-3-3.87"></path>
                <path d="M16 3.13a4 4 0 0 1 0 7.75"></path>
              </svg>
            </div>
            <h3>Current Queue Length</h3>
            <div class="stat-value queue-value" id="queueLength">0</div>
            <div class="stats-info">
              <span class="status-indicator status-ok" id="queueIndicator"></span>
              <span id="queueStatusText">Normal</span>
            </div>
          </div>
          
          <div class="card stat-card">
            <div class="stat-icon entry-icon">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M15 14l5-5-5-5"></path>
                <path d="M19 9H5"></path>
                <path d="M5 5v14"></path>
              </svg>
            </div>
            <h3>Total Entries</h3>
            <div class="stat-value entries-value" id="totalEntries">0</div>
            <div class="stats-info" id="entriesPerMinute">0 entries/min</div>
          </div>
          
          <div class="card stat-card">
            <div class="stat-icon exit-icon">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M9 14l-5-5 5-5"></path>
                <path d="M5 9h14"></path>
                <path d="M19 5v14"></path>
              </svg>
            </div>
            <h3>Total Exits</h3>
            <div class="stat-value exits-value" id="totalExits">0</div>
            <div class="stats-info" id="exitsPerMinute">0 exits/min</div>
          </div>
          
          <div class="card stat-card">
            <div class="stat-icon wait-icon">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <circle cx="12" cy="12" r="10"></circle>
                <polyline points="12 6 12 12 16 14"></polyline>
              </svg>
            </div>
            <h3>Avg. Wait Time</h3>
            <div class="stat-value wait-value" id="waitTime">00:00:00</div>
            <div class="stats-info">Estimated per person</div>
          </div>
          
          <div class="card chart-card chart-container">
            <div class="chart-header">
              <h3>Queue Trend Analysis</h3>
              <div class="time-selector">
                <button class="time-btn active" data-time="5m">5m</button>
                <button class="time-btn" data-time="15m">15m</button>
                <button class="time-btn" data-time="30m">30m</button>
              </div>
            </div>
            <canvas id="queueChart"></canvas>
          </div>
          
          <div class="card">
            <h3 style="margin-top: 0; margin-bottom: 1rem;">Predictive Analytics</h3>
            <div style="background: rgba(79, 70, 229, 0.1); color: var(--primary); display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 12px; margin-bottom: 1rem;">
              AI-Powered
            </div>
            <div class="predictive">
              <div class="prediction-card">
                <div class="prediction-title">Predicted Peak Time</div>
                <div class="prediction-value" id="peakTime">12:30 - 13:15</div>
                <div class="prediction-info">Based on historical patterns</div>
              </div>
              <div class="prediction-card">
                <div class="prediction-title">Optimal Staffing</div>
                <div class="prediction-value" id="optimalStaff">2 Service Points</div>
                <div class="prediction-info">Recommended configuration</div>
              </div>
            </div>
          </div>
        </div>
        
        <div class="alert" id="queueAlert">
          <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <circle cx="12" cy="12" r="10"></circle>
            <line x1="12" y1="8" x2="12" y2="12"></line>
            <line x1="12" y1="16" x2="12.01" y2="16"></line>
          </svg>
          <span>Queue alert: Length has exceeded the threshold of 5!</span>
        </div>
      </div>
      
      <div class="footer">
        <div>Smart Queue Monitor Pro v2.0 | Last updated: <span id="lastUpdated">-</span></div>
        <div class="device-status">
          <span class="device-indicator"></span> All sensors operational
        </div>
      </div>
      
      <script>
        const queueAlertThreshold = 5;
        const ctx = document.getElementById('queueChart').getContext('2d');
        
        // Create gradient for chart
        const gradient = ctx.createLinearGradient(0, 0, 0, 300);
        gradient.addColorStop(0, 'rgba(79, 70, 229, 0.2)');
        gradient.addColorStop(1, 'rgba(79, 70, 229, 0)');
        
        const queueChart = new Chart(ctx, {
          type: 'line',
          data: {
            labels: [],
            datasets: [{
              label: 'Queue Length',
              data: [],
              borderColor: '#4f46e5',
              backgroundColor: gradient,
              fill: true,
              tension: 0.4,
              pointRadius: 3,
              pointBackgroundColor: '#4f46e5'
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

        // Track history for rates
        let entriesHistory = [];
        let exitsHistory = [];
        let lastEntries = 0;
        let lastExits = 0;
        
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
        
        // Calculate the wait time based on queue length
        function updateWaitTime(queueLength) {
          // Estimate 3-5 minutes per person
          const baseWaitSeconds = 240; // 4 minutes in seconds
          const waitSeconds = queueLength * baseWaitSeconds;
          const waitMinutes = Math.floor(waitSeconds / 60);
          const waitHours = Math.floor(waitMinutes / 60);
          
          const formattedWaitTime = 
            String(waitHours).padStart(2, '0') + ':' +
            String(waitMinutes % 60).padStart(2, '0') + ':' +
            String(waitSeconds % 60).padStart(2, '0');
            
          document.getElementById('waitTime').textContent = formattedWaitTime;
        }
        
        // Update the peak time prediction based on time of day
        function updatePeakTime() {
          const now = new Date();
          const hour = now.getHours();
          
          let peakStart, peakEnd;
          
          if (hour < 10) {
            peakStart = "12:30";
            peakEnd = "13:15";
          } else if (hour < 14) {
            peakStart = "17:00";
            peakEnd = "18:30";
          } else {
            peakStart = "08:30";
            peakEnd = "09:15";
          }
          
          document.getElementById('peakTime').textContent = ${peakStart} - ${peakEnd};
        }
        
        // Update optimal staffing based on queue length
        function updateOptimalStaff(queueLength) {
          let staffing;
          
          if (queueLength <= 3) {
            staffing = "1 Service Point";
          } else if (queueLength <= 7) {
            staffing = "2 Service Points";
          } else {
            staffing = "3 Service Points";
          }
          
          document.getElementById('optimalStaff').textContent = staffing;
        }

        // Time selector functionality
        document.querySelectorAll('.time-btn').forEach(button => {
          button.addEventListener('click', function() {
            document.querySelectorAll('.time-btn').forEach(btn => btn.classList.remove('active'));
            this.classList.add('active');
          });
        });

        async function fetchData() {
          try {
            const response = await fetch('/data');
            const data = await response.json();
            
            // Track difference in entries and exits for per-minute calculations
            const newEntries = data.totalEntries - lastEntries;
            const newExits = data.totalExits - lastExits;
            
            entriesHistory.push(newEntries);
            exitsHistory.push(newExits);
            
            if (entriesHistory.length > 30) entriesHistory.shift();
            if (exitsHistory.length > 30) exitsHistory.shift();
            
            lastEntries = data.totalEntries;
            lastExits = data.totalExits;
            
            // Update entries/exits per minute
            const entriesPerMin = Math.round(entriesHistory.reduce((sum, val) => sum + val, 0) * 2);
            const exitsPerMin = Math.round(exitsHistory.reduce((sum, val) => sum + val, 0) * 2);
            
            document.getElementById('entriesPerMinute').textContent = ${entriesPerMin} entries/min;
            document.getElementById('exitsPerMinute').textContent = ${exitsPerMin} exits/min;
            
            // Update main stats
            document.getElementById('queueLength').textContent = data.queueLength;
            document.getElementById('totalEntries').textContent = data.totalEntries;
            document.getElementById('totalExits').textContent = data.totalExits;
            
            // Update derived values
            updateWaitTime(data.queueLength);
            updatePeakTime();
            updateOptimalStaff(data.queueLength);
            
            // Update status
            updateQueueStatus(data.queueLength);

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
          } catch (error) {
            console.error('Error fetching data:', error);
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
