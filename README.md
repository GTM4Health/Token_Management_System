# GTM4Health — Token Management System

<p align="center">
  <img src="static/logo.jpeg" alt="GTM4Health Logo" height="80"/>
</p>

<p align="center">
  A real-time, web-based queue management system built for healthcare facilities.<br/>
  Patients receive numbered tokens, a main display shows who is being served, and staff control the queue from a dedicated calling unit.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Python-3.9%2B-blue?style=flat-square&logo=python"/>
  <img src="https://img.shields.io/badge/Flask-2.x-lightgrey?style=flat-square&logo=flask"/>
  <img src="https://img.shields.io/badge/Socket.IO-4.x-black?style=flat-square&logo=socket.io"/>
  <img src="https://img.shields.io/badge/SQLite-embedded-blue?style=flat-square&logo=sqlite"/>
  <img src="https://img.shields.io/badge/Deployed%20on-Render-46E3B7?style=flat-square"/>
  <img src="https://img.shields.io/badge/Frontend%20on-Vercel-black?style=flat-square&logo=vercel"/>
</p>

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Features](#features)
4. [Project Structure](#project-structure)
5. [Pages & Interfaces](#pages--interfaces)
6. [API Reference](#api-reference)
7. [Real-Time Events (Socket.IO)](#real-time-events-socketio)
8. [Database Schema](#database-schema)
9. [Setup & Installation](#setup--installation)
10. [Running Locally](#running-locally)
11. [Deployment](#deployment)
12. [ESP32 Hardware Integration](#esp32-hardware-integration)
13. [3D Design Files](#3d-design-files)

---

## Overview

The **GTM4Health Token Management System (TMS)** is a full-stack, real-time queue management solution designed to streamline patient flow in clinics and healthcare facilities. It eliminates manual queuing by issuing sequential numbered tokens and broadcasting live updates to all connected screens simultaneously via WebSockets.

**How it works end-to-end:**

```
Patient arrives → Gets a token from the Kiosk → Waits in lobby watching the Main Display
                                                         ↓
                                              Staff calls Next Token from Calling Unit
                                                         ↓
                                       Main Display updates live (with voice announcement)
                                                         ↓
                                              Patient proceeds to room
```

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        CLIENT LAYER                             │
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │ Main Display │  │ Calling Unit │  │   Token Dispenser    │  │
│  │ (display.html│  │(operator.html│  │    (kiosk.html)      │  │
│  │   + TV/screen│  │   + tablet)  │  │ (tablet / ESP32)     │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────┬───────────┘  │
│         │                 │                      │              │
└─────────┼─────────────────┼──────────────────────┼─────────────┘
          │  Socket.IO      │  REST API            │  REST API
          ▼                 ▼                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                     SERVER LAYER (Flask)                        │
│               server.py  +  Flask-SocketIO  +  CORS            │
│                    Hosted on: Render.com                        │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   SQLite Database     │
                    │      token.db         │
                    │  (tokens table)       │
                    └───────────────────────┘
```

---

## Features

### Core Queue Management
- **Sequential Token Generation** — Tokens are issued as `001`, `002`, `003`, … (zero-padded, 3-digit format).
- **Call Next Token** — Operator calls the next waiting patient in FIFO order; the current token is automatically marked as `Completed`.
- **Call Again** — Re-broadcasts the currently serving token (useful if a patient didn't hear their number).
- **Call Specific Token** — Operator can skip to any token number by entering it manually.
- **Full Queue Reset** — Wipes all tokens and resets the counter back to `001`. Confirmation dialog prevents accidents.

### Real-Time Updates
- **Socket.IO WebSockets** — All screens (Main Display, Calling Unit, Kiosk) update instantly when the queue changes. No page refresh needed.
- **AJAX Polling Fallback** — Every 2 seconds, screens re-fetch the current token and waiting queue to stay in sync even if a Socket.IO event is missed.

### Main Display Extras
- **Text-to-Speech (TTS)** — When a token is called, the browser announces it aloud digit-by-digit (e.g., *"Token Zero, Zero, One. Please proceed to room."*) using the Web Speech API.
- **Live Waiting Queue** — A scrollable sidebar shows all tokens currently waiting, with a live count.
- **Reset Toast** — A non-intrusive notification banner appears when the queue is reset.
- **Responsive Design** — Works on full HD TVs, tablets, and mobile screens.

### Kiosk (Token Dispenser)
- Displays the latest generated token number prominently.
- Automatically opens a **print popup** (`/print/<token>`) when a new token is generated — ideal for thermal printers connected to the kiosk device.
- The print popup can be toggled on/off via `localStorage` (`printPopup` flag).
- Integrates with **ESP32** hardware for physical button-press token dispensing.

### Operator / Calling Unit
- Shows the **current serving token** prominently (190px font).
- **Manual Call** — Numeric input field to call any specific token by number.
- Buttons for: **Next**, **Call Again**, **Reset**.
- Live socket listener updates the displayed token automatically.

### Deployment & Integration
- Backend hosted on **Render.com** (Python web service).
- Frontend hosted on **Vercel** (`https://gtm-tms.vercel.app`).
- CORS and Socket.IO origins locked to the Vercel frontend for security.
- **Health check endpoint** at `/health` for uptime monitoring.

---

## Project Structure

```
Token_Management_System/
│
├── server.py                        # Flask backend — all API routes and Socket.IO events
├── requirements.txt                 # Python dependencies
├── token.db                         # SQLite database (auto-created on first run)
│
├── templates/
│   ├── display.html                 # Main Display — shown on TV/large screen in the lobby
│   ├── operator.html                # Calling Unit — used by clinic staff
│   ├── kiosk.html                   # Token Dispenser — patients generate their token here
│   └── print.html                   # Token Print Popup — thermal printer slip
│
├── static/
│   ├── logo.jpeg                    # GTM4Health organization logo
│   └── demologo.jpeg                # Demo/hospital partner logo (centered in header)
│
├── esp code/
│   └── finalintegrationw2devices/   # Arduino/ESP32 firmware for physical kiosk button
│
├── 3D design STL files for tms v1/  # STL files for Version 1 physical enclosure
├── 3D design STL files for tms v3/  # STL files for Version 3 physical enclosure
│
└── Documents and brochures/
    ├── TMS brochure v1.png
    ├── tech doc token management system.pdf
    └── tokenmanagementsystemreqdoc.pdf
```

---

## Pages & Interfaces

### 1. Main Display — `/` or `/display`

> **Intended for:** A large TV or monitor in the patient waiting area.

| Area | Description |
|------|-------------|
| **Left panel** | Shows the current token being served in large bold text (180px). Displays "Please proceed to the room" below it. |
| **Right panel** | Shows the live waiting queue — all tokens with `Waiting` status, with a count badge. |
| **Voice announcement** | When a token changes, the browser speaks it aloud using the Web Speech API. |
| **Apps menu** | Top-right hamburger menu (⊞) navigates between all three interfaces. |

---

### 2. Calling Unit (Operator Panel) — `/operator`

> **Intended for:** The clinic receptionist or staff member at the front desk.

| Control | Action |
|---------|--------|
| **Next Token** button | Marks current token as `Completed`, calls the next `Waiting` token as `Serving`. |
| **Call Again** button | Re-emits the `serving_changed` event for the currently serving token. |
| **Manual Token input + Call Token** | Enter any token number and call it directly, bypassing the queue order. |
| **Reset Queue** button | Deletes all tokens and resets the counter. Asks for confirmation first. |
| **Current Token display** | Shows the currently serving token in large text, auto-updates via Socket.IO. |

---

### 3. Token Dispenser (Kiosk) — `/kiosk`

> **Intended for:** A self-service tablet or ESP32-connected kiosk where patients arrive.

| Behaviour | Description |
|-----------|-------------|
| **Display on load** | Shows the most recently generated token in large text. |
| **On new token (Socket.IO)** | Updates displayed token and opens a print popup window (if enabled). |
| **Print popup** | Opens `/print/<token>` in a small popup — designed for thermal slip printing. |
| **Print toggle** | `localStorage.setItem("printPopup", "0")` disables the print popup. |

---

### 4. Print Slip — `/print/<token>`

> **Intended for:** Thermal printer connected to the kiosk.

A minimal, printable HTML page displaying the token number and branding. Automatically triggers `window.print()` on load.

---

## API Reference

All endpoints are served by the Flask backend at:  
`https://token-management-system-cvo8.onrender.com`

### GET Endpoints

| Endpoint | Description | Response |
|----------|-------------|----------|
| `GET /` | Renders Main Display page | HTML |
| `GET /display` | Alias for Main Display | HTML |
| `GET /operator` | Renders Calling Unit page | HTML |
| `GET /kiosk` | Renders Token Dispenser page | HTML |
| `GET /print/<token>` | Renders print slip for given token | HTML |
| `GET /current` | Returns the token currently being served | `{"token": "005"}` |
| `GET /latest` | Returns the last generated token number | `{"token": "007"}` |
| `GET /waiting` | Returns list of all waiting tokens (FIFO order) | `["003","004","005"]` |
| `GET /health` | Health check / uptime ping | `{"status": "online", "service": "..."}` |

### POST Endpoints

| Endpoint | Description | Request Body | Response |
|----------|-------------|--------------|----------|
| `POST /generate` | Generates the next sequential token and adds it to the queue with status `Waiting` | _(none)_ | `{"token": "008"}` |
| `POST /next` | Marks current `Serving` token as `Completed`, promotes oldest `Waiting` token to `Serving` | _(none)_ | `{"serving": "003"}` |
| `POST /call_again` | Re-emits the currently serving token to all clients | _(none)_ | `{"token": "003"}` |
| `POST /call_specific` | Calls a specific token number by jumping to it | `{"token": "005"}` | `{"message": "Called", "token": "005"}` |
| `POST /reset` | Deletes all tokens and resets the ID sequence | _(none)_ | `{"message": "Queue Reset Successfully", "token": "000"}` |

#### Error Responses

| Status | Meaning |
|--------|---------|
| `400` | Missing or invalid request body (e.g., token number not provided) |
| `404` | Token not found (used in `/call_specific`) |

---

## Real-Time Events (Socket.IO)

The server uses **Flask-SocketIO** with `eventlet` async mode. All clients connect to:  
`wss://token-management-system-cvo8.onrender.com`

### Events Emitted by Server → Clients

| Event | Payload | Triggered When |
|-------|---------|----------------|
| `new_token` | `{"token": "008"}` | A new token is generated via `POST /generate` |
| `serving_changed` | `{"token": "003"}` | A token is called via `/next`, `/call_again`, or `/call_specific` |
| `queue_reset` | _(no payload)_ | Queue is reset via `POST /reset` |

### How Each Page Responds

| Event | Main Display | Calling Unit | Kiosk |
|-------|-------------|-------------|-------|
| `new_token` | Refreshes waiting queue | _(not used)_ | Updates displayed token + opens print popup |
| `serving_changed` | Updates serving token, refreshes queue, triggers TTS | Updates current token display | _(not used)_ |
| `queue_reset` | Clears display, shows "---", shows toast | Clears current token | Resets to "000" |

---

## Database Schema

The system uses a single **SQLite** database (`token.db`) with one table:

```sql
CREATE TABLE IF NOT EXISTS tokens (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    token          TEXT NOT NULL,          -- Zero-padded 3-digit number e.g. "007"
    status         TEXT NOT NULL,          -- "Waiting" | "Serving" | "Completed"
    generated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Token Lifecycle

```
[Generated]  →  status = "Waiting"
     ↓
[Called]     →  status = "Serving"   (only one token can be Serving at a time)
     ↓
[Next called] → status = "Completed"
```

> **Note:** On queue reset, all rows are deleted and `sqlite_sequence` is cleared so token numbering restarts from `001`.

---

## Setup & Installation

### Prerequisites

- Python **3.9+**
- `pip` package manager
- A modern web browser (Chrome recommended for best TTS support)

### Step 1 — Clone the Repository

```bash
git clone https://github.com/your-org/Token_Management_System.git
cd Token_Management_System
```

### Step 2 — Create a Virtual Environment (Recommended)

```bash
# Windows
python -m venv venv
venv\Scripts\activate

# macOS / Linux
python3 -m venv venv
source venv/bin/activate
```

### Step 3 — Install Dependencies

```bash
pip install -r requirements.txt
```

**Dependencies installed:**

| Package | Purpose |
|---------|---------|
| `Flask` | Web framework and routing |
| `Flask-SocketIO` | WebSocket support via Socket.IO |
| `Flask-CORS` | Cross-Origin Resource Sharing headers |
| `eventlet` | Async networking for Socket.IO |

---

## Running Locally

### Start the Server

```bash
python server.py
```

The server starts on `http://0.0.0.0:5000` by default. Open your browser and navigate to:

| Interface | URL |
|-----------|-----|
| Main Display | http://localhost:5000/ |
| Calling Unit | http://localhost:5000/operator |
| Token Dispenser | http://localhost:5000/kiosk |
| Health Check | http://localhost:5000/health |

> **Note:** When running locally, the HTML templates hard-code the API URL to the **Render.com** hosted backend (`https://token-management-system-cvo8.onrender.com`). To test fully locally, update the `API_URL` constant in each HTML template from the Render URL to `http://localhost:5000`.

### Quick Local API Test (using curl or Postman)

```bash
# Generate a new token
curl -X POST http://localhost:5000/generate

# Get the current serving token
curl http://localhost:5000/current

# Get all waiting tokens
curl http://localhost:5000/waiting

# Call the next token
curl -X POST http://localhost:5000/next

# Call a specific token
curl -X POST http://localhost:5000/call_specific \
     -H "Content-Type: application/json" \
     -d '{"token": "003"}'

# Reset the queue
curl -X POST http://localhost:5000/reset
```

---

## Deployment

The project uses a **split deployment** architecture:

| Component | Platform | URL |
|-----------|----------|-----|
| Backend (Flask + Socket.IO) | [Render.com](https://render.com) | `https://token-management-system-cvo8.onrender.com` |
| Frontend (HTML served by Flask) | [Vercel](https://vercel.com) | `https://gtm-tms.vercel.app` |

### Deploying the Backend to Render

1. Push your repository to GitHub.
2. Create a new **Web Service** on Render.
3. Set the following:
   - **Build Command:** `pip install -r requirements.txt`
   - **Start Command:** `python server.py`
   - **Environment:** Python 3
4. Set the `PORT` environment variable if needed (Render auto-injects this).
5. CORS and Socket.IO origins in `server.py` are already scoped to `https://gtm-tms.vercel.app`.

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PORT` | `5000` | Port the Flask server listens on. Render sets this automatically. |

> **Important:** The SQLite database (`token.db`) is stored on the server filesystem. On Render's free tier, the disk may reset on redeploy. For persistent storage across deployments, consider upgrading to a Render Disk or migrating to PostgreSQL.

---

## ESP32 Hardware Integration

The `esp code/finalintegrationw2devices/` folder contains **Arduino firmware** for a physical token dispensing kiosk built with an **ESP32** microcontroller.

### Hardware Overview

The ESP32 kiosk:
- Connects to Wi-Fi.
- Sends a `POST /generate` request to the backend when the patient presses a physical button.
- Can optionally drive a small thermal printer via serial to print the token slip.
- Works alongside the web-based kiosk interface (`kiosk.html`).

### Setup

1. Open the firmware in the Arduino IDE.
2. Update the Wi-Fi credentials (`ssid`, `password`) and the backend URL in the sketch.
3. Flash the firmware to your ESP32 board.
4. The device will connect to Wi-Fi and begin listening for button presses.

---

## 3D Design Files

Physical enclosure designs are provided as STL files for 3D printing:

| Folder | Version | Contents |
|--------|---------|---------|
| `3D design STL files for tms v1/` | v1 | First iteration of the physical kiosk enclosure |
| `3D design STL files for tms v3/` | v3 | Refined enclosure with improved fitment for ESP32 and printer |

STL files can be opened and sliced in tools like **Cura**, **PrusaSlicer**, or **Bambu Studio**.

---

## Project Roles

| Interface | User / Actor | Device |
|-----------|-------------|--------|
| **Main Display** | Patients in waiting area | TV / Large Monitor |
| **Token Dispenser (Kiosk)** | Arriving patients | Tablet / ESP32 kiosk |
| **Calling Unit (Operator)** | Clinic staff / receptionist | Tablet / Desktop |

---

## License

This project was developed by **GTM4Health**. All rights reserved.

---

<p align="center">© GTM4Health 2026</p>
