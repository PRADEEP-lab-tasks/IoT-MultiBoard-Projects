created by Pradeep

/*
  ESP32 CRO with Web Server + Adjustable Controls
  --------------------------------------------------
  Live waveform viewer with on-page controls for:
    - Time base (sample delay, like time/div)
    - Voltage scale (volts/div, display only)
    - Trigger level (visual line + simple trigger-on-rising-edge)
    - Pause/Resume

  1. Set WiFi SSID/password below
  2. Upload, open Serial Monitor to get the IP
  3. Open that IP in a browser

  Signal: connect to GPIO34 (0-3.3V range only).
*/

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

#define ADC_PIN 34
#define NUM_SAMPLES 300
#define VREF 3.3
#define ADC_MAX 4095

WebServer server(80);
int samples[NUM_SAMPLES];

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 CRO</title>
  <style>
    body { font-family: sans-serif; background:#111; color:#0f0; text-align:center; }
    canvas { background:#000; border:1px solid #0f0; margin-top:15px; }
    .controls { margin-top:15px; display:flex; justify-content:center; gap:25px; flex-wrap:wrap; }
    .ctrl-group { display:flex; flex-direction:column; align-items:center; }
    label { font-size:13px; margin-bottom:5px; }
    input[type=range] { width:160px; }
    button { background:#0f0; color:#000; border:none; padding:6px 14px; border-radius:4px; cursor:pointer; font-weight:bold; }
    button:hover { background:#3f3; }
    #status { margin-top:8px; font-size:13px; color:#0af; }
    #readout { font-size:14px; margin-top:5px; }
  </style>
</head>
<body>
  <h2>ESP32 Oscilloscope</h2>
  <canvas id="scope" width="600" height="300"></canvas>
  <div id="readout">
    Vpp: -- V &nbsp;|&nbsp; Vmax: -- V &nbsp;|&nbsp; Vmin: -- V &nbsp;|&nbsp; Vavg: -- V &nbsp;|&nbsp; Vrms: -- V<br>
    Amplitude: -- V &nbsp;|&nbsp; Frequency: -- Hz &nbsp;|&nbsp; Period: -- ms
  </div>
  <p id="status">Connecting...</p>

  <div class="controls">
    <div class="ctrl-group">
      <label>Time Base (sample delay: <span id="delayVal">50</span> us)</label>
      <input type="range" id="delaySlider" min="5" max="500" value="50" step="5">
    </div>
    <div class="ctrl-group">
      <label>Volts/Div: <span id="voltsDivVal">0.5</span> V</label>
      <input type="range" id="voltsDivSlider" min="0.1" max="1.0" value="0.5" step="0.1">
    </div>
    <div class="ctrl-group">
      <label>Trigger Level: <span id="trigVal">1.65</span> V</label>
      <input type="range" id="trigSlider" min="0" max="3.3" value="1.65" step="0.05">
    </div>
    <div class="ctrl-group">
      <label>&nbsp;</label>
      <button id="pauseBtn">Pause</button>
    </div>
  </div>

<script>
const canvas = document.getElementById('scope');
const ctx = canvas.getContext('2d');
const statusEl = document.getElementById('status');
const readoutEl = document.getElementById('readout');
const delaySlider = document.getElementById('delaySlider');
const delayVal = document.getElementById('delayVal');
const voltsDivSlider = document.getElementById('voltsDivSlider');
const voltsDivVal = document.getElementById('voltsDivVal');
const trigSlider = document.getElementById('trigSlider');
const trigVal = document.getElementById('trigVal');
const pauseBtn = document.getElementById('pauseBtn');

let paused = false;
let currentDelay = 50;
let voltsPerDiv = 0.5;
let triggerLevel = 1.65;

delaySlider.oninput = () => { currentDelay = delaySlider.value; delayVal.textContent = currentDelay; };
voltsDivSlider.oninput = () => { voltsPerDiv = parseFloat(voltsDivSlider.value); voltsDivVal.textContent = voltsPerDiv.toFixed(1); drawGrid(); };
trigSlider.oninput = () => { triggerLevel = parseFloat(trigSlider.value); trigVal.textContent = triggerLevel.toFixed(2); };
pauseBtn.onclick = () => { paused = !paused; pauseBtn.textContent = paused ? "Resume" : "Pause"; };

function drawGrid() {
  ctx.strokeStyle = "#033";
  ctx.lineWidth = 1;
  const divs = Math.round(3.3 / voltsPerDiv);
  const divHeight = canvas.height / divs;
  for (let i = 0; i <= divs; i++) {
    const y = i * divHeight;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(canvas.width, y);
    ctx.stroke();
  }
}

function findTriggerStart(voltages) {
  // simple rising-edge trigger: find first point crossing triggerLevel going up
  for (let i = 1; i < voltages.length; i++) {
    if (voltages[i-1] < triggerLevel && voltages[i] >= triggerLevel) {
      return i;
    }
  }
  return 0; // no trigger found, just show from start
}

function findAllRisingEdges(voltages, level) {
  const edges = [];
  for (let i = 1; i < voltages.length; i++) {
    if (voltages[i-1] < level && voltages[i] >= level) edges.push(i);
  }
  return edges;
}

function computeStats(voltages, sampleDelayUs) {
  const vmax = Math.max(...voltages);
  const vmin = Math.min(...voltages);
  const vpp = vmax - vmin;
  const amplitude = vpp / 2;
  const vavg = voltages.reduce((a,b) => a+b, 0) / voltages.length;
  const vrms = Math.sqrt(voltages.reduce((a,b) => a + b*b, 0) / voltages.length);

  // Frequency via rising-edge trigger crossings at the midpoint (vavg)
  const edges = findAllRisingEdges(voltages, vavg);
  let freq = 0, periodMs = 0;
  if (edges.length >= 2) {
    const samplesPerCycle = (edges[edges.length-1] - edges[0]) / (edges.length - 1);
    const periodUs = samplesPerCycle * sampleDelayUs;
    periodMs = periodUs / 1000;
    freq = 1e6 / periodUs;
  }

  return { vmax, vmin, vpp, amplitude, vavg, vrms, freq, periodMs };
}

function drawWave(rawData) {
  const voltages = rawData.map(v => (v / 4095) * 3.3);

  ctx.clearRect(0, 0, canvas.width, canvas.height);
  drawGrid();

  const startIdx = findTriggerStart(voltages);
  const visible = voltages.slice(startIdx);

  ctx.strokeStyle = "#0f0";
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  const xStep = canvas.width / visible.length;
  for (let i = 0; i < visible.length; i++) {
    const y = canvas.height - (visible[i] / 3.3) * canvas.height;
    const x = i * xStep;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // trigger level line
  ctx.strokeStyle = "#f80";
  ctx.setLineDash([4, 4]);
  const trigY = canvas.height - (triggerLevel / 3.3) * canvas.height;
  ctx.beginPath();
  ctx.moveTo(0, trigY);
  ctx.lineTo(canvas.width, trigY);
  ctx.stroke();
  ctx.setLineDash([]);

  const stats = computeStats(voltages, parseInt(currentDelay));
  const freqText = stats.freq > 0 ? stats.freq.toFixed(1) + " Hz" : "-- (no repeat detected)";
  const periodText = stats.periodMs > 0 ? stats.periodMs.toFixed(3) + " ms" : "--";

  readoutEl.innerHTML =
    `Vpp: ${stats.vpp.toFixed(2)} V &nbsp;|&nbsp; Vmax: ${stats.vmax.toFixed(2)} V &nbsp;|&nbsp; ` +
    `Vmin: ${stats.vmin.toFixed(2)} V &nbsp;|&nbsp; Vavg: ${stats.vavg.toFixed(2)} V &nbsp;|&nbsp; Vrms: ${stats.vrms.toFixed(2)} V<br>` +
    `Amplitude: ${stats.amplitude.toFixed(2)} V &nbsp;|&nbsp; Frequency: ${freqText} &nbsp;|&nbsp; Period: ${periodText}`;
}

async function fetchData() {
  if (!paused) {
    try {
      const res = await fetch('/data?delay=' + currentDelay);
      const data = await res.json();
      drawWave(data);
      statusEl.textContent = "Live | Samples: " + data.length;
    } catch (e) {
      statusEl.textContent = "Connection lost, retrying...";
    }
  }
  setTimeout(fetchData, 50);
}

drawGrid();
fetchData();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

void handleData() {
  int sampleDelayUs = 50;
  if (server.hasArg("delay")) {
    sampleDelayUs = server.arg("delay").toInt();
    if (sampleDelayUs < 5) sampleDelayUs = 5;
    if (sampleDelayUs > 1000) sampleDelayUs = 1000;
  }

  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = analogRead(ADC_PIN);
    delayMicroseconds(sampleDelayUs);
  }

  String json = "[";
  for (int i = 0; i < NUM_SAMPLES; i++) {
    json += samples[i];
    if (i < NUM_SAMPLES - 1) json += ",";
  }
  json += "]";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Open this in your browser: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}
