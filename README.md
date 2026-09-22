# Token Management System (TMS)

A smart hospital token management system developed using **ESP32**, **Flask**, **SQLite**, and **Socket.IO** to automate patient queue management and improve hospital workflow.

The system allows patients to generate queue tokens using an ESP32-based token dispenser, while doctors and nurses can manage the queue through a web-based operator dashboard. A real-time public display keeps patients informed of the currently serving token.

---

# Features

## Token Generation
- Generate tokens using an ESP32 push button.
- Sequential token numbering (001, 002, 003...).
- Automatic storage in SQLite database.

## Live Public Display
Displays:
- Current Token
- Waiting Queue

Updates instantly using Socket.IO.

## Doctor Operator Dashboard
- Call Next Token
- Call Again
- Reset Queue (5-second safety hold)
- Manual Token Calling (Emergency/Missed Token)

## Manual Token Calling

Allows the operator to call any previously generated token without disturbing the existing queue order.

Example:

```
Queue:
004
005
006
007
008

Current:
003

Manual Call:
008

Next Call resumes from:
004
```

## Queue Reset Protection

The reset button must be held for **5 seconds** before the queue is cleared, preventing accidental resets.

## WiFi Configuration Portal

If WiFi credentials are unavailable, the ESP32 automatically creates an Access Point.

```
SSID : TMS_SETUP
Password : token123
```

Users can configure:
- WiFi SSID
- WiFi Password
- Server IP Address

Settings are permanently stored using ESP32 Preferences (Flash Memory).

## ESP32 Devices

### Token Generator
- Generate Token Button

### Doctor Interface
- Call Next
- Call Again
- Reset Queue (5-second hold)

Both devices communicate wirelessly with the Flask server over WiFi.

---

# Technologies Used

## Hardware
- ESP32 DevKit V1
- Push Buttons
- WiFi Network

## Software
- Python
- Flask
- Flask-SocketIO
- SQLite
- HTML
- CSS
- JavaScript
- Arduino IDE

---

# System Architecture

```
Patient
   │
   ▼
ESP32 Token Generator
   │
HTTP Request
   │
   ▼
Flask Server
   │
SQLite Database
   │
Socket.IO
   │
 ┌───────────────┬──────────────────┬────────────────────┐
 │               │                  │
 ▼               ▼                  ▼
Public Display  Operator Dashboard  ESP32 Doctor Interface
```

---

# Project Structure

```
TokenManagementSystem/
│
├── server.py
├── token.db
├── templates/
│   ├── kiosk.html
│   ├── display.html
│   ├── operator.html
│   └── print.html
│
├── static/
│   ├── logo.png
│   └── styles.css
│
├── Arduino/
│   └── finalintegrationw2devices.ino
│
└── README.md
```

---

# Installation

## Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/TokenManagementSystem.git
```

## Install Python Dependencies

```bash
pip install flask
pip install flask-socketio
pip install eventlet
```

## Run the Server

```bash
python server.py
```

The server will be available at:

```
http://localhost:5000
```

or

```
http://<YOUR_PC_IP>:5000
```

## Upload the ESP32 Firmware

Open the Arduino sketch and upload it to:
- ESP32 Token Generator
- ESP32 Doctor Interface

Both devices connect to the Flask server over WiFi.

---

# Operator Dashboard Functions

| Button | Function |
|---------|----------|
| Call Next | Calls the next waiting token |
| Call Again | Repeats the current serving token |
| Call Token | Manually calls any generated token |
| Reset Queue | Clears the queue after holding the reset button for 5 seconds |

---

# Future Improvements

- Thermal receipt printer
- Voice announcement system
- Multi-doctor queue support
- Department-wise queues
- QR code-based token generation
- SMS/WhatsApp notifications
- Cloud deployment
- Analytics dashboard

---

# License

This project is intended for educational and research purposes.
