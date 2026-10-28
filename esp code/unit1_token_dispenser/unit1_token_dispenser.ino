// ============================================================
//  GTM4Health TMS — Unit-1 : Token Dispenser (OLED Only)
//
//  Hardware:
//    ESP32
//    1.3" OLED (SH1106 / SSD1306 128×64) — I2C
//        VCC → 3.3V      GND → GND
//        SCK → GPIO 22   SDA → GPIO 21
//    Token Button → GPIO 4  (other leg → GND)
//
//  Button behaviour:
//    Short press  → Generate token, show number on OLED
//    Hold 10 s    → Clear saved WiFi → enter WiFi config portal
//
//  Libraries needed (Arduino Library Manager):
//    • ArduinoJson  (Benoit Blanchon)
//    • U8g2         (oliver)
//
//  NOTE: If your display is SSD1306 (not SH1106) swap the
//        constructor on line marked *** SSD1306 ALTERNATIVE ***
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include <Wire.h>

// ── OLED constructor ──────────────────────────────────────────
// SH1106 (most common with 1.3" modules):
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// *** SSD1306 ALTERNATIVE — uncomment if your module is SSD1306:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ── Credentials (stored in NVS flash) ────────────────────────
String ssid      = "";
String password  = "";
String serverURL = "";

// ── Web / DNS (captive portal for WiFi setup) ────────────────
WebServer  webServer(80);
DNSServer  dnsServer;
const byte DNS_PORT = 53;
Preferences preferences;
bool apMode = false;

// ── Button ───────────────────────────────────────────────────
const int     buttonPin        = 4;
bool          lastButtonState  = HIGH;
unsigned long buttonPressStart = 0;
volatile bool buttonISRFlag    = false;
volatile unsigned long isrTime = 0;

void IRAM_ATTR onButtonFall() {
    unsigned long now = millis();
    if (now - isrTime > 50) {
        buttonISRFlag = true;
        isrTime = now;
    }
}

// ── Token tracking & Server Sync ─────────────────────────────
int           lastToken    = 0;     // 0 when reset or no tokens issued
bool          tokenIssued  = false;
unsigned long lastSyncTime = 0;
const unsigned long SYNC_INTERVAL_MS = 1500;  // check server every 1.5s for reset

// ============================================================
//  OLED HELPERS
// ============================================================

// Small text — 4 lines
void showOLED(const String& l1, const String& l2 = "",
              const String& l3 = "", const String& l4 = "") {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    if (l1.length()) u8g2.drawStr(0, 12, l1.c_str());
    if (l2.length()) u8g2.drawStr(0, 26, l2.c_str());
    if (l3.length()) u8g2.drawStr(0, 42, l3.c_str());
    if (l4.length()) u8g2.drawStr(0, 56, l4.c_str());
    u8g2.sendBuffer();
}

// Large token number — fills the display
void showTokenOLED(int token) {
    u8g2.clearBuffer();

    // Top label
    u8g2.setFont(u8g2_font_7x13B_tf);
    u8g2.drawStr(20, 14, "YOUR TOKEN");

    // Horizontal divider
    u8g2.drawHLine(0, 17, 128);

    // Big number centred
    u8g2.setFont(u8g2_font_logisoso32_tf);
    String t = String(token);
    int w = u8g2.getStrWidth(t.c_str());
    u8g2.drawStr((128 - w) / 2, 60, t.c_str());

    u8g2.sendBuffer();
}

// Shown immediately when queue is reset from Unit-2
void showResetOLED() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_7x13B_tf);
    u8g2.drawStr(16, 14, "QUEUE RESET");

    u8g2.drawHLine(0, 17, 128);

    u8g2.setFont(u8g2_font_logisoso32_tf);
    String t = "0";
    int w = u8g2.getStrWidth(t.c_str());
    u8g2.drawStr((128 - w) / 2, 55, t.c_str());

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 64, "Press btn for next");

    u8g2.sendBuffer();
}

// Idle screen — shows last token (or 0 when reset/ready)
void showIdleOLED() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 11, "Last Token Issued:");

    u8g2.drawHLine(0, 14, 128);

    u8g2.setFont(u8g2_font_logisoso32_tf);
    String t = tokenIssued ? String(lastToken) : "0";
    int w = u8g2.getStrWidth(t.c_str());
    u8g2.drawStr((128 - w) / 2, 55, t.c_str());

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 64, "Press btn for next");

    u8g2.sendBuffer();
}

// ============================================================
//  CAPTIVE PORTAL HTML
// ============================================================

const char setupPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GTM4Health TMS Setup</title>
<style>
body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#eef6fb;
     display:flex;justify-content:center;align-items:center;
     min-height:100vh;box-sizing:border-box;}
