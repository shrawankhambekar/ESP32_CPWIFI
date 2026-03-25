/*
 * ESP32_CPWIFI — Web Server Example
 * Author : Shrawan Khambekar
 * GitHub : https://github.com/shrawankhambekar/ESP32_CPWIFI
 *
 * After flashing, open Serial Monitor to get the IP.
 * Visit that IP in your browser (on same WiFi).
 */

#include <ESP32_CPWIFI.h>
#include <WebServer.h>

CPWIFI wifi;
WebServer server(80);

void handleRoot() {
  String uptime = String(millis() / 1000);
  String page = R"html(
<!DOCTYPE html><html>
<head>
  <title>ESP32_CPWIFI</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta http-equiv="refresh" content="10">
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;display:flex;justify-content:center;align-items:center;min-height:100vh}
    .card{background:#1e293b;border-radius:16px;padding:32px;width:90%;max-width:420px;box-shadow:0 8px 32px rgba(0,0,0,0.4)}
    h1{color:#38bdf8;font-size:22px;margin-bottom:24px;text-align:center}
    .badge{background:#022c22;color:#4ade80;font-size:13px;padding:4px 12px;border-radius:99px;display:inline-block;margin-bottom:24px}
    .row{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid #334155}
    .row:last-child{border-bottom:none}
    .label{color:#94a3b8;font-size:13px}
    .val{color:#38bdf8;font-weight:bold;font-size:14px}
    .footer{text-align:center;margin-top:20px;color:#475569;font-size:11px}
  </style>
</head>
<body><div class="card">
  <h1>&#128225; ESP32_CPWIFI</h1>
  <div style="text-align:center"><span class="badge">&#x2714; Online</span></div>
  <div class="row"><span class="label">Local IP</span><span class="val">)html" + wifi.localIP() + R"html(</span></div>
  <div class="row"><span class="label">Public IP</span><span class="val">)html" + wifi.publicIP() + R"html(</span></div>
  <div class="row"><span class="label">Gateway</span><span class="val">)html" + wifi.gatewayIP() + R"html(</span></div>
  <div class="row"><span class="label">Portal URL</span><span class="val">)html" + wifi.portalURL() + R"html(</span></div>
  <div class="row"><span class="label">Uptime</span><span class="val">)html" + uptime + R"html( sec</span></div>
  <div class="footer">ESP32_CPWIFI by Shrawan Khambekar &bull; Auto-refreshes every 10s</div>
</div></body></html>
)html";
  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  wifi.setLED(15);

  if (wifi.begin("YourWiFiName", "YourUsername", "YourPassword")) {
    server.on("/", handleRoot);
    server.begin();
    Serial.println("Open in browser: http://" + wifi.localIP());
  }
}

void loop() {
  wifi.maintain();
  server.handleClient();
}
