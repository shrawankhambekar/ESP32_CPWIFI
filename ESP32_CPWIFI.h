/*
 * ESP32_CPWIFI - Automatic Captive Portal WiFi Library
 * Author  : Shrawan Khambekar
 * Version : 1.2.0
 * License : MIT
 * GitHub  : https://github.com/shrawankhambekar/ESP32_CPWIFI
 *
 * USAGE (minimum):
 *   CPWIFI wifi;
 *   wifi.begin("MyWiFi", "username", "password");
 */

#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include "mbedtls/md5.h"
#include "esp_wifi.h"
#include "esp_mac.h"

class CPWIFI {
public:

  // ═══════════════════════════════════════════
  //   MAC MODE SELECTION
  // ═══════════════════════════════════════════
  enum MACMode { MAC_RANDOM, MAC_FIXED, MAC_REAL, MAC_CUSTOM };

  // ═══════════════════════════════════════════
  //   PUBLIC API — only these 3 are required
  // ═══════════════════════════════════════════

  bool begin(const char* ssid, const char* username, const char* password) {
    _ssid = ssid;
    _user = username;
    _pass = password;
    _connectWiFi();
    _autoDetect();
    return _login();
  }

  // Put in loop() — silently re-authenticates if session expires
  void maintain() {
    if (millis() - _lastCheck < 300000) return;
    _lastCheck = millis();
    if (!_testInternet()) {
      _log("Session expired — re-authenticating...");
      _login();
    }
  }

  // ═══════════════════════════════════════════
  //   OPTIONAL SETTINGS — call before begin()
  // ═══════════════════════════════════════════

  // Attach LED pin — turns ON when internet is active
  CPWIFI& setLED(int pin) {
    _led = pin;
    pinMode(_led, OUTPUT);
    digitalWrite(_led, LOW);
    return *this;
  }

  // Manually set gateway IP if auto-detect fails
  CPWIFI& setGateway(const char* ip)  { _manualGateway = ip; return *this; }

  // Manually set portal POST URL if auto-detect fails
  CPWIFI& setPortalURL(const char* u) { _manualPostURL = u; return *this; }

  // Disable all serial logs
  CPWIFI& silent() { _verbose = false; return *this; }

  // MAC address control:
  //   wifi.setMAC(CPWIFI::MAC_RANDOM)                    — new random MAC every boot (DEFAULT)
  //   wifi.setMAC(CPWIFI::MAC_FIXED)                     — always same spoofed MAC 02:AB:CD:11:22:33
  //   wifi.setMAC(CPWIFI::MAC_REAL)                      — use real hardware MAC, no spoofing
  //   wifi.setMAC(CPWIFI::MAC_CUSTOM, "AA:BB:CC:DD:EE:FF") — your own MAC
  CPWIFI& setMAC(MACMode mode, const char* customMAC = "") {
    _macMode = mode;
    if (mode == MAC_CUSTOM && strlen(customMAC) > 0) {
      sscanf(customMAC, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &_customMAC[0], &_customMAC[1], &_customMAC[2],
        &_customMAC[3], &_customMAC[4], &_customMAC[5]);
    }
    return *this;
  }

  // ═══════════════════════════════════════════
  //   STATUS HELPERS
  // ═══════════════════════════════════════════
  bool   isOnline()  { return _online; }
  String localIP()   { return WiFi.localIP().toString(); }
  String publicIP()  { return _publicIP; }
  String gatewayIP() { return _gateway; }
  String portalURL() { return _postURL; }

private:
  const char* _ssid          = "";
  const char* _user          = "";
  const char* _pass          = "";
  const char* _manualGateway = "";
  const char* _manualPostURL = "";
  int     _led      = -1;
  bool    _verbose  = true;
  bool    _online   = false;
  String  _gateway  = "";
  String  _postURL  = "";
  String  _logoutURL= "";
  String  _publicIP = "";
  MACMode _macMode  = MAC_RANDOM;
  uint8_t _customMAC[6] = {0x02, 0xAB, 0xCD, 0x11, 0x22, 0x33};
  unsigned long _lastCheck = 0;