.container{background:white;padding:30px;border-radius:12px;
           width:100%;max-width:400px;
           box-shadow:0 4px 20px rgba(0,0,0,.15);
           border-top:6px solid #1683c4;box-sizing:border-box;}
h2{color:#0b4f7c;margin:0 0 6px 0;text-align:center;font-size:20px;}
.sub{color:#607080;font-size:13px;text-align:center;
     margin-bottom:20px;letter-spacing:1px;}
label{color:#607080;font-size:13px;font-weight:600;
      letter-spacing:1px;display:block;margin-top:14px;}
input[type=text],input[type=password]{
  width:100%;padding:12px;margin-top:5px;
  border:1px solid #c6d0d8;border-radius:8px;
  font-size:15px;box-sizing:border-box;}
.pw-wrap{position:relative;display:flex;align-items:center;margin-top:5px;}
.pw-wrap input{margin-top:0;padding-right:44px;}
.eye-btn{position:absolute;right:6px;top:50%;transform:translateY(-50%);
         background:none;border:none;padding:8px;cursor:pointer;
         display:flex;align-items:center;justify-content:center;
         width:auto;margin-top:0;}
.submit-btn{width:100%;padding:14px;margin-top:20px;
            background:#0b4f7c;color:white;border:none;
            font-size:17px;border-radius:8px;cursor:pointer;font-weight:600;}
.hint{color:#888;font-size:12px;text-align:center;margin-top:10px;}
</style>
</head>
<body>
<div class="container">
  <h2>&#128268; GTM4Health TMS</h2>
  <div class="sub">UNIT-1 &mdash; TOKEN DISPENSER SETUP</div>
  <form action="/save" method="GET">
    <label>WIFI NAME (SSID)</label>
    <input type="text" name="ssid" placeholder="Your WiFi name" required>
    <label>PASSWORD</label>
    <div class="pw-wrap">
      <input type="password" id="pw" name="password" placeholder="WiFi password">
      <button type="button" class="eye-btn" onclick="togglePw()" aria-label="Show password">
        <svg id="eye-icon" viewBox="0 0 24 24" width="22" height="22" fill="none" stroke="#607080" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
          <circle cx="12" cy="12" r="3"></circle>
        </svg>
      </button>
    </div>
    <label>SERVER URL</label>
    <input type="text" name="server"
           value="https://tms-v1-2.onrender.com" required>
    <button type="submit" class="submit-btn">Connect &amp; Save</button>
  </form>
  <p class="hint">Leave password blank for open networks.</p>
</div>
<script>
function togglePw(){
  var p=document.getElementById('pw');
  var ic=document.getElementById('eye-icon');
  if(p.type==='password'){
    p.type='text';
    ic.innerHTML='<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
  }else{
    p.type='password';
    ic.innerHTML='<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
  }
}
</script>
</body>
</html>
)rawliteral";

const char savedPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="refresh" content="4;url=https://gtm-tms2.vercel.app">
<title>Settings Saved!</title>
<style>
body{font-family:Arial,sans-serif;margin:0;background:#eef6fb;
     display:flex;justify-content:center;align-items:center;min-height:100vh;}
.box{background:white;padding:40px 30px;border-radius:12px;
     text-align:center;border-top:6px solid #27ae60;
     max-width:360px;width:90%;box-shadow:0 4px 20px rgba(0,0,0,.12);}
h2{color:#27ae60;margin:0 0 12px 0;}
p{color:#555;margin:8px 0;line-height:1.5;}
a{color:#1683c4;text-decoration:none;font-weight:600;}
.count{font-size:40px;font-weight:700;color:#0b4f7c;margin:16px 0;}
</style>
</head>
<body>
<div class="box">
  <h2>&#10003; Settings Saved!</h2>
  <p>Device is connecting to your WiFi...</p>
  <div class="count" id="c">4</div>
  <p>Redirecting to<br><a href="https://gtm-tms2.vercel.app">gtm-tms2.vercel.app</a></p>
</div>
<script>
var n=4;
var t=setInterval(function(){
  n--;
  var el=document.getElementById('c');
  if(el) el.innerText=n;
  if(n<=0){
    clearInterval(t);
    window.location.href='https://gtm-tms2.vercel.app';
  }
},1000);
</script>
</body>
</html>
)rawliteral";

// ============================================================
//  FLASH STORAGE
// ============================================================

void saveWiFi() {
    preferences.begin("wifi", false);
    preferences.putString("ssid",     ssid);
    preferences.putString("password", password);
    preferences.putString("server",   serverURL);
    preferences.end();
}

bool loadWiFi() {
    preferences.begin("wifi", true);
    ssid      = preferences.getString("ssid",     "");
    password  = preferences.getString("password", "");
    serverURL = preferences.getString("server",   "");
    preferences.end();
    return !(ssid == "" || serverURL == "");
}

void clearWiFi() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    Serial.println("WiFi credentials cleared.");
}

// ============================================================
//  SERVER SYNC (Detects Queue Reset from Unit-2 immediately)
// ============================================================

void checkServerSync(bool initialSync = false) {
    if (WiFi.status() != WL_CONNECTED || serverURL.length() == 0) return;

    HTTPClient http;
    http.setTimeout(1200);
    String url = serverURL + "/latest";
    http.begin(url);
    int code = http.GET();

    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            String tStr = doc["token"] | String("000");
            int serverToken = tStr.toInt();

            if (serverToken == 0) {
                // Server queue is empty / reset
                if (tokenIssued || lastToken != 0) {
                    Serial.println("Sync: Queue reset detected! Resetting display to 0.");
                    lastToken   = 0;
                    tokenIssued = false;
                    showResetOLED();
                    delay(1200);
                    showIdleOLED();
                } else if (initialSync) {
                    lastToken   = 0;
                    tokenIssued = false;
                    showIdleOLED();
                }
            } else {
                // Server has active tokens
                if (lastToken != serverToken || !tokenIssued) {
                    Serial.print("Sync: Updating token to ");
                    Serial.println(serverToken);
                    lastToken   = serverToken;
                    tokenIssued = true;
                    showIdleOLED();
                }
            }
        }
    }
    http.end();
}

// ============================================================
//  DEVICE AUTO-REGISTRATION
// ============================================================

void registerDispenserWithServer() {
    if (WiFi.status() != WL_CONNECTED || serverURL.length() == 0) return;
    HTTPClient http;
    http.setTimeout(3000);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    http.begin(serverURL + "/api/device/register");
    http.addHeader("Content-Type", "application/json");
    String body = "{\"device_id\":\"" + String(macStr) + "\",\"device_type\":\"dispenser\"}";
    http.POST(body);
    http.end();
}

// ============================================================
//  WIFI CONNECTION
// ============================================================

void connectWiFi() {
    Serial.println("\nConnecting to WiFi: " + ssid);
    showOLED("Connecting WiFi", ssid, "Please wait...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
        showOLED("WiFi Connected!", WiFi.localIP().toString(),
                 "", "Syncing token...");
        delay(800);
        registerDispenserWithServer();
        checkServerSync(true);
        showIdleOLED();
    } else {
        Serial.println("\nFailed to connect.");
        showOLED("WiFi Failed!", "Hold btn 10s", "to reconfigure");
    }
}

// ============================================================
//  CAPTIVE PORTAL — AP MODE
// ============================================================

void startAP() {
    apMode = true;
    WiFi.mode(WIFI_AP);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apSSID[32];
    snprintf(apSSID, sizeof(apSSID), "GTM4Health Unit-1-%02X%02X", mac[4], mac[5]);
    WiFi.softAP(apSSID);
    delay(500);
    IPAddress apIP = WiFi.softAPIP();
    String portalURL = "http://" + apIP.toString() + "/";

    Serial.println("\n=== Config Portal ===");
    Serial.println("SSID: " + String(apSSID));
    Serial.print("Then open: "); Serial.println(portalURL);

    showOLED("Config Mode (AP)", String(apSSID), "Auto-opens setup", apIP.toString());

    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/", []() {
        webServer.send_P(200, "text/html", setupPage);
    });

    // Captive-portal probe redirects (Android / iOS / Windows / macOS)
    auto redir = [portalURL]() {
        webServer.sendHeader("Location", portalURL, true);
        webServer.send(302, "text/plain", "");
    };
    webServer.on("/generate_204",              redir);
    webServer.on("/gen_204",                   redir);
    webServer.on("/hotspot-detect.html",       redir);
    webServer.on("/library/test/success.html", redir);
    webServer.on("/ncsi.txt",                  redir);
    webServer.on("/connecttest.txt",           redir);
    webServer.on("/redirect",                  redir);
    webServer.on("/canonical.html",            redir);
    webServer.on("/success.txt",               redir);
    webServer.on("/fwlink",                    redir);
    webServer.onNotFound(redir);

    webServer.on("/save", []() {
        if (webServer.hasArg("ssid") && webServer.hasArg("server")) {
            ssid      = webServer.arg("ssid");
            password  = webServer.arg("password");
            serverURL = webServer.arg("server");
            saveWiFi();
            webServer.send_P(200, "text/html", savedPage);
            showOLED("Saved!", "Restarting...");
            // Wait 800ms for HTTP response to reach phone/PC, then shut down AP
            // so the user's device immediately switches back to internet WiFi
            // before the browser redirects to gtm-tms2.vercel.app
            delay(800);
            WiFi.softAPdisconnect(true);
            delay(400);
            ESP.restart();
        } else {
            webServer.send(400, "text/plain", "Missing ssid or server");
        }
    });

    webServer.begin();
    Serial.println("Captive portal started.");
}

