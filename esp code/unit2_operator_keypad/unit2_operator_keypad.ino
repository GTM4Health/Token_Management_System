// ============================================================
//  GTM4Health TMS — Unit-2 : Operator Panel
//
//  Hardware:
//    ESP32
//    1.3" OLED (SH1106 / SSD1306 128×64) — I2C
//        VCC → 3.3V      GND → GND
//        SCK → GPIO 22   SDA → GPIO 21
//
//  4×4 Membrane Keypad — wire LEFT TO RIGHT on connector:
//  ┌───────────────────────────────────────────────────────────┐
//  │ Keypad pin │  1  │  2  │  3  │  4  │  5  │  6  │  7  │  8 │
//  │ Signal     │  R1 │  R2 │  R3 │  R4 │  C1 │  C2 │  C3 │  C4│
//  │ ESP32 GPIO │  13 │  12 │  14 │  27 │  26 │  25 │  33 │  32│
//  └───────────────────────────────────────────────────────────┘
//
//  Keypad layout:
//    [ 1 ][ 2 ][ 3 ][ A ]
//    [ 4 ][ 5 ][ 6 ][ B ]
//    [ 7 ][ 8 ][ 9 ][ C ]
//    [ * ][ 0 ][ # ][ D ]
//
//  Physical Buttons (momentary push — one leg → GPIO, other → GND)
//        CALL NEXT  → GPIO 15
//        CALL AGAIN → GPIO 19
//        RESET      → GPIO 5    (hold 5 s to confirm)
//
//  ── KEYPAD — number entry only ──────────────────────────────
//    0–9  : type the token number (shown live in large font)
//    #    : confirm → calls that token  (POST /call_specific)
//    *    : backspace one digit
//           (no digits typed + hold 10 s → clear WiFi & reconfigure)
//    A B C D : NOT USED
//
//  ── PHYSICAL BUTTONS ────────────────────────────────────────
//    NEXT  (GPIO 15) : call next token in queue  (POST /next)
//    AGAIN (GPIO 19) : call same token again     (POST /call_again)
//    RESET (GPIO  5) : hold 5 s → reset queue   (POST /reset)
//
//  Libraries (Arduino Library Manager):
//    • ArduinoJson  (Benoit Blanchon)
//    • U8g2         (oliver)
//    • Keypad       (Mark Stanley, Alexander Brevig)
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Keypad.h>

// ── OLED ─────────────────────────────────────────────────────
// SH1106 (most 1.3" I2C modules):
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// SSD1306 alternative — uncomment if needed:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ── 4×4 Keypad ───────────────────────────────────────────────
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};   // R1 R2 R3 R4 → keypad connector pins 1-4
byte colPins[COLS] = {26, 25, 33, 32};   // C1 C2 C3 C4 → keypad connector pins 5-8
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ── Physical button pins ──────────────────────────────────────
const int PIN_NEXT  = 15;
const int PIN_AGAIN = 19;   // ← changed from GPIO 2 (strapping/LED pin)
const int PIN_RESET = 5;    // ← changed from GPIO 4

// ── WiFi / Server / Counter Identity ─────────────────────────────
String ssid        = "";
String password    = "";
String serverURL   = "";
String unitName    = "Calling Unit";
String counterName = "Room 1";

WebServer  webServer(80);
DNSServer  dnsServer;
const byte DNS_PORT = 53;
Preferences preferences;
bool apMode = false;

// ── Token input buffer ────────────────────────────────────────
String inputBuffer = "";
const int MAX_DIGITS = 3;

// ── Last served info ──────────────────────────────────────────
String lastServed = "";
bool   hasServed  = false;

// ── * key hold (WiFi clear) ───────────────────────────────────
unsigned long starStart = 0;
bool          starHeld  = false;

// ── Physical button edge-detection states ────────────────────
//    Initialised to actual pin readings in setup() so that
//    a LOW pin at boot does NOT fire a false press event.
bool lastNextState  = HIGH;
bool lastAgainState = HIGH;
bool lastResetState = HIGH;



// ============================================================
//  OLED HELPERS
// ============================================================

