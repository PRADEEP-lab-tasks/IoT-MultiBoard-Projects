#include <WiFi.h>
#include <WebServer.h>
#include <RF24.h>
#include <SPI.h>

struct Ctrl {
  uint16_t throttle;
  int16_t  roll;
  int16_t  pitch;
  int16_t  yaw;
  uint8_t  arming;
} __attribute__((packed));

RF24 radio(4, 5); // CE, CSN
const byte pipeAddr[6] = "DRONE"; // 5 bytes address

WebServer server(80);
Ctrl ctrl = {0, 0, 0, 0, 0};
unsigned long lastSend = 0;

const char* AP_SSID = "DroneGround";
const char* AP_PASS = "drone1234";

const char* INDEX_HTML = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Drone Controller</title>
<style>
  body{font-family:sans-serif;margin:20px}
  .row{display:flex;gap:16px;flex-wrap:wrap}
  .box{border:1px solid #ccc;padding:12px;border-radius:8px}
  input[type=range]{width:240px}
</style>
</head><body>
<h2>Drone Controller</h2>
<div class="row">
  <div class="box">
    <b>Throttle</b><br/>
    <input id="thr" type="range" min="0" max="1000" value="0"/>
  </div>
  <div class="box">
    <b>Roll</b><br/>
    <input id="roll" type="range" min="-500" max="500" value="0"/>
  </div>
  <div class="box">
    <b>Pitch</b><br/>
    <input id="pitch" type="range" min="-500" max="500" value="0"/>
  </div>
  <div class="box">
    <b>Yaw</b><br/>
    <input id="yaw" type="range" min="-500" max="500" value="0"/>
  </div>
</div>
<div class="box" style="margin-top:12px">
  <button id="arm">Arm</button>
  <button id="disarm">Disarm</button>
  <span id="status"></span>
</div>
<script>
let thr=0, roll=0, pitch=0, yaw=0, arming=0;
const send = ()=>{
  fetch(`/cmd?th=${thr}&rl=${roll}&pt=${pitch}&yw=${yaw}&ar=${arming}`)
    .then(r=>r.text()).then(t=>{ document.getElementById('status').innerText=t; })
    .catch(()=>{ document.getElementById('status').innerText='link error'; });
};
for (let id of ['thr','roll','pitch','yaw']) {
  document.getElementById(id).addEventListener('input', e=>{
    if(id==='thr') thr=+e.target.value;
    else if(id==='roll') roll=+e.target.value;
    else if(id==='pitch') pitch=+e.target.value;
    else if(id==='yaw') yaw=+e.target.value;
  });
}
document.getElementById('arm').onclick = ()=>{ arming=1; send(); };
document.getElementById('disarm').onclick = ()=>{ arming=0; thr=0; send(); };
setInterval(send, 50); // 20 Hz updates
</script>
</body></html>
)HTML";

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleCmd() {
  if (server.hasArg("th")) ctrl.throttle = (uint16_t) constrain(server.arg("th").toInt(), 0, 1000);
  if (server.hasArg("rl")) ctrl.roll     = (int16_t)  constrain(server.arg("rl").toInt(), -500, 500);
  if (server.hasArg("pt")) ctrl.pitch    = (int16_t)  constrain(server.arg("pt").toInt(), -500, 500);
  if (server.hasArg("yw")) ctrl.yaw      = (int16_t)  constrain(server.arg("yw").toInt(), -500, 500);
  if (server.hasArg("ar")) ctrl.arming   = (uint8_t)  constrain(server.arg("ar").toInt(), 0, 1);

  bool ok = radio.write(&ctrl, sizeof(ctrl));
  server.send(200, "text/plain", ok ? "sent" : "rf fail");
}

void setup() {
  Serial.begin(115200);

  // Wi-Fi AP
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.begin();

  // RF24 setup
  if (!radio.begin()) Serial.println("RF24 begin failed");
  radio.setAutoAck(true);
  radio.setRetries(2, 15);
  radio.setPALevel(RF24_PA_HIGH);      // PA/LNA, ensure good power supply
  radio.setDataRate(RF24_1MBPS);       // balance range/latency
  radio.setChannel(76);                // pick a quiet channel
  radio.openWritingPipe(pipeAddr);
  radio.stopListening();
}

void loop() {
  server.handleClient();
}