// ============================================================
//  API — Generate Token
// ============================================================

void generateToken() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No WiFi.");
        showOLED("No WiFi!", "Reconnecting...");
        connectWiFi();
        return;
    }

    showOLED("Generating", "token...", "Please wait");

    HTTPClient http;
    http.setTimeout(5000);
    String url = serverURL + "/generate";
    Serial.print("POST -> "); Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST("{}");

    Serial.print("HTTP: "); Serial.println(code);

    if (code == 200) {
        String payload = http.getString();
        Serial.println(payload);
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            int token = doc["token"] | 0;
            if (token == 0) {
                String ts = doc["token"] | String("");
                token = ts.toInt();
            }
            if (token > 0) {
                lastToken   = token;
                tokenIssued = true;
                Serial.print("Token issued: "); Serial.println(token);
                showTokenOLED(token);
                // Short 1s display before returning to idle (reduced from 5s)
                buttonISRFlag = false;
                unsigned long showStart = millis();
                while (millis() - showStart < 1000) {
                    if (buttonISRFlag) break;  // allow immediate next press
                    delay(20);
                }
            } else {
                showOLED("Token Error", "Invalid response");
                delay(1200);
            }
        } else {
            showOLED("Parse Error", "Check server", "response format");
            delay(1500);
        }
    } else if (code == -1) {
        showOLED("Connect Error", "Server unreachable", "Check URL/WiFi");
        delay(1500);
    } else {
        showOLED("Server Error", "HTTP: " + String(code), "Try again");
        delay(1500);
    }

    http.end();

    // Back to idle (shows last token number)
    showIdleOLED();
    lastSyncTime = millis();
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(300);

    // I2C + OLED
    Wire.begin(21, 22);   // SDA=GPIO21, SCL=GPIO22
    u8g2.begin();
    showOLED("GTM4Health TMS", "Unit-1", "Starting...");
    delay(800);

    // Button pin + interrupt so taps during HTTP sync are never missed
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), onButtonFall, FALLING);

    Serial.println("\n=== Unit-1 : Token Dispenser ===");

    if (loadWiFi()) {
        Serial.println("Credentials found, connecting...");
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Failed → starting config portal");
            startAP();
        }
    } else {
        Serial.println("No credentials → starting config portal");
        startAP();
    }
}