void drawOLED(const String& l1, const String& l2 = "",
              const String& l3 = "", const String& l4 = "") {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    if (l1.length()) u8g2.drawStr(0, 12, l1.c_str());
    if (l2.length()) u8g2.drawStr(0, 26, l2.c_str());
    if (l3.length()) u8g2.drawStr(0, 42, l3.c_str());
    if (l4.length()) u8g2.drawStr(0, 56, l4.c_str());
    u8g2.sendBuffer();
}

void drawBigToken(const String& label, const String& token) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tf);
    u8g2.drawStr(0, 13, label.c_str());
    u8g2.drawHLine(0, 16, 128);
    u8g2.setFont(u8g2_font_logisoso32_tf);
    String t = token.length() ? token : "-";
    int w = u8g2.getStrWidth(t.c_str());
    u8g2.drawStr((128 - w) / 2, 55, t.c_str());
    u8g2.sendBuffer();
}

// Live input — shows digits as operator types in large font
void drawInputOLED() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tf);
    u8g2.drawStr(0, 13, "Call Token:");
    u8g2.drawHLine(0, 16, 128);
    u8g2.setFont(u8g2_font_logisoso32_tf);
    String d = inputBuffer.length() ? inputBuffer : "_";
    int w = u8g2.getStrWidth(d.c_str());
    u8g2.drawStr((128 - w) / 2, 55, d.c_str());
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 63, "#=Call  *=Backspace");
    u8g2.sendBuffer();
}

void showIdle() {
    if (hasServed && lastServed.length()) {
        drawBigToken(counterName + ":", lastServed);
    } else {
        drawOLED("GTM4Health TMS",
                 counterName,
                 "Unit-2 Ready",
                 "Type token + #");
    }
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
  <div class="sub">UNIT-2 &mdash; OPERATOR PANEL SETUP</div>
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
    <div style="background:#f4f8fb;border:1px solid #dde6ed;border-radius:8px;padding:12px;margin-top:16px;font-size:13px;line-height:1.4;color:#4a5c6d;">
      <strong>&#10003; Auto-Naming Active:</strong> This unit is automatically numbered (Calling Unit 1, 2, etc.) by the server and remembered. You can customize the unit name and &quot;Please proceed to&quot; destination anytime from the web <strong>/admin</strong> dashboard.
    </div>
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
  <p>Device is restarting and connecting to WiFi...</p>
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
//  DEVICE MAC & FLASH STORAGE
// ============================================================

String getDeviceMAC() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void saveWiFi() {
    preferences.begin("wifi", false);
    preferences.putString("ssid",      ssid);
    preferences.putString("password",  password);
    preferences.putString("server",    serverURL);
    preferences.putString("unit_name", unitName);
    preferences.putString("counter",   counterName);
    preferences.end();
}

bool loadWiFi() {
    preferences.begin("wifi", true);
    ssid        = preferences.getString("ssid",      "");
    password    = preferences.getString("password",  "");
    serverURL   = preferences.getString("server",    "");
    unitName    = preferences.getString("unit_name", "Calling Unit");
    counterName = preferences.getString("counter",   "Room 1");
    preferences.end();
    if (unitName.length() == 0) unitName = "Calling Unit";
    if (counterName.length() == 0) counterName = "Room 1";
    return !(ssid == "" || serverURL == "");
}

void clearWiFi() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
}

// ============================================================
//  DEVICE AUTO-REGISTRATION (Server Handshake)
// ============================================================

void registerDeviceWithServer() {
    if (WiFi.status() != WL_CONNECTED || serverURL.length() == 0) return;

    HTTPClient http;
    http.setTimeout(4000);
    String url = serverURL + "/api/device/register";
    String mac = getDeviceMAC();

    Serial.println("Registering device MAC: " + mac);
    drawOLED("Connecting Server", mac, "Registering Unit");

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String body = "{\"device_id\":\"" + mac + "\",\"device_type\":\"calling_unit\"}";
    int code = http.POST(body);

    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            if (doc.containsKey("unit_name"))    unitName    = String((const char*)doc["unit_name"]);
            if (doc.containsKey("proceed_text")) counterName = String((const char*)doc["proceed_text"]);
            saveWiFi();
            Serial.println("Registered! " + unitName + " -> " + counterName);
            drawOLED(unitName, counterName, "Ready.");
            delay(1000);
        }
    } else {
        Serial.print("Registration HTTP code: "); Serial.println(code);
    }
    http.end();
}

