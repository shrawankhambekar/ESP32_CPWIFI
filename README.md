# ESP32_CPWIFI — Complete Library Manual

**Automatic Captive Portal WiFi Login Library for ESP32**
**Author:** Shrawan Khambekar
**Version:** 1.2.0
**License:** MIT
**GitHub:** https://github.com/shrawankhambekar/ESP32_CPWIFI

---

## Table of Contents

1. [What Is This Library](#1-what-is-this-library)
2. [How It Works — Full Internal Flow](#2-how-it-works--full-internal-flow)
3. [What Is a Captive Portal](#3-what-is-a-captive-portal)
4. [Hardware Compatibility](#4-hardware-compatibility)
5. [Requirements](#5-requirements)
6. [Installation](#6-installation)
7. [Minimum Working Example](#7-minimum-working-example)
8. [Complete API Reference](#8-complete-api-reference)
9. [MAC Address Modes — Full Guide](#9-mac-address-modes--full-guide)
10. [Serial Log Reference](#10-serial-log-reference)
11. [Portal Compatibility](#11-portal-compatibility)
12. [MikroTik CHAP Auth — Technical Detail](#12-mikrotik-chap-auth--technical-detail)
13. [Session Management](#13-session-management)
14. [Examples](#14-examples)
15. [Chaining Options](#15-chaining-options)
16. [Troubleshooting](#16-troubleshooting)
17. [FAQ](#17-faq)
18. [Changelog](#18-changelog)

---

## 1. What Is This Library

ESP32_CPWIFI is an Arduino library that allows an ESP32 to automatically
authenticate through a captive portal WiFi network — the kind of network
where a login page appears before internet access is granted.

These networks are common in:
- Hostels and dormitories
- College and university campuses
- Hotels and restaurants
- ISP public hotspots
- Corporate guest WiFi

Without this library, an ESP32 connected to such a network would have local
WiFi connectivity but no internet access because it cannot interact with the
browser-based login page. This library solves that by simulating the login
process programmatically.

---

## 2. How It Works — Full Internal Flow

    BOOT
     |
     |-- [1] MAC SPOOFING
     |       Sets MAC address based on user-selected mode
     |       (Random / Fixed / Real / Custom)
     |       Ensures router sees it as a new or known device
     |
     |-- [2] WIFI CONNECTION
     |       Connects to the provided SSID
     |       Reads assigned local IP and gateway IP from DHCP
     |
     |-- [3] PORTAL URL DETECTION
     |       GET http://gatewayIP/login
     |       Searches HTML for hidden form named "sendin" reads action URL
     |       Falls back to first action= found on page
     |       Falls back to http://gatewayIP/login if nothing found
     |       Derives logout URL as same domain + /logout
     |
     |-- [4] LOGOUT PREVIOUS SESSION
     |       GET http://portalDomain/logout
     |       Clears existing session so fresh login page is served
     |
     |-- [5] AUTH TYPE DETECTION
     |       GET http://gatewayIP/login again
     |       Searches for hexMD5() JavaScript function in page
     |       |-- Found     --> MikroTik CHAP MD5 authentication
     |       |-- Not found --> Plain POST authentication
     |
     |-- [6A] MIKROTIK CHAP MD5 AUTH
     |       Parses octal-encoded chapID (prefix bytes) from JS
     |       Parses octal-encoded chapChallenge (suffix bytes) from JS
     |       Computes: MD5(chapID + plainPassword + chapChallenge)
     |       POSTs: username + hashed_password + dst + popup
     |
     |-- [6B] PLAIN POST AUTH (fallback)
     |       POSTs: username + password (URL-encoded) + dst + popup
     |
     |-- [7] INTERNET VERIFICATION
     |       HTTP GET to Google IP 142.250.182.46
     |       Any response > 0 confirms internet access
     |       Fetches public IP from api.ipify.org
     |
     |-- [8] LED + STATUS
     |       Sets LED HIGH if online
     |       Returns true/false to user code
     |
     |-- LOOP --> maintain() every 5 minutes
                  Tests internet
                  Re-runs full login flow if session expired

---

## 3. What Is a Captive Portal

A captive portal is a network authentication mechanism where:

1. Device connects to WiFi and gets a local IP normally
2. All HTTP traffic is intercepted by the router
3. Router redirects browser to a login page
4. User submits credentials via the login form
5. Router grants internet access to that device's MAC and IP pair

The login form on most ISP hotspots submits a POST request with credentials.
This library replicates that POST request directly from the ESP32, bypassing
the need for a browser.

MikroTik hotspots (most common in India) add a security layer called CHAP.
The password is never sent in plain text. Instead the server sends a random
challenge and the client must compute MD5(chapID + password + challenge) and
send that hash. This library handles the full CHAP flow automatically.

---

## 4. Hardware Compatibility

| Chip      | Example Boards                          | WiFi | MAC Spoof | Status         |
|-----------|-----------------------------------------|------|-----------|----------------|
| ESP32     | DevKit v1, WROOM-32, WROVER, NodeMCU-32 | YES  | YES       | Full support   |
| ESP32-S2  | S2 Mini, FeatherS2, Lolin S2 Mini       | YES  | YES       | Full support   |
| ESP32-S3  | DevKitC, S3 Box, XIAO S3, Lolin S3      | YES  | YES       | Full support   |
| ESP32-C3  | SuperMini, XIAO C3, DevKitM-1           | YES  | YES       | Full support   |
| ESP32-C6  | DevKitC-1, Seeed XIAO C6                | YES  | YES       | Full support   |
| ESP32-H2  | DevKitC-1                               | NO   | NO        | No WiFi on chip|

NOTE: ESP32-H2 has no WiFi radio. It supports only Bluetooth, Zigbee, and
Thread. This library cannot function on H2 regardless of code changes.

NOTE: MAC Spoofing on C3/C6 uses esp_iface_mac_addr_set() instead of
esp_wifi_set_mac(). This is handled automatically by the library using
compile-time chip detection. No user action needed.

---

## 5. Requirements

| Requirement         | Version                  |
|---------------------|--------------------------|
| Arduino IDE         | 2.0.0 or higher          |
| Arduino ESP32 Core  | 3.0.0 or higher REQUIRED |
| ESP32 board support | Installed via Board Manager |

IMPORTANT: ESP32 core 2.x does not have esp_iface_mac_addr_set().
MAC spoofing on C3/C6 will fail to compile on core 2.x.
Upgrade via: Tools > Board > Board Manager > esp32 > Update

No external libraries required. All dependencies are built into ESP32 core.

---

## 6. Installation

### Method A — Arduino Library Manager (easiest)
1. Open Arduino IDE
2. Go to Sketch > Include Library > Manage Libraries
3. Search: ESP32_CPWIFI
4. Click Install

### Method B — ZIP Install
1. Go to https://github.com/shrawankhambekar/ESP32_CPWIFI
2. Click Code > Download ZIP
3. In Arduino IDE: Sketch > Include Library > Add .ZIP Library
4. Select the downloaded ZIP file

### Method C — Manual Folder Install
1. Download and extract the repository
2. Copy the ESP32_CPWIFI folder to your Arduino libraries directory:
   Windows : C:\Users\YourName\Documents\Arduino\libraries\
   Mac     : /Users/YourName/Arduino/libraries/
   Linux   : /home/YourName/Arduino/libraries/
3. Restart Arduino IDE

### Verify Installation
After installing go to File > Examples and look for ESP32_CPWIFI in the list.

---

## 7. Minimum Working Example

    #include <ESP32_CPWIFI.h>

    CPWIFI wifi;

    void setup() {
      Serial.begin(115200);
      wifi.begin("YourWiFiName", "YourUsername", "YourPassword");
    }

    void loop() {
      wifi.maintain();
    }

This is all that is needed. The library auto-detects the portal,
authenticates, and keeps the session alive.

---

## 8. Complete API Reference

---

### 8.1 begin(ssid, username, password)

DESCRIPTION:
The main function. Connects to WiFi, detects the captive portal,
authenticates, and verifies internet access. Must be called once in setup().

PARAMETERS:
  ssid       const char*   WiFi network name
  username   const char*   Portal login username
  password   const char*   Portal login password

RETURNS: bool
  true  = internet is accessible, login succeeded
  false = login failed (wrong credentials, portal unreachable, timeout)

EXAMPLE:
    wifi.begin("HostelWiFi", "student01", "pass1234");

    if (wifi.begin("HostelWiFi", "student01", "pass1234")) {
      Serial.println("Online!");
    } else {
      Serial.println("Failed — check credentials");
    }

NOTES:
- All optional settings must be called before begin()
- Blocks until WiFi connects or times out (max 20 seconds)
- Allows 5 seconds for portal auth to take effect after POST
- Internet verified via HTTP request to Google IP 142.250.182.46

---

### 8.2 maintain()

DESCRIPTION:
Silently checks internet every 5 minutes and re-authenticates if session
expired. Must be called continuously inside loop().

PARAMETERS: None
RETURNS: void

EXAMPLE:
    void loop() {
      wifi.maintain();
      server.handleClient();
      readSensor();
    }

NOTES:
- Zero overhead when called frequently — uses internal timer
- Only triggers real check every 300000 ms (5 minutes)
- Re-runs full login flow including logout + re-auth on failure
- Does not block loop() during normal operation

---

### 8.3 setLED(pin)

DESCRIPTION:
Attaches a GPIO pin as status LED. LED turns ON when internet is active
and OFF when offline or session expired.

PARAMETERS:
  pin   int   GPIO pin number connected to LED

RETURNS: CPWIFI& (chainable)

EXAMPLE:
    wifi.setLED(15);   // external LED on GPIO 15
    wifi.setLED(2);    // onboard LED on most ESP32 boards

CIRCUIT:
    GPIO 15 ---- 220ohm ---- LED (+) ---- GND

NOTES:
- pinMode and initial LOW are handled automatically
- LED state updated automatically by maintain() on session changes
- Use GPIO 2 (onboard LED) for testing without external hardware

---

### 8.4 setMAC(mode, customMAC)

DESCRIPTION:
Controls how ESP32 MAC address is set before connecting. MAC spoofing
makes the router treat ESP32 as a new or different device.

PARAMETERS:
  mode        CPWIFI::MACMode   One of 4 modes below
  customMAC   const char*       Only required for MAC_CUSTOM mode

MAC MODES:
  CPWIFI::MAC_RANDOM   New random MAC every boot (DEFAULT)
  CPWIFI::MAC_FIXED    Always use 02:AB:CD:11:22:33
  CPWIFI::MAC_REAL     Use real hardware MAC, no spoofing
  CPWIFI::MAC_CUSTOM   Use your own specified MAC address

RETURNS: CPWIFI& (chainable)

EXAMPLE:
    wifi.setMAC(CPWIFI::MAC_RANDOM);
    wifi.setMAC(CPWIFI::MAC_FIXED);
    wifi.setMAC(CPWIFI::MAC_REAL);
    wifi.setMAC(CPWIFI::MAC_CUSTOM, "DE:AD:BE:EF:00:01");

MAC ADDRESS RULES:
- First byte must be 0x02 for locally administered unicast
- First byte must be even — odd = multicast, will not work
- Valid   : 02:xx:xx:xx:xx:xx  or  06:xx:xx:xx:xx:xx
- Invalid : 01:xx:xx:xx:xx:xx  or  03:xx:xx:xx:xx:xx

CHIP IMPLEMENTATION (automatic, no user action needed):
  ESP32, S2, S3 (Xtensa)  --> esp_wifi_set_mac(WIFI_IF_STA, mac)
  ESP32-C3, C6 (RISC-V)   --> esp_iface_mac_addr_set(mac, ESP_MAC_WIFI_STA)

---

### 8.5 setGateway(ip)

DESCRIPTION:
Manually overrides gateway IP used to reach the portal. Normally not
needed — library reads gateway from DHCP automatically.

PARAMETERS:
  ip   const char*   Gateway IP as string

RETURNS: CPWIFI& (chainable)

EXAMPLE:
    wifi.setGateway("192.168.1.1");
    wifi.begin("WiFi", "user", "pass");

WHEN TO USE:
- Serial log shows wrong gateway IP
- Network uses non-standard gateway addressing
- DHCP not providing correct gateway

---

### 8.6 setPortalURL(url)

DESCRIPTION:
Manually overrides the portal POST URL. Normally not needed — library
parses it from HTML login page automatically.

PARAMETERS:
  url   const char*   Full portal POST URL

RETURNS: CPWIFI& (chainable)

EXAMPLE:
    wifi.setPortalURL("http://wifi.com/login");
    wifi.begin("WiFi", "user", "pass");

HOW TO FIND YOUR PORTAL URL MANUALLY:
1. Connect laptop to the WiFi
2. Open DevTools (F12) > Network tab > check Preserve Log
3. Submit login form
4. Find the POST request — its URL is your portal URL

---

### 8.7 silent()

DESCRIPTION:
Disables all [CPWIFI] serial output. Use in production deployments.

PARAMETERS: None
RETURNS: CPWIFI& (chainable)

EXAMPLE:
    wifi.silent();
    wifi.setLED(15);
    wifi.begin("WiFi", "user", "pass");

NOTES:
- LED still works normally in silent mode
- isOnline(), localIP(), publicIP() still work normally
- Not recommended during development

---

### 8.8 isOnline()

DESCRIPTION:
Returns current internet connectivity status.

RETURNS: bool — true if internet accessible, false if not

EXAMPLE:
    if (wifi.isOnline()) {
      sendSensorData();
    }

---

### 8.9 localIP()

DESCRIPTION:
Returns ESP32 local IP on the WiFi network.

RETURNS: String — e.g. "192.168.88.245"

EXAMPLE:
    Serial.println("Local IP: " + wifi.localIP());
    Serial.println("Open: http://" + wifi.localIP());

---

### 8.10 publicIP()

DESCRIPTION:
Returns public internet IP of the network. Fetched from api.ipify.org
after successful authentication.

RETURNS: String — e.g. "103.168.165.85" or empty string if not fetched

EXAMPLE:
    Serial.println("Public IP: " + wifi.publicIP());

---

### 8.11 gatewayIP()

DESCRIPTION:
Returns the gateway IP used for portal detection.

RETURNS: String — e.g. "192.168.88.1"

EXAMPLE:
    Serial.println("Gateway: " + wifi.gatewayIP());

---

### 8.12 portalURL()

DESCRIPTION:
Returns the portal POST URL that was detected or manually set.

RETURNS: String — e.g. "http://wifi.com/login"

EXAMPLE:
    Serial.println("Portal: " + wifi.portalURL());

---

## 9. MAC Address Modes — Full Guide

MAC_RANDOM (default)
Router sees a brand new device every reboot. Always gets a fresh login
page. Best for development and testing.
    wifi.setMAC(CPWIFI::MAC_RANDOM);

MAC_FIXED
Always presents 02:AB:CD:11:22:33. Router remembers it across reboots.
Good for stable deployments with consistent identity.
    wifi.setMAC(CPWIFI::MAC_FIXED);

MAC_REAL
No MAC spoofing. Uses factory-programmed hardware MAC. Use when router
has a MAC whitelist and your device is pre-authorized.
    wifi.setMAC(CPWIFI::MAC_REAL);

MAC_CUSTOM
Specify any MAC address. Use to mimic a trusted device or maintain a
known identity.
    wifi.setMAC(CPWIFI::MAC_CUSTOM, "AA:BB:CC:DD:EE:FF");

---

## 10. Serial Log Reference

NORMAL OPERATION:
    [CPWIFI] MAC         | Random  02:AB:CD:F1:3A:7C
    [CPWIFI] Connecting  | C10 WIFI
    [CPWIFI] Connected   | Local IP : 192.168.88.245
    [CPWIFI] Gateway     | 192.168.88.1
    [CPWIFI] Portal URL  | http://wifi.com/login
    [CPWIFI] Auth        | MikroTik CHAP MD5
    [CPWIFI] Status      | Online!
    [CPWIFI] Public IP   | 103.168.165.85

ERROR LOGS:
    [CPWIFI] FAIL        | WiFi timeout
    [CPWIFI] FAIL        | Portal unreachable (code -1)
    [CPWIFI] FAIL        | CHAP suffix not found
    [CPWIFI] FAIL        | CHAP byte parsing failed
    [CPWIFI] Status      | FAILED — check credentials
    [CPWIFI] Session expired — re-authenticating...

MAC MODE LOGS:
    [CPWIFI] MAC         | Random  02:AB:CD:11:22:33
    [CPWIFI] MAC         | Fixed   02:AB:CD:11:22:33
    [CPWIFI] MAC         | Custom  DE:AD:BE:EF:00:01
    [CPWIFI] MAC         | Real hardware MAC (no spoof)

---

## 11. Portal Compatibility

| Portal Type                  | Auth Method       | Supported        |
|------------------------------|-------------------|------------------|
| MikroTik Hotspot             | CHAP MD5          | YES Full         |
| Desire Tech ISP              | CHAP MD5          | YES Full         |
| Plain HTML form              | POST              | YES Full         |
| College / hostel networks    | CHAP MD5 or plain | YES Full         |
| Hotel WiFi portals           | Plain POST        | YES Full         |
| OAuth2 / Google login        | Browser redirect  | NO               |
| HTTPS-only with cert pinning | TLS + browser     | NO               |
| SMS OTP portals              | Browser + SMS     | NO               |

---

## 12. MikroTik CHAP Auth — Technical Detail

MikroTik portals never send plain passwords. The login page contains JS:

    hexMD5('\215' + document.login.password.value + '\373\322\320\121...')

The octal-encoded bytes are:
  Prefix (1 byte)   : chapID — a session identifier
  Suffix (16 bytes) : chapChallenge — random value, changes every page load

Hash computation:
    hashedPassword = MD5(chapID + plainPassword + chapChallenge)

POST body sent:
    username=student01&password=a3f8d2...md5hash...&dst=&popup=true

This library:
1. Parses octal byte sequences from JS using _parseOctal()
2. Computes MD5 using mbedtls_md5 (built into ESP32 core)
3. POSTs hex-encoded hash as the password field

The plain password never leaves the device.

---

## 13. Session Management

Sessions expire due to:
- Fixed time limits (e.g. 8 hours, 24 hours)
- Inactivity timeout (e.g. 30 minutes idle)
- Router restart

The library handles this via maintain():

    Every 5 minutes:
      HTTP GET to 142.250.182.46 (Google IP, no DNS needed)
      Response > 0  : session active, do nothing
      Response <= 0 : session expired
          Run full login flow again
          Update LED state

maintain() MUST be called in loop() for this to work.

---

## 14. Examples

### BasicConnect
Path: File > Examples > ESP32_CPWIFI > BasicConnect

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

### WithWebServer
Path: File > Examples > ESP32_CPWIFI > WithWebServer

Connects and serves a live status webpage. After flashing open the IP
shown in Serial Monitor in any browser on the same WiFi network.

Page shows: Local IP, Public IP, Gateway, Portal URL, Uptime.
Auto-refreshes every 10 seconds.

---

## 15. Chaining Options

All optional settings return CPWIFI& and can be chained:

    wifi.setLED(15)
        .setMAC(CPWIFI::MAC_RANDOM)
        .setGateway("192.168.88.1")
        .silent()
        .begin("MyWiFi", "user", "pass");

---

## 16. Troubleshooting

| Symptom                          | Likely Cause              | Fix                                      |
|----------------------------------|---------------------------|------------------------------------------|
| WiFi timeout                     | Wrong SSID or weak signal | Check SSID spelling, move closer         |
| Portal unreachable (code -1)     | Wrong gateway IP          | Use setGateway() manually                |
| Portal unreachable (code 302)    | Portal is redirecting     | Use setPortalURL() with redirect target  |
| CHAP suffix not found            | Portal JS format differs  | Open GitHub issue with portal HTML       |
| FAILED check credentials         | Wrong username/password   | Verify by logging in via browser         |
| Portal URL fallback in logs      | sendin form not found     | Not an error — fallback still works      |
| LED never turns on               | Wrong GPIO or polarity    | Check pin number and LED orientation     |
| Fails to compile on C3           | Old ESP32 core            | Update to core 3.0.0 via Board Manager   |
| Session drops frequently         | Short portal timeout      | Confirm maintain() is called in loop()   |

---

## 17. FAQ

Q: Does this work without knowing the portal URL?
A: Yes. The library fetches the login page and parses the form URL automatically.

Q: Does this work on WPA2 password-protected WiFi?
A: Yes for the portal auth part. The WiFi network password is a separate
   parameter — currently the library handles open WiFi plus portal auth.

Q: Will this work if I move to a different hostel or network?
A: Yes, as long as the new portal is MikroTik CHAP or plain POST type.
   The library auto-detects on every boot.

Q: Is the password ever sent in plain text over the network?
A: For MikroTik CHAP portals — No. Only the MD5 hash is transmitted.
   For plain POST portals — Yes, same as what your browser does.

Q: Can two ESP32s use the same credentials simultaneously?
A: Depends on portal policy. Most ISP portals allow only one session
   per username at a time.

Q: Why does it say Already authenticated sometimes?
A: The router remembered your previous session by MAC or IP. The library
   detects this, skips the POST, and turns LED on. This is correct.

Q: Can I use this for ESP8266?
A: No. This library uses ESP32-specific APIs. A separate port is needed.

---

## 18. Changelog

### v1.2.0
- Added setMAC() with 4 modes: RANDOM, FIXED, REAL, CUSTOM
- Added chip-specific MAC spoofing for C3/C6 using esp_iface_mac_addr_set
- Removed noMACSpoof() — replaced by setMAC(MAC_REAL)
- Aligned serial log output with pipe separator
- Improved portal URL detection — prioritizes hidden sendin form

### v1.1.0
- Added multi-chip support: ESP32, S2, S3, C3, C6
- Added esp_mac.h include for RISC-V chips
- Improved auto-detection fallback logic
- Cleaner serial output format

### v1.0.0
- Initial release
- MikroTik CHAP MD5 authentication
- Auto portal URL detection
- Session renewal via maintain()
- LED indicator support
- Plain POST fallback

---

## License

MIT License — Copyright (c) 2026 Shrawan Khambekar

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the Software),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software. The software is
provided as is, without warranty of any kind.

---

If this library helped your project, give it a star on GitHub.
Found a bug? Open an issue at github.com/shrawankhambekar/ESP32_CPWIFI