  void _log(String msg) { if (_verbose) Serial.println("[CPWIFI] " + msg); }

  // ── Apply MAC based on user selected mode ──
  void _applyMAC() {
    if (_macMode == MAC_REAL) {
      _log("MAC        | Real hardware MAC (no spoof)");
      return;
    }

    uint8_t mac[6];

    if (_macMode == MAC_RANDOM) {
      mac[0]=0x02; mac[1]=0xAB; mac[2]=0xCD;
      mac[3]=random(0,256); mac[4]=random(0,256); mac[5]=random(0,256);
    }
    else if (_macMode == MAC_FIXED) {
      memcpy(mac, (uint8_t[]){0x02,0xAB,0xCD,0x11,0x22,0x33}, 6);
    }
    else if (_macMode == MAC_CUSTOM) {
      memcpy(mac, _customMAC, 6);
    }

    // Apply correct API per chip family
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
    esp_iface_mac_addr_set(mac, ESP_MAC_WIFI_STA);
#else
    esp_wifi_set_mac(WIFI_IF_STA, mac);
#endif

    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    _log("MAC        | " + String(
      _macMode==MAC_RANDOM ? "Random " :
      _macMode==MAC_FIXED  ? "Fixed  " : "Custom ") + String(buf));
  }

  // ── Step 1: Connect to WiFi ──
  void _connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(200);
    _applyMAC();
    WiFi.begin(_ssid);
    _log("Connecting | " + String(_ssid));
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis()-t > 20000) { _log("FAIL       | WiFi timeout"); return; }
      delay(500);
    }
    _log("Connected  | Local IP : " + WiFi.localIP().toString());
  }

  // ── Step 2: Auto-detect portal URLs ──
  void _autoDetect() {
    _gateway = (strlen(_manualGateway) > 0)
               ? String(_manualGateway)
               : WiFi.gatewayIP().toString();
    _log("Gateway    | " + _gateway);

    if (strlen(_manualPostURL) > 0) {
      _postURL   = String(_manualPostURL);
      _logoutURL = _domainOf(_postURL) + "/logout";
      _log("Portal URL | " + _postURL + " (manual)");
      return;
    }

    HTTPClient http;
    http.begin("http://" + _gateway + "/login");
    http.addHeader("User-Agent","Mozilla/5.0");
    http.setTimeout(8000);
    if (http.GET() == 200) {
      String html = http.getString();

      // Priority 1: hidden 'sendin' form — most reliable for MikroTik
      int idx = html.indexOf("name=\"sendin\"");
      if (idx != -1) {
        int searchFrom = (idx - 200 > 0) ? idx - 200 : 0;
        int a = html.indexOf("action=\"", searchFrom);
        if (a != -1) {
          a += 8;
          int e = html.indexOf("\"", a);
          _postURL = html.substring(a, e);
        }
      }

      // Priority 2: first action= on page
      if (_postURL == "") {
        int a = html.indexOf("action=\"");
        if (a != -1) {
          a += 8;
          int e = html.indexOf("\"", a);
          _postURL = html.substring(a, e);
        }
      }

      // Priority 3: fallback to gateway
      if (_postURL == "") {
        _postURL = "http://" + _gateway + "/login";
        _log("Portal URL | " + _postURL + " (fallback)");
      } else {
        _log("Portal URL | " + _postURL);
      }

      _logoutURL = _domainOf(_postURL) + "/logout";
    }
    http.end();
  }

  // ── Step 3: Full login flow ──
  bool _login() {
    _logout();

    HTTPClient http;
    http.begin("http://" + _gateway + "/login");
    http.addHeader("User-Agent","Mozilla/5.0");
    http.setTimeout(10000);
    int gc = http.GET();
    if (gc != 200) {
      http.end();
      _log("FAIL       | Portal unreachable (code " + String(gc) + ")");
      return false;
    }
    String html = http.getString();
    http.end();

    if (html.indexOf("logged in") != -1) {
      _log("Auth       | Already authenticated");
      _setOnline(true);
      return true;
    }

    int js = html.indexOf("hexMD5('");
    if (js != -1) {
      // MikroTik CHAP MD5
      uint8_t pre[32], suf[32];
      int pLen = _parseOctal(html, js+8, pre, 32);
      int sfx  = html.indexOf("value + '", js);
      if (sfx == -1) { _log("FAIL       | CHAP suffix not found"); return false; }
      int sLen = _parseOctal(html, sfx+9, suf, 32);
      if (pLen==0||sLen==0) { _log("FAIL       | CHAP byte parsing failed"); return false; }
      _log("Auth       | MikroTik CHAP MD5");
      _post("username="+_enc(_user)+"&password="+_chapMD5(pre,pLen,_pass,suf,sLen)+"&dst=&popup=true");
    } else {
      // Plain text fallback
      _log("Auth       | Plain POST");
      _post("username="+_enc(_user)+"&password="+_enc(_pass)+"&dst=&popup=true");
    }

    delay(5000);
    bool ok = _testInternet();
    _setOnline(ok);

    if (ok) {
      _fetchPublicIP();
      _log("Status     | Online!");
      _log("Public IP  | " + _publicIP);
    } else {
      _log("Status     | FAILED — check credentials");
    }
    return ok;
  }

  void _post(String body) {
    HTTPClient http;
    http.begin(_postURL);
    http.addHeader("Content-Type","application/x-www-form-urlencoded");
    http.addHeader("User-Agent","Mozilla/5.0");
    http.addHeader("Referer","http://"+_gateway+"/login");
    http.setTimeout(10000);
    http.POST(body);
    http.end();
  }

  void _logout() {
    if (_logoutURL == "") return;
    HTTPClient http;
    http.begin(_logoutURL);
    http.setTimeout(5000);
    http.GET();
    http.end();
    delay(800);
  }

  bool _testInternet() {
    HTTPClient h;
    h.begin("http://142.250.182.46");
    h.setTimeout(6000);
    bool ok = (h.GET() > 0);
    h.end();
    return ok;
  }

  void _fetchPublicIP() {
    HTTPClient h;
    h.begin("http://api.ipify.org");
    h.setTimeout(6000);
    if (h.GET() == 200) _publicIP = h.getString();
    h.end();
  }

  void _setOnline(bool s) {
    _online = s;
    if (_led >= 0) digitalWrite(_led, s ? HIGH : LOW);
  }

  String _domainOf(String url) {
    int sl = url.indexOf("/", 7);
    return (sl != -1) ? url.substring(0, sl) : url;
  }

  String _enc(const char* s) {
    String o=""; int i=0;
    while(s[i]){
      char c=s[i++];
      if(isAlphaNumeric(c)||c=='-'||c=='_'||c=='.'||c=='~') o+=c;
      else{char b[4];sprintf(b,"%%%02X",(uint8_t)c);o+=b;}
    }
    return o;
  }

  int _parseOctal(const String& html, int start, uint8_t* buf, int maxLen) {
    int n=0,i=start;
    while(i<(int)html.length()&&n<maxLen){
      char c=html[i];
      if(c=='\'') break;
      if(c=='\\'){
        i++;
        char nx=html[i];
        if(nx>='0'&&nx<='7'){
          int v=nx-'0'; i++;
          if(i<(int)html.length()&&html[i]>='0'&&html[i]<='7'){v=v*8+(html[i]-'0');i++;}
          if(i<(int)html.length()&&html[i]>='0'&&html[i]<='7'){v=v*8+(html[i]-'0');i++;}
          buf[n++]=(uint8_t)v;
        } else { buf[n++]=(uint8_t)nx; i++; }
      } else { buf[n++]=(uint8_t)c; i++; }
    }
    return n;
  }

  String _chapMD5(uint8_t* pre,int pLen,const char* pass,uint8_t* suf,int sLen){
    uint8_t digest[16];
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx); mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx,pre,pLen);
    mbedtls_md5_update(&ctx,(const uint8_t*)pass,strlen(pass));
    mbedtls_md5_update(&ctx,suf,sLen);
    mbedtls_md5_finish(&ctx,digest); mbedtls_md5_free(&ctx);
    String r="";
    for(int i=0;i<16;i++){char b[3];sprintf(b,"%02x",digest[i]);r+=b;}
    return r;
  }
};
