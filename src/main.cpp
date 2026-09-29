#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

constexpr int LDR_PIN = 34;
constexpr int LED_PIN = 18;

// The photoresistor reading rises as the room gets darker.
constexpr int BRIGHT_READING = 150;
constexpr int DARK_READING = 4000;

WebServer server(80);

bool autoMode = true;
int manualBrightness = 50;
int currentBrightness = 0;
int ldrReading = 0;

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Adaptive Smart Lamp</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #edf3f1;
      color: #20352f;
      text-align: center;
      padding: 30px 12px;
    }
    .card {
      max-width: 520px;
      margin: auto;
      padding: 28px;
      background: white;
      border-radius: 20px;
      box-shadow: 0 8px 25px #0002;
    }
    button {
      padding: 12px 22px;
      margin: 8px;
      border: 0;
      border-radius: 10px;
      background: #16856b;
      color: white;
      font-size: 16px;
      cursor: pointer;
    }
    button.selected {
      background: #154c40;
    }
    input[type="range"] {
      width: 85%;
    }
    .value {
      font-size: 34px;
      font-weight: bold;
      margin: 14px;
    }
    .note {
      color: #63736d;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 Adaptive Smart Lamp</h1>
    <p>Choose automatic brightness or control the lamp yourself.</p>

    <button id="autoButton" onclick="setMode('auto')">AUTO</button>
    <button id="manualButton" onclick="setMode('manual')">MANUAL</button>

    <p id="mode">Mode: connecting...</p>
    <p>Photoresistor reading</p>
    <div class="value" id="sensor">--</div>

    <div id="manualControls">
      <p>Manual brightness</p>
      <input
        id="slider"
        type="range"
        min="0"
        max="100"
        value="50"
        oninput="setBrightness(this.value)"
      >
      <div class="value"><span id="sliderValue">50</span>%</div>
    </div>

    <p class="value"><span id="brightness">--</span>%</p>
    <p id="lamp">Lamp: --</p>
    <p class="note">
      In AUTO mode, the lamp gets brighter when the room gets darker.
    </p>
  </div>

  <script>
    async function setMode(mode) {
      await fetch('/mode?value=' + mode);
      updateStatus();
    }

    async function setBrightness(value) {
      document.getElementById('sliderValue').textContent = value;
      await fetch('/brightness?value=' + value);
      updateStatus();
    }

    async function updateStatus() {
      try {
        const response = await fetch('/status');
        const data = await response.json();

        document.getElementById('mode').textContent =
          'Mode: ' + (data.auto ? 'AUTO' : 'MANUAL');
        document.getElementById('sensor').textContent = data.ldr;
        document.getElementById('brightness').textContent = data.brightness;
        document.getElementById('lamp').textContent =
          'Lamp: ' + (data.brightness > 0 ? 'ON' : 'OFF');

        document.getElementById('manualControls').style.display =
          data.auto ? 'none' : 'block';

        document.getElementById('autoButton').className =
          data.auto ? 'selected' : '';
        document.getElementById('manualButton').className =
          data.auto ? '' : 'selected';

        document.getElementById('slider').value = data.manual;
        document.getElementById('sliderValue').textContent = data.manual;
      } catch (error) {
        document.getElementById('mode').textContent =
          'Waiting for ESP32 connection...';
      }
    }

    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

// Temporary on/off check: 0% is OFF; any value above 0% is ON.
void applyBrightness(int percent) {
  percent = constrain(percent, 0, 100);
  currentBrightness = percent;
  digitalWrite(LED_PIN, percent > 0 ? HIGH : LOW);
}

void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleMode() {
  String value = server.arg("value");

  if (value == "auto") {
    autoMode = true;
  } else if (value == "manual") {
    autoMode = false;
    applyBrightness(manualBrightness);
  }

  server.send(200, "text/plain", "OK");
}

void handleBrightness() {
  if (server.hasArg("value")) {
    manualBrightness = constrain(server.arg("value").toInt(), 0, 100);

    if (!autoMode) {
      applyBrightness(manualBrightness);
    }
  }

  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"auto\":";
  json += (autoMode ? "true" : "false");
  json += ",\"ldr\":";
  json += String(ldrReading);
  json += ",\"brightness\":";
  json += String(currentBrightness);
  json += ",\"manual\":";
  json += String(manualBrightness);
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP32 Adaptive Smart Lamp starting...");

  pinMode(LED_PIN, OUTPUT);
  analogReadResolution(12);
  applyBrightness(0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/brightness", handleBrightness);
  server.on("/status", handleStatus);
  server.begin();

  Serial.println("Web server started.");
  Serial.println("Open http://localhost:8181 in your browser.");
}

void loop() {
  server.handleClient();

  static unsigned long lastUpdate = 0;
  static unsigned long lastPrint = 0;

  if (millis() - lastUpdate >= 250) {
    lastUpdate = millis();
    ldrReading = analogRead(LDR_PIN);

    if (autoMode) {
      int brightness = map(
        ldrReading,
        BRIGHT_READING,
        DARK_READING,
        0,
        100
      );

      brightness = constrain(brightness, 0, 100);
      applyBrightness(brightness);
    }
  }

  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    Serial.printf(
      "LDR: %d | Mode: %s | Brightness: %d%%\r\n",
      ldrReading,
      autoMode ? "AUTO" : "MANUAL",
      currentBrightness
    );
  }
}