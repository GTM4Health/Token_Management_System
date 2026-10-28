// ============================================================
//  GTM4Health TMS — Unit-1 : Token Dispenser (Push-Button Only)
//
//  Hardware:
//    ESP32
//    Token Button -> GPIO 4 (other leg -> GND, internal pullup)
//
//  Button Behaviour:
//    Short press (<10 s) -> Generate sequential token (POST /generate)
//    Hold >=10 s         -> Clear saved WiFi & enter AP setup mode
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
String ssid      = "";
String password  = "";
String serverURL = "";
String unitName  = "Dispenser 1";

// ── Web / DNS (Captive Portal for WiFi Setup) ─────────────────
WebServer webServer(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;
Preferences preferences;
bool apMode = false;

// ── Push Button Pin ───────────────────────────────────────────
const int buttonPin = 4;
bool lastButtonState = HIGH;
unsigned long buttonPressStart = 0;
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
<title>GTM4Health TMS Setup</title>
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
  <div class="sub">TOKEN DISPENSER SETUP</div>
  <form action="/save" method="GET">
    <label>WIFI NAME (SSID)</label>
    <input type="text" name="ssid" placeholder="Your WiFi name" required>
    <label>PASSWORD</label>
    <input type="password" name="password" placeholder="WiFi password">
    <label>SERVER URL</label>
    <input type="text" name="server" value="https://token-management-system-cvo8.onrender.com" required>
    <div class="note">
      <strong>&#10003; Auto-Registration:</strong> This dispenser automatically registers with the server on connection. Manage its name &amp; location from the web <strong>/admin</strong> dashboard.
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
    preferences.putString("ssid",      ssid);
    preferences.putString("password",  password);
    preferences.putString("server",    serverURL);
    preferences.putString("unit_name", unitName);
    preferences.end();
}

bool loadWiFi() {
    preferences.begin("wifi", true);
    ssid      = preferences.getString("ssid",      "");
    password  = preferences.getString("password",  "");
    serverURL = preferences.getString("server",    "");
    unitName  = preferences.getString("unit_name", "Dispenser 1");
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

    Serial.println("Registering Dispenser: " + mac);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String body = "{\"device_id\":\"" + mac + "\",\"device_type\":\"dispenser\"}";
    int code = http.POST(body);

    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            if (doc.containsKey("unit_name")) {
                unitName = String((const char*)doc["unit_name"]);
                saveWiFi();
            }
            Serial.println("Registered! Name: " + unitName);
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
    snprintf(apSSID, sizeof(apSSID), "GTM4Health Dispenser-%02X%02X", mac[4], mac[5]);
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
//  GENERATE TOKEN API
// ============================================================

void generateToken() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot generate: WiFi Disconnected!");
        return;
    }

    HTTPClient http;
    String url = serverURL + "/generate";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String body = "{\"device_id\":\"" + getDeviceMAC() + "\"}";
    int code = http.POST(body);

    Serial.print("Generate HTTP code: ");
    Serial.println(code);

    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            Serial.print(">>> TOKEN ISSUED: ");
            Serial.println((const char*)doc["token"]);
        }
    } else {
        Serial.println("Failed to contact server.");
    }
    http.end();
}

// ============================================================
//  SETUP & LOOP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(buttonPin, INPUT_PULLUP);

    Serial.println();
    Serial.println("=========================================");
    Serial.println("Starting GTM4Health Token Dispenser...");
    Serial.println("Hardware: Push Button only on GPIO 4");
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

    // ── Background heartbeat ping every 30 s to maintain "Online" status ──
    if (WiFi.status() == WL_CONNECTED && millis() - lastPingTime >= 30000) {
        lastPingTime = millis();
        HTTPClient http;
        http.setTimeout(2500);
        http.begin(serverURL + "/api/device/ping?device_id=" + getDeviceMAC());
        http.GET();
        http.end();
    }

    // ── GPIO 4 Button Handling ─────────────────────────────────
    bool currentButtonState = digitalRead(buttonPin);

    // Button pressed down
    if (currentButtonState == LOW && lastButtonState == HIGH) {
        buttonPressStart = millis();
    }

    // Button released
    if (currentButtonState == HIGH && lastButtonState == LOW) {
        unsigned long heldFor = millis() - buttonPressStart;

        if (heldFor >= 10000) {
            Serial.println("Button held 10 s -> Clearing WiFi, entering config mode...");
            clearWiFi();
            delay(500);
            ESP.restart();
        } else if (heldFor >= 50) { // 50 ms debounce
            Serial.println("Button pressed: Generating Token...");
            generateToken();
        }
    }

    lastButtonState = currentButtonState;
    delay(20);
}
