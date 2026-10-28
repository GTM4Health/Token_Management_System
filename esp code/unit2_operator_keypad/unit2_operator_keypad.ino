// ============================================================
//  GTM4Health TMS — Unit-2 : Calling Unit (Push-Button Only)
//
//  Hardware:
//    ESP32
//    Physical Buttons (momentary push — one leg -> GPIO, other -> GND):
//      CALL NEXT  -> GPIO 15  (INPUT_PULLUP)
//      CALL AGAIN -> GPIO 13  (INPUT_PULLUP)
//      RESET      -> GPIO 14  (INPUT_PULLUP, hold 5 s to confirm)
//
//  Button Behaviour:
//    NEXT (GPIO 15)  : Short press -> Call next token in queue (POST /next)
//                      Hold 10 s   -> Clear saved WiFi & enter AP setup mode
//    AGAIN (GPIO 13) : Short press -> Repeat last called token (POST /call_again)
//    RESET (GPIO 14) : Hold 5 s    -> Reset entire queue (POST /reset)
//
//  NO OLED, NO KEYPAD.
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// ── WiFi & Server Credentials (stored in NVS flash) ───────────
String ssid        = "";
String password    = "";
String serverURL   = "";
String unitName    = "Calling Unit 1";
String proceedText = "Room 1";