// ============================================================
//  WIFI
// ============================================================

void connectWiFi() {
    Serial.println("\nConnecting: " + ssid);
    drawOLED("Connecting WiFi", ssid, unitName, "Please wait...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
        delay(500); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
        registerDeviceWithServer();
        showIdle();
    } else {
        Serial.println("\nFailed.");
        drawOLED("WiFi Failed!", "Hold * (10s,", "no digits) to reset");
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
    if (unitName.length() && unitName != "Calling Unit") {
        snprintf(apSSID, sizeof(apSSID), "GTM4Health %s", unitName.c_str());
    } else {
        snprintf(apSSID, sizeof(apSSID), "GTM4Health Unit-2-%02X%02X", mac[4], mac[5]);
    }
    WiFi.softAP(apSSID);
    delay(500);
    IPAddress apIP = WiFi.softAPIP();
    String portalURL = "http://" + apIP.toString() + "/";

    Serial.println("\n=== Config Portal ===");
    Serial.println("SSID: " + String(apSSID));
    Serial.print("Open: "); Serial.println(portalURL);
    drawOLED(unitName, String(apSSID), "Auto-opens setup", apIP.toString());

    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/", []() { webServer.send_P(200, "text/html", setupPage); });

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
            ssid        = webServer.arg("ssid");
            password    = webServer.arg("password");
            serverURL   = webServer.arg("server");
            if (webServer.hasArg("counter") && webServer.arg("counter").length() > 0) {
                counterName = webServer.arg("counter");
            }
            saveWiFi();
            webServer.send_P(200, "text/html", savedPage);
            drawOLED("Saved!", unitName, "Restarting...");
            delay(800);
            WiFi.softAPdisconnect(true);
            delay(400);
            ESP.restart();
        } else {
            webServer.send(400, "text/plain", "Missing ssid or server");
        }
    });
    webServer.begin();
}

// ============================================================
//  API CALLS
// ============================================================

String apiPost(const String& endpoint, const String& body = "{}") {
    if (WiFi.status() != WL_CONNECTED) { Serial.println("No WiFi!"); return ""; }
    HTTPClient http;
    http.setTimeout(4000);
    String url = serverURL + endpoint;
    Serial.print("POST "); Serial.print(url);
    Serial.print(" | "); Serial.println(body);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    Serial.print("HTTP: "); Serial.println(code);
    String resp = (code > 0) ? http.getString() : "";
    http.end();
    return resp;
}

String parseServedToken(const String& json) {
    JsonDocument doc;
    if (!deserializeJson(doc, json)) {
        if (doc.containsKey("serving")) return String((const char*)doc["serving"]);
        if (doc.containsKey("token"))   return String((const char*)doc["token"]);
    }
    return "";
}

// ── Call Next (resumes sequential queue) ─────────────────────
void doCallNext() {
    Serial.println("ACTION: Call Next for " + unitName + " (" + counterName + ")");
    drawOLED("Calling Next...", counterName, "Please wait");
    String body = "{\"device_id\":\"" + getDeviceMAC() + "\",\"counter\":\"" + counterName + "\"}";
    String resp = apiPost("/next", body);
    if (resp.length()) {
        String t = parseServedToken(resp);
        if (t.length() && t != "---") {
            lastServed = t; hasServed = true;
            drawBigToken(counterName + ":", lastServed);
        } else {
            drawOLED("No waiting", "tokens in queue", counterName);
        }
    } else {
        drawOLED("Server Error", "Check WiFi/URL");
    }
    delay(1000);
    showIdle();
}

