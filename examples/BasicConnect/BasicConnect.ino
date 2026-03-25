/*
 * ESP32_CPWIFI — Basic Connection Example
 * Author : Shrawan Khambekar
 * GitHub : https://github.com/shrawankhambekar/ESP32_CPWIFI
 *
 * Connects to a captive portal WiFi with 3 lines.
 * Change SSID, USERNAME, PASSWORD and flash.
 */

#include <ESP32_CPWIFI.h>

CPWIFI wifi;

void setup() {
  Serial.begin(115200);
  wifi.setLED(15);
  wifi.begin("YourWiFiName", "YourUsername", "YourPassword");
}

void loop() {
  wifi.maintain();
}