// ============================================================
//  LOOP
// ============================================================

void loop() {

    // AP mode — serve captive portal
    if (apMode) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        return;
    }

    webServer.handleClient();

    // ── Button on GPIO 4 ─────────────────────────────────────
    bool cur = digitalRead(buttonPin);

    if (cur == LOW && lastButtonState == HIGH) {
        // Falling edge — button pressed
        buttonPressStart = millis();
        buttonISRFlag    = false;
    }

    // Check if held for 10s while still holding
    if (cur == LOW && buttonPressStart > 0 && (millis() - buttonPressStart >= 10000)) {
        Serial.println("10s hold → clearing WiFi credentials...");
        showOLED("Clearing WiFi", "Restarting...");
        clearWiFi();
        delay(1000);
        ESP.restart();
    }

    if (cur == HIGH && lastButtonState == LOW) {
        // Rising edge — button released
        unsigned long held = millis() - buttonPressStart;
        buttonPressStart = 0;
        buttonISRFlag    = false;

        if (held >= 30 && held < 10000) {
            // Normal press → generate token
            Serial.println("Button pressed → generating token");
            generateToken();
        }
    } else if (cur == HIGH && buttonISRFlag) {
        // Quick tap occurred during background sync HTTP request
        buttonISRFlag = false;
        Serial.println("Button tap (ISR) → generating token");
        generateToken();
    }

    lastButtonState = cur;

    // ── Background sync with server (detects reset immediately) ──
    if (cur == HIGH && !buttonISRFlag && millis() - lastSyncTime >= SYNC_INTERVAL_MS) {
        lastSyncTime = millis();
        checkServerSync(false);
    }

    // ── Auto-reconnect if WiFi drops ─────────────────────────
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost → reconnecting...");
        showOLED("WiFi lost...", "Reconnecting");
        connectWiFi();
    }
}