// ── Web / DNS (Captive Portal for WiFi Setup) ─────────────────
WebServer webServer(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;
Preferences preferences;
bool apMode = false;

// ── Physical Push Button Pins ─────────────────────────────────
const int nextButton  = 15;
const int againButton = 13;
const int resetButton = 14;
unsigned long lastPingTime = 0;

// ── Helper: Read Unique Hardware MAC ──────────────────────────
String getDeviceMAC() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// ============================================================
//  CAPTIVE PORTAL HTML (Clean Setup Page)
// ============================================================

const char setupPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GTM4Health TMS Calling Unit Setup</title>
<style>
body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#eef6fb;display:flex;justify-content:center;align-items:center;min-height:100vh;box-sizing:border-box;}
.container{background:white;padding:30px;border-radius:12px;width:100%;max-width:400px;box-shadow:0 4px 20px rgba(0,0,0,.15);border-top:6px solid #1683c4;box-sizing:border-box;}
h2{color:#0b4f7c;margin:0 0 6px 0;text-align:center;font-size:20px;}
.sub{color:#607080;font-size:13px;text-align:center;margin-bottom:20px;letter-spacing:1px;}
label{color:#607080;font-size:13px;font-weight:600;letter-spacing:1px;display:block;margin-top:14px;}
input[type=text],input[type=password]{width:100%;padding:12px;margin-top:5px;border:1px solid #c6d0d8;border-radius:8px;font-size:15px;box-sizing:border-box;}
.note{background:#f4f8fb;border:1px solid #dde6ed;border-radius:8px;padding:12px;margin-top:16px;font-size:13px;line-height:1.4;color:#4a5c6d;}
button{width:100%;padding:14px;margin-top:20px;background:#0b4f7c;color:white;border:none;font-size:17px;border-radius:8px;cursor:pointer;font-weight:600;}
.hint{color:#888;font-size:12px;text-align:center;margin-top:10px;}
</style>
</head>
<body>
<div class="container">
  <h2>&#128268; GTM4Health TMS</h2>
  <div class="sub">CALLING UNIT SETUP</div>
  <form action="/save" method="GET">
    <label>WIFI NAME (SSID)</label>
    <input type="text" name="ssid" placeholder="Your WiFi name" required>
    <label>PASSWORD</label>
    <input type="password" name="password" placeholder="WiFi password">
    <label>SERVER URL</label>
    <input type="text" name="server" value="https://token-management-system-cvo8.onrender.com" required>
    <div class="note">
      <strong>&#10003; Auto-Registration:</strong> This calling unit is automatically numbered (Calling Unit 1, 2, etc.) on connect. You can customize the name and "Please proceed to" destination anytime from the web <strong>/admin</strong> dashboard.
    </div>
    <button type="submit">Connect &amp; Save</button>
  </form>
  <p class="hint">Leave password blank for open networks.</p>
</div>
</body>
</html>
)rawliteral";

const char savedPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings Saved!</title>
<style>
body{font-family:Arial,sans-serif;margin:0;background:#eef6fb;display:flex;justify-content:center;align-items:center;min-height:100vh;}
.box{background:white;padding:40px 30px;border-radius:12px;text-align:center;border-top:6px solid #27ae60;max-width:360px;width:90%;box-shadow:0 4px 20px rgba(0,0,0,.12);}
h2{color:#27ae60;margin:0 0 12px 0;}
p{color:#555;margin:8px 0;line-height:1.5;}
</style>
</head>
<body>
<div class="box">
  <h2>&#10003; Settings Saved!</h2>
  <p>Connecting to WiFi and registering with server...</p>
</div>
</body>
</html>
)rawliteral";

// ============================================================
//  NVS FLASH STORAGE
// ============================================================

void saveWiFi() {
    preferences.begin("wifi", false);
    preferences.putString("ssid",         ssid);
    preferences.putString("password",     password);
    preferences.putString("server",       serverURL);
    preferences.putString("unit_name",    unitName);
    preferences.putString("proceed_text", proceedText);
    preferences.end();
}

bool loadWiFi() {
    preferences.begin("wifi", true);
    ssid        = preferences.getString("ssid",         "");
    password    = preferences.getString("password",     "");
    serverURL   = preferences.getString("server",       "");
    unitName    = preferences.getString("unit_name",    "Calling Unit 1");
    proceedText = preferences.getString("proceed_text", "Room 1");
    preferences.end();
    return !(ssid == "" || serverURL == "");
}

void clearWiFi() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    Serial.println("Saved WiFi Cleared!");
}

// ============================================================
//  DEVICE AUTO-REGISTRATION (Server Handshake)
// ============================================================

void registerWithServer() {
    if (WiFi.status() != WL_CONNECTED || serverURL.length() == 0) return;

    HTTPClient http;
    http.setTimeout(4000);
    String url = serverURL + "/api/device/register";
    String mac = getDeviceMAC();

    Serial.println("Registering Calling Unit: " + mac);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String body = "{\"device_id\":\"" + mac + "\",\"device_type\":\"calling_unit\"}";
    int code = http.POST(body);

    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            if (doc.containsKey("unit_name"))    unitName    = String((const char*)doc["unit_name"]);
            if (doc.containsKey("proceed_text")) proceedText = String((const char*)doc["proceed_text"]);
            saveWiFi();
            Serial.println("Registered! Unit: " + unitName + " -> " + proceedText);
        }
    } else {
        Serial.print("Registration failed, HTTP: ");
        Serial.println(code);
    }
    http.end();
}

// ============================================================
//  WIFI CONNECTION
// ============================================================

void connectWiFi() {
    Serial.println("\nConnecting to WiFi: " + ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
        registerWithServer();
    } else {
        Serial.println("\nWiFi Connection Failed.");
    }
}

// ============================================================
//  START AP (Captive Portal)
// ============================================================

void startAP() {
    apMode = true;
    WiFi.mode(WIFI_AP);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apSSID[32];
    snprintf(apSSID, sizeof(apSSID), "GTM4Health Caller-%02X%02X", mac[4], mac[5]);
    WiFi.softAP(apSSID);
    delay(500);

    IPAddress apIP = WiFi.softAPIP();
    String portalURL = "http://" + apIP.toString() + "/";

    Serial.println("\n=========================================");
    Serial.println("Configuration Mode (AP)");
    Serial.println("SSID : " + String(apSSID));
    Serial.println("Open : " + portalURL);
    Serial.println("=========================================");

    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/", []() {
        webServer.send_P(200, "text/html", setupPage);
    });

    auto redir = [portalURL]() {
        webServer.sendHeader("Location", portalURL, true);
        webServer.send(302, "text/plain", "");
    };
    webServer.on("/generate_204",        redir);
    webServer.on("/gen_204",             redir);
    webServer.on("/hotspot-detect.html", redir);
    webServer.on("/ncsi.txt",            redir);
    webServer.on("/connecttest.txt",     redir);
    webServer.onNotFound(redir);

    webServer.on("/save", []() {
        if (webServer.hasArg("ssid") && webServer.hasArg("server")) {
            ssid      = webServer.arg("ssid");
            password  = webServer.arg("password");
            serverURL = webServer.arg("server");
            saveWiFi();
            webServer.send_P(200, "text/html", savedPage);
            delay(1000);
            ESP.restart();
        } else {
            webServer.send(400, "text/plain", "Missing ssid or server");
        }
    });

    webServer.begin();
}

// ============================================================
//  API ACTIONS (NEXT / AGAIN / RESET)
// ============================================================

String apiPost(const String& endpoint) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No WiFi!");
        return "";
    }
    HTTPClient http;
    http.setTimeout(4000);
    String url = serverURL + endpoint;

    String body = "{\"device_id\":\"" + getDeviceMAC() + "\"}";
    Serial.print("POST " + url + " | " + body);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);

    Serial.println(" -> HTTP: " + String(code));
    String resp = (code > 0) ? http.getString() : "";
    http.end();
    return resp;
}

void doCallNext() {
    Serial.println("\n>>> ACTION: CALL NEXT");
    String resp = apiPost("/next");
    if (resp.length()) {
        Serial.println("Response: " + resp);
    }
}

void doCallAgain() {
    Serial.println("\n>>> ACTION: CALL AGAIN");
    String resp = apiPost("/call_again");
    if (resp.length()) {
        Serial.println("Response: " + resp);
    }
}

void doResetQueue() {
    Serial.println("\n>>> ACTION: RESET QUEUE");
    String resp = apiPost("/reset");
    if (resp.length()) {
        Serial.println("Response: " + resp);
    }
}

// ============================================================
//  SETUP & LOOP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(nextButton,  INPUT_PULLUP);
    pinMode(againButton, INPUT_PULLUP);
    pinMode(resetButton, INPUT_PULLUP);

    Serial.println();
    Serial.println("=========================================");
    Serial.println("Starting GTM4Health Calling Unit...");
    Serial.println("Hardware: Push Buttons on GPIO 15 (Next), 13 (Again), 14 (Reset)");
    Serial.println("=========================================");

    if (loadWiFi()) {
        Serial.println("Saved WiFi found. Connecting...");
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            startAP();
        }
    } else {
        Serial.println("No WiFi credentials. Starting AP setup portal...");
        startAP();
    }
}

void loop() {
    if (apMode) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        return;
    }

    // ── Background heartbeat ping every 2.5 s to maintain "Online" status ──
    if (WiFi.status() == WL_CONNECTED && millis() - lastPingTime >= 2500) {
        lastPingTime = millis();
        HTTPClient http;
        http.setTimeout(1200);
        http.begin(serverURL + "/api/device/ping?device_id=" + getDeviceMAC());
        http.GET();
        http.end();
    }

    // ── GPIO 15 : Call Next (short press) / WiFi config (hold 10 s) ──
    if (digitalRead(nextButton) == LOW) {
        unsigned long t0 = millis();
        while (digitalRead(nextButton) == LOW) {
            if (millis() - t0 >= 10000) {
                Serial.println("NEXT button held 10 s -> Clearing WiFi, entering config mode...");
                clearWiFi();
                delay(500);
                ESP.restart();
            }
            delay(10);
        }

        if (millis() - t0 >= 50 && millis() - t0 < 10000) {
            doCallNext();
            delay(300);
        }
    }

    // ── GPIO 13 : Call Again (short press) ──
    if (digitalRead(againButton) == LOW) {
        unsigned long t0 = millis();
        while (digitalRead(againButton) == LOW) {
            delay(10);
        }
        if (millis() - t0 >= 50) {
            doCallAgain();
            delay(300);
        }
    }

    // ── GPIO 14 : Reset Queue (hold 5 s to confirm) ──
    if (digitalRead(resetButton) == LOW) {
        unsigned long t0 = millis();
        Serial.println("Reset button pressed... Hold 5 s to confirm reset");
        bool confirmed = false;

        while (digitalRead(resetButton) == LOW) {
            if (millis() - t0 >= 5000) {
                confirmed = true;
                break;
            }
            delay(10);
        }

        if (confirmed) {
            doResetQueue();
            delay(1000);
            while (digitalRead(resetButton) == LOW) delay(10);
        } else {
            Serial.println("Reset cancelled (button released before 5 s).");
        }
    }

    delay(20);
}
