#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

String ssid = "";
String password = "";
String serverURL = "";
bool printPopupEnabled = true;   // default: print popup ON

WebServer webServer(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

Preferences preferences;
bool apMode = false;

// ======================================================
// CAPTIVE PORTAL — Setup Page
// ======================================================

const char setupPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GTM4Health TMS Setup</title>
<style>
body{font-family:Arial;margin:0;padding:20px;background:#eef6fb;display:flex;justify-content:center;align-items:center;min-height:100vh;box-sizing:border-box;}
.container{background:white;padding:30px;border-radius:12px;width:100%;max-width:400px;box-shadow:0 4px 20px rgba(0,0,0,.15);border-top:6px solid #1683c4;box-sizing:border-box;}
h2{color:#0b4f7c;margin:0 0 6px 0;text-align:center;font-size:20px;}
.sub{color:#607080;font-size:13px;text-align:center;margin-bottom:20px;letter-spacing:1px;}
label{color:#607080;font-size:13px;font-weight:600;letter-spacing:1px;display:block;margin-top:14px;}
input[type=text],input[type=password]{width:100%;padding:12px;margin-top:5px;border:1px solid #c6d0d8;border-radius:8px;font-size:15px;box-sizing:border-box;}
.toggle-row{display:flex;align-items:center;justify-content:space-between;margin-top:18px;padding:12px 14px;background:#f4f8fb;border-radius:8px;border:1px solid #dde6ed;}
.toggle-label{color:#12314d;font-size:14px;font-weight:600;}
.toggle-sub{color:#8a9baa;font-size:11px;margin-top:2px;}
input[type=checkbox]{width:20px;height:20px;cursor:pointer;accent-color:#0b4f7c;}
button{width:100%;padding:14px;margin-top:20px;background:#0b4f7c;color:white;border:none;font-size:17px;border-radius:8px;cursor:pointer;font-weight:600;}
.hint{color:#888;font-size:12px;text-align:center;margin-top:10px;}
</style>
</head>
<body>
<div class="container">
  <h2>&#128268; GTM4Health TMS</h2>
  <div class="sub">WIFI SETUP</div>
  <form action="/save" method="GET">
    <label>WIFI NAME (SSID)</label>
    <input type="text" name="ssid" placeholder="Your WiFi name" required>
    <label>PASSWORD</label>
    <input type="password" name="password" placeholder="WiFi password">
    <label>SERVER URL</label>
    <input type="text" name="server" value="https://token-management-system-cvo8.onrender.com" required>
    <div class="toggle-row">
      <div>
        <div class="toggle-label">Print Pop-up</div>
        <div class="toggle-sub">Auto-open print window on token dispenser</div>
      </div>
      <input type="checkbox" name="printpopup" value="1" checked>
    </div>
    <button type="submit">Connect &amp; Save</button>
  </form>
  <p class="hint">Leave password blank for open networks.</p>
</div>
</body>
</html>
)rawliteral";

// ======================================================
// CAPTIVE PORTAL — Saved Page (auto-redirects to Vercel)
// ======================================================

const char savedPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings Saved!</title>
<style>
body{font-family:Arial;margin:0;background:#eef6fb;display:flex;justify-content:center;align-items:center;min-height:100vh;}
.box{background:white;padding:40px 30px;border-radius:12px;text-align:center;border-top:6px solid #27ae60;max-width:360px;width:90%;box-shadow:0 4px 20px rgba(0,0,0,.12);}
h2{color:#27ae60;margin:0 0 12px 0;}
p{color:#555;margin:8px 0;line-height:1.5;}
a{color:#1683c4;text-decoration:none;}
.count{font-size:40px;font-weight:700;color:#0b4f7c;margin:16px 0;}
</style>
</head>
<body>
<div class="box">
  <h2>&#10003; Settings Saved!</h2>
  <p>ESP32 is connecting to your WiFi...</p>
  <div class="count" id="c">5</div>
  <p>Redirecting to<br><a href="https://gtm-tms.vercel.app">gtm-tms.vercel.app</a></p>
</div>
<script>
var PRINT_ENABLED = "PRINT_FLAG";
var n=5;
var t=setInterval(function(){
  n--;document.getElementById('c').innerText=n;
  if(n<=0){
    clearInterval(t);
    localStorage.setItem('printPopup', PRINT_ENABLED);
    window.location.href='https://gtm-tms.vercel.app/kiosk';
  }
},1000);
</script>
</body>
</html>
)rawliteral";

// ======================================================
// BUTTON PINS
// ======================================================

const int buttonPin   = 4;
const int nextButton  = 15;
const int againButton = 13;
const int resetButton = 14;

bool lastButtonState      = HIGH;
unsigned long lastPressTime     = 0;
unsigned long resetPressStart   = 0;
unsigned long buttonPressStart  = 0;   // tracks GPIO4 hold duration

// ======================================================
// FLASH STORAGE
// ======================================================

void saveWiFi() {
    preferences.begin("wifi", false);
    preferences.putString("ssid",        ssid);
    preferences.putString("password",    password);
    preferences.putString("server",      serverURL);
    preferences.putBool("printpopup",    printPopupEnabled);
    preferences.end();
}

bool loadWiFi() {
    preferences.begin("wifi", true);
    ssid             = preferences.getString("ssid",      "");
    password         = preferences.getString("password",  "");
    serverURL        = preferences.getString("server",    "");
    printPopupEnabled = preferences.getBool("printpopup", true);
    preferences.end();
    if (ssid == "" || serverURL == "") return false;
    return true;
}

void clearWiFi() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    Serial.println("Saved WiFi Cleared!");
}

// ======================================================
// WIFI CONNECTION
// ======================================================

void connectWiFi() {
    Serial.println();
    Serial.println("Connecting to saved WiFi...");
    WiFi.mode(WIFI_STA);
    Serial.print("Connecting to "); Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("=========================================");
        Serial.println("WiFi Connected!");
        Serial.print("ESP32 IP : "); Serial.println(WiFi.localIP());
        Serial.print("Connected SSID : "); Serial.println(WiFi.SSID());
        Serial.print("Server URL : "); Serial.println(serverURL);
        Serial.println("=========================================");
    } else {
        Serial.println();
        Serial.println("Connection Failed.");
    }
}

// ======================================================
// START AP (Captive Portal)
// ======================================================

void startAP() {
    apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("GTM4Health Unit-1");   // open network, no password
    delay(500);

    IPAddress apIP = WiFi.softAPIP();
    Serial.println();
    Serial.println("=========================================");
    Serial.println("Configuration Mode");
    Serial.println("SSID : GTM4Health Unit-1  (no password needed)");
    Serial.print("Open : http://"); Serial.println(apIP);
    Serial.println("=========================================");

    // DNS captures ALL domains → ESP IP → triggers captive portal popup
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/", []() {
        webServer.send_P(200, "text/html", setupPage);
    });

    // Captive portal detection — Android, iOS, Windows all hit these
    webServer.on("/generate_204",        []() { webServer.sendHeader("Location", "/"); webServer.send(302, "text/plain", ""); });
    webServer.on("/gen_204",             []() { webServer.sendHeader("Location", "/"); webServer.send(302, "text/plain", ""); });
    webServer.on("/hotspot-detect.html", []() { webServer.sendHeader("Location", "/"); webServer.send(302, "text/plain", ""); });
    webServer.on("/ncsi.txt",            []() { webServer.sendHeader("Location", "/"); webServer.send(302, "text/plain", ""); });
    webServer.on("/connecttest.txt",     []() { webServer.sendHeader("Location", "/"); webServer.send(302, "text/plain", ""); });
    webServer.on("/redirect",            []() { webServer.sendHeader("Location", "/"); webServer.send(302, "text/plain", ""); });
    webServer.onNotFound([]() {
        webServer.sendHeader("Location", "http://192.168.4.1/");
        webServer.send(302, "text/plain", "");
    });

    webServer.on("/save", []() {
        if (webServer.hasArg("ssid") && webServer.hasArg("server")) {
            ssid              = webServer.arg("ssid");
            password          = webServer.arg("password");
            serverURL         = webServer.arg("server");
            printPopupEnabled = webServer.hasArg("printpopup") &&
                                webServer.arg("printpopup") == "1";
            saveWiFi();
            Serial.print("Print Popup: ");
            Serial.println(printPopupEnabled ? "ENABLED" : "DISABLED");

            // Build page dynamically so PRINT_FLAG is substituted correctly
            String page = String(savedPage);
            page.replace("PRINT_FLAG", printPopupEnabled ? "1" : "0");
            webServer.send(200, "text/html", page);
            delay(6000);
            ESP.restart();
        } else {
            webServer.send(400, "text/plain", "Missing ssid or server");
        }
    });

    webServer.begin();
    Serial.println("Web server started in AP mode");
}

// ======================================================
// API CALLS
// ======================================================

void generateToken() {
    if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi Disconnected!"); return; }
    HTTPClient http;
    String url = serverURL + "/generate";   // FIX: full HTTPS URL, no port
    Serial.print("URL = "); Serial.println(url);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.POST("{}");
    Serial.print("HTTP Response Code: "); Serial.println(responseCode);
    if (responseCode == 200) {
        String payload = http.getString();
        Serial.println(payload);
        JsonDocument doc;
        if (!deserializeJson(doc, payload)) {
            Serial.print("Generated Token : "); Serial.println((const char*)doc["token"]);
        }
    } else {
        Serial.println("Failed to contact server.");
    }
    http.end();
}

void callNext() {
    if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi Disconnected!"); return; }
    HTTPClient http;
    String url = serverURL + "/next";
    http.begin(url);
    int responseCode = http.POST("{}");
    Serial.print("Next Response Code: "); Serial.println(responseCode);
    if (responseCode > 0) Serial.println(http.getString());
    http.end();
}

void callAgain() {
    if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi Disconnected!"); return; }
    HTTPClient http;
    String url = serverURL + "/call_again";
    http.begin(url);
    int responseCode = http.POST("{}");
    Serial.print("Again Response Code: "); Serial.println(responseCode);
    if (responseCode > 0) Serial.println(http.getString());
    http.end();
}

void resetQueue() {
    if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi Disconnected!"); return; }
    HTTPClient http;
    String url = serverURL + "/reset";
    http.begin(url);
    int responseCode = http.POST("{}");
    Serial.print("Reset Response Code: "); Serial.println(responseCode);
    if (responseCode > 0) Serial.println(http.getString());
    http.end();
}

// ======================================================
// SETUP
// ======================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(buttonPin,   INPUT_PULLUP);
    pinMode(nextButton,  INPUT_PULLUP);
    pinMode(againButton, INPUT_PULLUP);
    pinMode(resetButton, INPUT_PULLUP);
    Serial.println();
    Serial.println("Starting Token Management System...");
    if (loadWiFi()) {
        Serial.println("Saved WiFi Found.");
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Saved WiFi not available. Starting Configuration Portal...");
            startAP();
        }
    } else {
        Serial.println("No WiFi Credentials Found.");
        startAP();
    }
}

// ======================================================
// LOOP
// ======================================================

void loop() {
    if (apMode) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        return;
    }

    webServer.handleClient();

    // ── GPIO 4 : Token button (short press) / WiFi config (hold 10 s) ──
    bool currentButtonState = digitalRead(buttonPin);

    if (currentButtonState == LOW && lastButtonState == HIGH) {
        // Button just pressed down — record start
        buttonPressStart = millis();
    }

    if (currentButtonState == HIGH && lastButtonState == LOW) {
        // Button just released — check hold duration
        unsigned long heldFor = millis() - buttonPressStart;

        if (heldFor >= 10000) {
            Serial.println("GPIO4 held 10 s → Clearing WiFi, entering config mode...");
            clearWiFi();
            delay(500);
            ESP.restart();
        } else if (heldFor > 300) {
            Serial.println("TOKEN BUTTON");
            generateToken();
        }
    }

    lastButtonState = currentButtonState;

    // ── GPIO 15 : Call Next (short press) / WiFi config (hold 10 s) ──
    if (digitalRead(nextButton) == LOW) {
        unsigned long nextPressStart = millis();

        while (digitalRead(nextButton) == LOW) {
            if (millis() - nextPressStart >= 10000) {
                Serial.println("GPIO15 held 10 s → Clearing WiFi, entering config mode...");
                clearWiFi();
                delay(500);
                ESP.restart();
            }
        }

        // Released before 10 s → normal Call Next
        if (millis() - nextPressStart < 10000) {
            Serial.println("NEXT PRESSED");
            callNext();
            delay(300);
        }
    }

    // Call Again
    if (digitalRead(againButton) == LOW) {
        Serial.println("CALL AGAIN PRESSED");
        callAgain();
        delay(300);
        while (digitalRead(againButton) == LOW);
    }

    // Reset (hold 5 seconds)
    if (digitalRead(resetButton) == LOW) {
        resetPressStart = millis();
        while (digitalRead(resetButton) == LOW) {
            if (millis() - resetPressStart >= 5000) {
                Serial.println("RESETTING QUEUE...");
                resetQueue();
                while (digitalRead(resetButton) == LOW);
                delay(300);
                break;
            }
        }
        if (millis() - resetPressStart < 5000) {
            Serial.println("Hold RESET for 5 seconds to reset queue.");
        }
    }

    // Reconnect if WiFi drops
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reconnecting...");
        connectWiFi();
    }
}