// ── Call Again (same token) ───────────────────────────────────
void doCallAgain() {
    Serial.println("ACTION: Call Again for " + unitName + " (" + counterName + ")");
    drawOLED("Calling Again...", counterName, "Please wait");
    String body = "{\"device_id\":\"" + getDeviceMAC() + "\",\"counter\":\"" + counterName + "\"}";
    String resp = apiPost("/call_again", body);
    if (resp.length()) {
        String t = parseServedToken(resp);
        if (t.length() && t != "---") {
            lastServed = t; hasServed = true;
            drawBigToken(counterName + ":", lastServed);
        } else {
            drawOLED("Nothing serving", "on " + counterName);
        }
    } else {
        drawOLED("Server Error", "Check WiFi/URL");
    }
    delay(1000);
    showIdle();
}

// ── Call Specific Token (typed number + #) ───────────────────
void doCallSpecific() {
    if (inputBuffer.length() == 0) {
        drawOLED("No number typed", "Type token then #");
        delay(1200);
        showIdle();
        return;
    }
    // Zero-pad to 3 digits (server stores as "001", "010" etc.)
    String padded = inputBuffer;
    while (padded.length() < 3) padded = "0" + padded;

    Serial.println("ACTION: Call Specific -> " + padded + " for " + unitName + " (" + counterName + ")");
    drawOLED("Calling Token:", padded, counterName, "Please wait...");

    String body = "{\"token\":\"" + padded + "\",\"device_id\":\"" + getDeviceMAC() + "\",\"counter\":\"" + counterName + "\"}";
    String resp = apiPost("/call_specific", body);

    if (resp.length()) {
        if (resp.indexOf("Invalid Token") >= 0 || resp.indexOf("404") >= 0) {
            drawOLED("Not found:", padded, "Check & try again");
            delay(1500);
        } else {
            lastServed = padded; hasServed = true;
            drawBigToken(counterName + ":", lastServed);
            delay(1200);
        }
    } else {
        drawOLED("Server Error", "Check WiFi/URL");
        delay(1200);
    }
    inputBuffer = "";
    showIdle();
}

// ── Reset Queue ───────────────────────────────────────────────
void doResetQueue() {
    Serial.println("ACTION: Reset Queue");
    drawOLED("Resetting...", "Please wait");
    String resp = apiPost("/reset");
    if (resp.length()) {
        lastServed = ""; hasServed = false;
        inputBuffer = "";
        drawOLED("Queue Reset!", "All tokens cleared.");
    } else {
        drawOLED("Server Error", "Check WiFi/URL");
    }
    delay(1200);
    showIdle();
}

// ── * key raw-scan for hold detection ────────────────────────
bool isStarHeld() {
    // * is at row 3, col 0 in our layout
    const byte r = 3, c = 0;
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], LOW);
    pinMode(colPins[c], INPUT_PULLUP);
    bool held = (digitalRead(colPins[c]) == LOW);
    pinMode(rowPins[r], INPUT);
    return held;
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(300);

    Wire.begin(21, 22);
    u8g2.begin();
    drawOLED("GTM4Health TMS", "Unit-2", "Starting...");
    delay(1200);

    // Set button pins with pull-ups FIRST
    pinMode(PIN_NEXT,  INPUT_PULLUP);
    pinMode(PIN_AGAIN, INPUT_PULLUP);
    pinMode(PIN_RESET, INPUT_PULLUP);

    // Small settle time so pins stabilise after mode change
    delay(100);

    // *** Critical: read ACTUAL pin state NOW to seed edge detection.
    //     This prevents a false press event on the first loop iteration
    //     if any pin happens to read LOW at startup.
    lastNextState  = digitalRead(PIN_NEXT);
    lastAgainState = digitalRead(PIN_AGAIN);
    lastResetState = digitalRead(PIN_RESET);

    Serial.println("\n=== Unit-2 : Operator Panel ===");

    if (loadWiFi()) {
        Serial.println("Credentials found, connecting...");
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) startAP();
    } else {
        Serial.println("No credentials → config portal");
        startAP();
    }
}

// ============================================================
//  LOOP
// ============================================================

