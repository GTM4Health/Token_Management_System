# GTM4Health Token Management System
## Bill of Materials (BOM)

**Document No.:** TMS-BOM-001  
**Version:** 1.0  
**Date:** October 2026  
**Prepared By:** GTM4Health Development Team  
**Project:** Token Management System (TMS)

---

## About This Document

A **Bill of Materials (BOM)** is a comprehensive, structured list of all the raw materials, components, subassemblies, parts, and quantities required to build, deploy, and operate the GTM4Health Token Management System. This BOM covers all software libraries, hardware components, firmware dependencies, cloud services, design assets, and documentation that make up the complete system.

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Software — Backend](#2-software--backend)
3. [Software — Frontend](#3-software--frontend)
4. [Firmware — ESP32 Kiosk Unit](#4-firmware--esp32-kiosk-unit)
5. [Hardware — Physical Kiosk Unit](#5-hardware--physical-kiosk-unit)
6. [Cloud & Hosting Services](#6-cloud--hosting-services)
7. [3D Design & Enclosure Assets](#7-3d-design--enclosure-assets)
8. [Documentation & Brochures](#8-documentation--brochures)
9. [BOM Summary Table](#9-bom-summary-table)

---

## 1. System Overview

The GTM4Health Token Management System is a digital queue management solution for healthcare facilities. It consists of:

- A **Python/Flask backend** deployed on Render.com
- A **Web-based frontend** (HTML/JS/CSS) served via Flask templates
- An **ESP32-based physical kiosk** with push buttons for generating and managing tokens
- A **3D-printed enclosure** for the physical kiosk unit
- **Cloud hosting** via Render.com (backend) and Vercel (frontend domain)

---

## 2. Software — Backend

All Python packages are declared in `requirements.txt`.

| # | Component | Type | Version | License | Purpose |
|---|-----------|------|---------|---------|---------|
| 1 | Python | Runtime | 3.9+ | PSF | Core programming language for the server |
| 2 | Flask | Python Package | 2.x | BSD-3-Clause | Web application framework; handles HTTP routes |
| 3 | Flask-SocketIO | Python Package | 5.x | MIT | WebSocket layer for real-time events |
| 4 | Flask-CORS | Python Package | 4.x | MIT | Cross-Origin Resource Sharing policy management |
| 5 | eventlet | Python Package | 0.36+ | MIT | Async I/O engine for Socket.IO / WebSockets |
| 6 | SQLite | Built-in (Python stdlib) | 3.x | Public Domain | Embedded relational database; stores token records |
| 7 | ArduinoJson | Arduino Library | 7.x | MIT | JSON parsing on the ESP32 for API responses |

### Backend Source Files

| File | Description |
|------|-------------|
| `server.py` | Main backend — all Flask routes, Socket.IO events, and database logic |
| `requirements.txt` | Python dependency declarations for Render deployment |
| `token.db` | SQLite database file (auto-created on first server start) |

---

## 3. Software — Frontend

The frontend is built with plain HTML, CSS, and JavaScript — no build tools or package manager required.

| # | Component | Type | Version | License | Purpose |
|---|-----------|------|---------|---------|---------|
| 1 | HTML5 | Web Standard | — | — | Structure for all interface pages |
| 2 | CSS3 | Web Standard | — | — | Styling and layout of all pages |
| 3 | JavaScript (ES6+) | Web Standard | — | — | Client-side logic, API calls, Socket.IO client |
| 4 | Socket.IO Client | JS Library (CDN) | 4.x | MIT | Real-time WebSocket communication with backend |
| 5 | Web Speech API | Browser API | — | W3C Standard | Text-to-speech voice announcements on Main Display |

### Frontend Source Files (Templates)

| File | Interface | Description |
|------|-----------|-------------|
| `templates/display.html` | Main Display | Lobby TV screen — shows current token + waiting queue, plays voice announcements |
| `templates/operator.html` | Calling Unit | Receptionist control panel — Next, Call Again, Call Specific, Reset |
| `templates/kiosk.html` | Token Dispenser | Patient-facing kiosk screen — shows latest token, triggers print popup |
| `templates/print.html` | Print Slip | Auto-print popup with token number, date, and branding |

### Static Assets

| File | Description |
|------|-------------|
| `static/logo.jpeg` | GTM4Health official logo used in all page headers |
| `static/demologo.jpeg` | Demo/secondary logo |

---

## 4. Firmware — ESP32 Kiosk Unit

The physical kiosk runs on an ESP32 microcontroller programmed via Arduino IDE.

### Arduino Libraries Required

| # | Library | Version | License | Purpose |
|---|---------|---------|---------|---------|
| 1 | WiFi (Arduino ESP32 core) | Built-in | LGPL | Wi-Fi connectivity for the ESP32 |
| 2 | HTTPClient (Arduino ESP32 core) | Built-in | LGPL | HTTP POST requests to the Flask API |
| 3 | ArduinoJson | 7.x | MIT | Parsing JSON responses from the server |
| 4 | WebServer (Arduino ESP32 core) | Built-in | LGPL | Captive portal web server in AP mode |
| 5 | DNSServer (Arduino ESP32 core) | Built-in | LGPL | DNS interception for captive portal setup flow |
| 6 | Preferences (Arduino ESP32 core) | Built-in | LGPL | Flash NVM storage for Wi-Fi credentials |

### Firmware Source Files

| File | Description |
|------|-------------|
| `esp code/finalintegrationw2devices/finalintegrationw2devices.ino` | Main ESP32 firmware — Wi-Fi setup, captive portal, button GPIO handling, API calls |

### GPIO Pin Assignments

| GPIO Pin | Function | Button Action |
|----------|----------|---------------|
| GPIO 4 | Token Button | Short press → Generate new token; Hold 10 s → Clear Wi-Fi & enter config mode |
| GPIO 15 | Next Button | Short press → Call next token; Hold 10 s → Clear Wi-Fi & enter config mode |
| GPIO 13 | Call Again Button | Short press → Re-announce current token |
| GPIO 14 | Reset Button | Hold 5 s → Reset entire queue |

---

## 5. Hardware — Physical Kiosk Unit

The following hardware components are required to assemble the physical ESP32 kiosk unit.

| # | Component | Specification | Qty | Notes |
|---|-----------|--------------|-----|-------|
| 1 | ESP32 Development Board | ESP32-WROOM-32 / 38-pin dev board | 1 | Main microcontroller; handles Wi-Fi and button logic |
| 2 | Push Button (Token Dispense) | Momentary tactile push button | 1 | Connected to GPIO 4; patients press this to get a token |
| 3 | Push Button (Next Token) | Momentary tactile push button | 1 | Connected to GPIO 15; staff calls next patient |
| 4 | Push Button (Call Again) | Momentary tactile push button | 1 | Connected to GPIO 13; re-announces current token |
| 5 | Push Button (Reset Queue) | Momentary tactile push button | 1 | Connected to GPIO 14; hold 5 s to reset queue |
| 6 | USB Micro-B or USB-C Cable | Matching ESP32 dev board connector | 1 | Power supply and firmware flashing |
| 7 | USB Power Adapter (5V) | 5V / 1A minimum | 1 | Powers the ESP32 unit |
| 8 | Jumper Wires | Male–Male / Male–Female | As needed | Connecting buttons to ESP32 GPIO pins |
| 9 | Breadboard or PCB | Standard 830-tie breadboard or custom PCB | 1 | Circuit assembly base |
| 10 | 3D-Printed Enclosure | See Section 7 | 1 | Houses the ESP32 and buttons |
| 11 | Thermal Receipt Printer (Optional) | ESC/POS compatible, USB or Serial | 1 | Prints token slips; connected to the kiosk tablet/device |

> **Note:** All buttons are wired with internal pull-up resistors (`INPUT_PULLUP` in firmware) — no external resistors are required.

---

## 6. Cloud & Hosting Services

| # | Service | Plan | Purpose | URL |
|---|---------|------|---------|-----|
| 1 | Render.com | Free Tier (Web Service) | Backend hosting — runs `server.py` (Flask + Socket.IO) | https://render.com |
| 2 | Vercel | Hobby (Free) | Frontend domain — serves `gtm-tms.vercel.app` | https://vercel.com |
| 3 | GitHub | Free | Source code version control and repository hosting | https://github.com |
| 4 | UptimeRobot *(Optional)* | Free | Monitors `/health` endpoint; keeps Render free-tier server awake | https://uptimerobot.com |

### Live Deployment URLs

| Resource | URL |
|----------|-----|
| Backend API | https://token-management-system-cvo8.onrender.com |
| Frontend (Main Display) | https://gtm-tms.vercel.app/ |
| Token Dispenser (Kiosk) | https://gtm-tms.vercel.app/kiosk |
| Calling Unit (Operator) | https://gtm-tms.vercel.app/operator |
| Health Check | https://token-management-system-cvo8.onrender.com/health |

---

## 7. 3D Design & Enclosure Assets

3D-printable enclosure designs are provided in STL format for two kiosk versions.

### Version 1 — Dual-Unit Enclosure

| # | File | Description | Size |
|---|------|-------------|------|
| 1 | `3D design STL files for tms v1/unit-1curved edges.stl` | Kiosk Unit 1 enclosure — curved edge design | ~225 KB |
| 2 | `3D design STL files for tms v1/Unit-2.stl` | Kiosk Unit 2 enclosure | ~254 KB |

### Version 3 — Single Unit (Two Shape Variants)

| # | File | Description | Size |
|---|------|-------------|------|
| 3 | `3D design STL files for tms v3/circular design tms v3.stl` | Circular-profile kiosk enclosure (v3) | ~68 KB |
| 4 | `3D design STL files for tms v3/triangular desgin tms v3.stl` | Triangular-profile kiosk enclosure (v3) | ~41 KB |

### Recommended 3D Print Settings

| Parameter | Recommended Value |
|-----------|-------------------|
| Material | PLA or PETG |
| Layer Height | 0.2 mm |
| Infill | 20–30% |
| Supports | As required per orientation |
| Printer Type | Any FDM printer with ≥ 200 × 200 mm build volume |

---

## 8. Documentation & Brochures

All documents are stored in the `Documents and brochures/` folder.

| # | Document | File | Description |
|---|----------|------|-------------|
| 1 | Project README | `README.md` | Top-level project overview, setup, and deployment summary |
| 2 | Frontend User Guide | `FRONTEND_GUIDE.md` | Step-by-step guide for staff using all three web interfaces |
| 3 | Backend Administration Guide | `BACKEND_GUIDE.md` | Technical guide for developers — API, database, deployment |
| 4 | Bill of Materials | `BILL_OF_MATERIALS.md` | This document — full component list for the system |
| 5 | Technical Documentation | `tech doc token management system.pdf` | Detailed technical reference document |
| 6 | Requirements Document | `tokenmanagementsystemreqdoc.pdf` | System requirements and specifications |
| 7 | System Brochure | `TMS brochure v1.png` | Visual brochure/marketing material for the TMS |

---

## 9. BOM Summary Table

A consolidated count of all items across all categories.

| Category | Total Items |
|----------|------------|
| Backend Software Components | 7 |
| Backend Source Files | 3 |
| Frontend Software Components | 5 |
| Frontend Source Files (Templates) | 4 |
| Frontend Static Assets | 2 |
| Firmware Arduino Libraries | 6 |
| Firmware Source Files | 1 |
| Hardware Components | 11 |
| Cloud / Hosting Services | 4 |
| 3D Design STL Files | 4 |
| Documentation Files | 7 |
| **TOTAL ITEMS** | **54** |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | October 2026 | GTM4Health Dev Team | Initial BOM created |

---

*For technical support or questions about this document, refer to the **Backend Administration Guide** (`BACKEND_GUIDE.md`) or the **Frontend User Guide** (`FRONTEND_GUIDE.md`).*

---

**© GTM4Health 2026 — Internal Documentation**
