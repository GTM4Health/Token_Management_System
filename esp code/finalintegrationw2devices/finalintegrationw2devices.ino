#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Preferences.h>

// WiFi Credentials (Loaded from Flash)

String ssid = "";
String password = "";


// Flask Server IP
String serverIP = "";

// Web server
WebServer webServer(80);

// Flash storage
Preferences preferences;
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>Token Management Setup</title>

<style>

body{
font-family:Arial;
margin:40px;
background:#f5f5f5;
}

.container{
background:white;
padding:20px;
border-radius:10px;
max-width:400px;
margin:auto;
box-shadow:0 0 10px gray;
}

input{
width:100%;
padding:10px;
margin-top:5px;
margin-bottom:15px;
}

button{
width:100%;
padding:12px;
background:#007bff;
color:white;
border:none;
font-size:18px;
border-radius:5px;
}

</style>

</head>

<body>

<div class="container">

<h2>Token Management WiFi Setup</h2>

<form action="/save">

SSID

<input name="ssid">

Password

<input type="password" name="password">

Server IP

<input name="server" value="">

<button type="submit">

Save

</button>

</form>

</div>

</body>

</html>

)rawliteral";

// ======================================================
// DEVICE TYPE
// ======================================================


const int buttonPin = 4;
const int nextButton = 15;      // D15
const int againButton = 13;     // D13
const int resetButton = 14;     // D14



bool lastButtonState = HIGH;
unsigned long lastPressTime = 0;
unsigned long resetPressStart = 0;
void saveWiFi()
{
    preferences.begin("wifi", false);

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    preferences.putString("server", serverIP);

    preferences.end();
}
bool loadWiFi()
{
    preferences.begin("wifi", true);

    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");

    serverIP = preferences.getString("server", "");

    preferences.end();

    if (ssid == "" || serverIP == "")
    return false;

return true;
}
void clearWiFi()
{
    preferences.begin("wifi", false);

    preferences.clear();

    preferences.end();

    Serial.println("Saved WiFi Cleared!");
}
void connectWiFi() {

  Serial.println();
  Serial.println("Connecting to saved WiFi...");

  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 15000) {

    delay(500);
    Serial.print(".");

  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("==============================");
    Serial.println("WiFi Connected!");
    Serial.print("ESP32 IP : ");
    Serial.println(WiFi.localIP());

    Serial.print("Connected SSID : ");
    Serial.println(WiFi.SSID());

    Serial.print("Server IP : ");
    Serial.println(serverIP);

    Serial.println("==============================");

  }

  else {

    Serial.println();
    Serial.println("Connection Failed.");

  }

}
void startAP() {

  WiFi.mode(WIFI_AP);

  WiFi.softAP("TMS_SETUP", "token123");

  Serial.println();
  Serial.println("==============================");
  Serial.println("Configuration Mode");
  Serial.println("SSID : TMS_SETUP");
  Serial.println("Password : token123");
  Serial.print("Open : http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("==============================");

  // Home Page
  webServer.on("/", []() {

    webServer.send(200, "text/html", webpage);

  });

  // Save WiFi Settings
  webServer.on("/save", []() {

    ssid = webServer.arg("ssid");
    password = webServer.arg("password");

    serverIP = webServer.arg("server");

    saveWiFi();

    webServer.send(200,
                   "text/html",
                   "<h2>Settings Saved!<br>Restarting ESP32...</h2>");

    delay(2000);

    ESP.restart();

  });

  webServer.begin();

}

void generateToken() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Disconnected!");
    return;

  }

  HTTPClient http;

String url = "http://" + serverIP + ":5000/generate";

Serial.print("Server IP = ");
Serial.println(serverIP);

Serial.print("URL = ");
Serial.println(url);

http.begin(url);

  http.addHeader("Content-Type", "application/json");

  int responseCode = http.POST("");

  Serial.print("HTTP Response Code: ");
  Serial.println(responseCode);

  if (responseCode == 200) {

    String payload = http.getString();

    Serial.println("Server Response:");
    Serial.println(payload);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {

      String token = doc["token"];

      Serial.println("----------------------------");
      Serial.print("Generated Token : ");
      Serial.println(token);
      Serial.println("----------------------------");

    } else {

      Serial.println("JSON Parse Failed");

    }

  } else {

    Serial.println("Failed to contact server.");

  }

  http.end();

}
void callNext() {

  Serial.println("CALL NEXT BUTTON");

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Disconnected!");
    return;

  }

  HTTPClient http;

  String url = "http://" + serverIP + ":5000/next";

  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);

  int responseCode = http.POST("");

  Serial.print("Response Code: ");
  Serial.println(responseCode);

  if (responseCode > 0) {

    Serial.println(http.getString());

  }

  http.end();

}

void callAgain() {

  Serial.println("CALL AGAIN BUTTON");

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Disconnected!");
    return;

  }

  HTTPClient http;

  String url = "http://" + serverIP + ":5000/call_again";

  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);

  int responseCode = http.POST("");

  Serial.print("Response Code: ");
  Serial.println(responseCode);

  if (responseCode > 0) {

    Serial.println(http.getString());

  }

  http.end();

}

void resetQueue() {

  Serial.println("RESET BUTTON");

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Disconnected!");
    return;

  }

  HTTPClient http;

  String url = "http://" + serverIP + ":5000/reset";

  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);

  int responseCode = http.POST("");

  Serial.print("Response Code: ");
  Serial.println(responseCode);

  if (responseCode > 0) {

    Serial.println(http.getString());

  }

  http.end();

}
void setup() {

  Serial.begin(115200);

  delay(1000);

  // Initialize ALL buttons
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(nextButton, INPUT_PULLUP);
  pinMode(againButton, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);

  Serial.println();
  Serial.println("Starting Token Management System...");

  if (loadWiFi()) {

    Serial.println("Saved WiFi Found.");

    connectWiFi();

    if (WiFi.status() != WL_CONNECTED) {

        Serial.println("Saved WiFi not available.");
        Serial.println("Starting Configuration Portal...");

        startAP();

    }

}
else {

    Serial.println("No WiFi Credentials Found.");

    startAP();

}
}

void loop() {

  webServer.handleClient();

  // ==========================
  // TOKEN BUTTON
  // ==========================
  bool currentButtonState = digitalRead(buttonPin);

  if (currentButtonState == LOW && lastButtonState == HIGH) {

    if (millis() - lastPressTime > 300) {

      lastPressTime = millis();

      Serial.println("TOKEN BUTTON");

      generateToken();

    }

  }

  lastButtonState = currentButtonState;

  // ==========================
  // CALL NEXT
  // ==========================
  if (digitalRead(nextButton) == LOW) {

    Serial.println("NEXT PRESSED");

    callNext();

    delay(300);

    while (digitalRead(nextButton) == LOW);

  }

  // ==========================
  // CALL AGAIN
  // ==========================
  if (digitalRead(againButton) == LOW) {

    Serial.println("CALL AGAIN PRESSED");

    callAgain();

    delay(300);

    while (digitalRead(againButton) == LOW);

  }

  // ==========================
  // RESET
  // ==========================
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

        Serial.println("Pressed for less than 5 seconds.");
        Serial.println("Hold RESET for 5 seconds to reset queue.");

    }
  }
}