void loop() {

    if (apMode) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        return;
    }

    webServer.handleClient();

    // ── Read keypad (non-null only on a NEW distinct key event) ─
    char key = keypad.getKey();

    // ── Read physical buttons ─────────────────────────────────
    bool nextPressed  = (digitalRead(PIN_NEXT)  == LOW);
    bool againPressed = (digitalRead(PIN_AGAIN) == LOW);
    bool resetPressed = (digitalRead(PIN_RESET) == LOW);

    if (key) { Serial.print("Key: "); Serial.println(key); }

    // ==========================================================
    //  KEYPAD: number entry (0–9)
    // ==========================================================
    if (key >= '0' && key <= '9') {
        if (inputBuffer.length() < MAX_DIGITS) inputBuffer += key;
        drawInputOLED();
    }

    // ==========================================================
    //  KEYPAD: # → call the typed token
    // ==========================================================
    if (key == '#') {
        doCallSpecific();
    }

    // ==========================================================
    //  KEYPAD: * → backspace while typing
    //           * with no digits typed + hold 10 s → clear WiFi
    // ==========================================================
    if (key == '*') {
        if (inputBuffer.length() > 0) {
            // Backspace
            inputBuffer.remove(inputBuffer.length() - 1);
            if (inputBuffer.length() == 0) showIdle();
            else drawInputOLED();
        } else {
            // Start WiFi-clear hold timer
            starStart = millis();
            starHeld  = true;
            drawOLED("Hold * for 10s", "to clear WiFi", "& reconfigure");
        }
    }
    if (starHeld) {
        if (!isStarHeld()) {
            // Released early — cancel
            starHeld = false;
            showIdle();
        } else if (millis() - starStart >= 10000) {
            starHeld = false;
            drawOLED("Clearing WiFi", "Restarting...");
            clearWiFi();
            delay(1000);
            ESP.restart();
        }
    }

    // ==========================================================
    //  PHYSICAL NEXT button (GPIO 15)
    //  Fires ONCE on the press moment (HIGH → LOW edge)
    // ==========================================================
    if (nextPressed && (lastNextState == HIGH)) {
        Serial.println("NEXT btn");
        inputBuffer = "";
        doCallNext();
    }
    lastNextState = nextPressed;

    // ==========================================================
    //  PHYSICAL AGAIN button (GPIO 19)
    //  Fires ONCE on the press moment (HIGH → LOW edge)
    // ==========================================================
    if (againPressed && (lastAgainState == HIGH)) {
        Serial.println("AGAIN btn");
        inputBuffer = "";
        doCallAgain();
    }
    lastAgainState = againPressed;

    // ==========================================================
    //  PHYSICAL RESET button (GPIO 5) — must hold 5 s
    //  Uses a blocking poll so button bounce cannot cause false cancel.
    //  On press-edge: blocks for up to 5 s reading the pin every 20 ms.
    //  Only fires reset if button is held the entire 5 s.
    // ==========================================================
    if (resetPressed && (lastResetState == HIGH)) {
        Serial.println("RESET btn pressed — waiting for 5s hold...");
        drawOLED("Hold RESET 5s", "to Reset Queue", "Release to cancel");

        delay(50);  // initial debounce — ignore first 50 ms of noise

        unsigned long holdStart = millis();
        bool held = true;

        while (millis() - holdStart < 5000) {
            if (digitalRead(PIN_RESET) == HIGH) {
                // Button released before 5 s
                held = false;
                break;
            }
            delay(20);  // poll every 20 ms
        }

        if (held) {
            Serial.println("RESET confirmed — executing");
            inputBuffer = "";
            doResetQueue();
        } else {
            Serial.println("RESET cancelled — released early");
            drawOLED("Cancelled.", "Hold 5s to reset.");
            delay(1200);
            showIdle();
        }

        // Wait until button is fully released before returning to loop
        while (digitalRead(PIN_RESET) == LOW) delay(10);
        lastResetState = HIGH;
    } else {
        lastResetState = resetPressed;
    }

    // ==========================================================
    //  Auto-reconnect if WiFi drops
    // ==========================================================
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost → reconnecting...");
        drawOLED("WiFi lost...", "Reconnecting");
        connectWiFi();
    }
}
