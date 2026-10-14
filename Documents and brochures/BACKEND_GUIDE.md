# GTM4Health Token Management System
## Backend Administration Guide

**Version:** 1.0 | **Last Updated:** October 2026  
**Backend URL:** [https://token-management-system-cvo8.onrender.com](https://token-management-system-cvo8.onrender.com)  
**Frontend URL:** [https://gtm-tms.vercel.app](https://gtm-tms.vercel.app)

---

## Who This Document Is For

This guide is for **developers, system administrators, or any technical staff** responsible for:

- Maintaining the server that powers the Token Management System
- Making code changes and redeploying
- Managing the database
- Understanding how the API and real-time communication work
- Troubleshooting backend-level issues

A basic understanding of Python, command-line tools, and Git is helpful but each section explains the steps in full.

---

## Table of Contents

1. [Architecture Summary](#1-architecture-summary)
2. [Tech Stack](#2-tech-stack)
3. [Repository Structure](#3-repository-structure)
4. [Local Development Setup](#4-local-development-setup)
5. [Running the Server Locally](#5-running-the-server-locally)
6. [Complete API Reference](#6-complete-api-reference)
7. [Real-Time Events (Socket.IO)](#7-real-time-events-socketio)
8. [Database — Schema & Management](#8-database--schema--management)
9. [Render.com — Backend Deployment](#9-rendercom--backend-deployment)
10. [Vercel — Frontend Deployment](#10-vercel--frontend-deployment)
11. [Environment Variables](#11-environment-variables)
12. [CORS & Security Configuration](#12-cors--security-configuration)
13. [Making Code Changes & Redeploying](#13-making-code-changes--redeploying)
14. [Monitoring & Health Checks](#14-monitoring--health-checks)
15. [Common Backend Issues & Fixes](#15-common-backend-issues--fixes)
16. [Important Limitations to Know](#16-important-limitations-to-know)

---

## 1. Architecture Summary

The system uses a **split deployment** pattern:

```
┌────────────────────────────────────────────────────────────────────┐
│                          USER DEVICES                              │
│                                                                    │
│  Main Display (TV)     Calling Unit (tablet)    Kiosk (tablet)     │
│        │                       │                      │            │
└────────┼───────────────────────┼──────────────────────┼────────────┘
         │                       │                      │
         │    HTTPS + WebSocket (Socket.IO)             │
         ▼                       ▼                      ▼
┌────────────────────────────────────────────────────────────────────┐
│                     VERCEL  (Frontend Host)                        │
│             https://gtm-tms.vercel.app                             │
│  Serves: display.html, operator.html, kiosk.html, print.html       │
│  (Static HTML files delivered via Flask templates on Render)       │
└───────────────────────────────┬────────────────────────────────────┘
                                │
                     REST API + Socket.IO
                                │
                                ▼
┌────────────────────────────────────────────────────────────────────┐
│                  RENDER.COM  (Backend Host)                        │
│       https://token-management-system-cvo8.onrender.com            │
│                                                                    │
│   Flask App (server.py)                                            │
│      ├── Flask-SocketIO (WebSocket real-time events)               │
│      ├── Flask-CORS (origin control)                               │
│      └── eventlet (async WebSocket engine)                         │
│                                                                    │
│   SQLite Database (token.db)                                       │
│      └── tokens table (id, token, status, generated_time)          │
└────────────────────────────────────────────────────────────────────┘
```

> **Key point:** The HTML pages are served by Flask from the `templates/` folder. The JavaScript inside each page connects back to the Render backend for all data and real-time events. Vercel simply delivers the initial HTML.

---

## 2. Tech Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| Web Framework | Flask | 2.x |
| Real-time Layer | Flask-SocketIO + Socket.IO (client) | 4.x |
| Cross-Origin | Flask-CORS | 4.x |
| Async Engine | eventlet | 0.36+ |
| Database | SQLite (built into Python) | 3.x |
| Backend Host | Render.com (Python web service) | — |
| Frontend Host | Vercel | — |

### Python Dependencies (`requirements.txt`)

```
Flask
Flask-SocketIO
Flask-CORS
eventlet
```

---

## 3. Repository Structure

```
Token_Management_System/
│
├── server.py                    ← MAIN BACKEND — all routes, socket events, DB logic
├── requirements.txt             ← Python package dependencies
├── token.db                     ← SQLite database (auto-created on startup)
│
├── templates/                   ← HTML pages served by Flask
│   ├── display.html             ← Main Display (lobby TV)
│   ├── operator.html            ← Calling Unit (staff page)
│   ├── kiosk.html               ← Token Dispenser (patient kiosk)
│   └── print.html               ← Thermal print slip popup
│
├── static/                      ← Static assets (images)
│   ├── logo.jpeg
│   └── demologo.jpeg
│
├── esp code/                    ← Arduino/ESP32 firmware for physical button kiosk
├── 3D design STL files for tms v1/
├── 3D design STL files for tms v3/
└── Documents and brochures/
```

---

## 4. Local Development Setup

### Prerequisites

- Python 3.9 or higher
- pip
- Git

### Step 1 — Clone the Repository

```bash
git clone https://github.com/your-org/Token_Management_System.git
cd Token_Management_System
```

### Step 2 — Create a Virtual Environment

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

### Step 4 — (Optional) Point HTML to Localhost

By default, all HTML templates contain:
```javascript
const API_URL = "https://token-management-system-cvo8.onrender.com";
```

For fully local development, change this to:
```javascript
const API_URL = "http://localhost:5000";
```

Do this in all three template files: `display.html`, `operator.html`, `kiosk.html`.  
**Remember to revert this before pushing to production.**

---

## 5. Running the Server Locally

```bash
python server.py
```

Expected output:
```
 * Running on http://0.0.0.0:5000
 * Running on http://127.0.0.1:5000
```

Open in browser:

| Page | URL |
|------|-----|
| Main Display | http://localhost:5000/ |
| Calling Unit | http://localhost:5000/operator |
| Kiosk | http://localhost:5000/kiosk |
| Health Check | http://localhost:5000/health |

The SQLite database (`token.db`) is created automatically on first run if it doesn't exist.

---

## 6. Complete API Reference

**Base URL (Production):** `https://token-management-system-cvo8.onrender.com`

---

### GET /health

Health check for uptime monitoring.

**Response:**
```json
{
  "status": "online",
  "service": "GTM4Health Token Management System"
}
```

---

### GET /current

Returns the token currently being served.

**Response (token active):**
```json
{ "token": "005" }
```
**Response (no active token):**
```json
{ "token": "---" }
```

---

### GET /latest

Returns the most recently generated token (regardless of status).

**Response:**
```json
{ "token": "008" }
```
**Response (no tokens yet):**
```json
{ "token": "000" }
```

---

### GET /waiting

Returns an ordered list of all tokens with `Waiting` status (oldest first).

**Response:**
```json
["003", "004", "005"]
```
**Response (empty queue):**
```json
[]
```

---

### POST /generate

Generates the next sequential token. Increments the last token number by 1 and inserts it with status `Waiting`. Emits `new_token` Socket.IO event to all clients.

**Request body:** None required

**Response:**
```json
{ "token": "009" }
```

**Behaviour:**
- Reads the last token in the database
- Increments by 1 (or starts at 1 if database is empty)
- Zero-pads to 3 digits (`001`, `002`, ..., `999`)
- Inserts into `tokens` table with `status = 'Waiting'`
- Broadcasts `new_token` event

---

### POST /next

Advances the queue by one. Marks the current `Serving` token as `Completed` and promotes the oldest `Waiting` token to `Serving`. Emits `serving_changed` event.

**Request body:** None required

**Response (success):**
```json
{ "serving": "004" }
```
**Response (no waiting tokens):**
```json
{ "message": "No waiting tokens", "serving": "---" }
```

---

### POST /call_again

Re-emits the currently serving token to all connected clients without changing any database state. Emits `serving_changed` event.

**Request body:** None required

**Response (success):**
```json
{ "token": "004" }
```
**Response (no token serving):**
```json
{ "message": "No token serving", "token": "---" }
```

---

### POST /call_specific

Calls a specific token by number. Marks the current `Serving` token as `Completed` and sets the specified token to `Serving`. Emits `serving_changed` event.

**Request body:**
```json
{ "token": "007" }
```

**Response (success):**
```json
{ "message": "Called", "token": "007" }
```

**Error responses:**

| HTTP Status | Response | Reason |
|-------------|----------|--------|
| `400` | `{"message": "Token is required"}` | No body or missing `token` field |
| `404` | `{"message": "Invalid Token"}` | Token number not found in DB |

---

### POST /reset

Deletes all tokens from the database and resets the auto-increment counter so the next token starts at `001`. Emits `queue_reset` event to all clients.

**Request body:** None required

**Response:**
```json
{
  "message": "Queue Reset Successfully",
  "token": "000"
}
```

---

## 7. Real-Time Events (Socket.IO)

The server uses **Flask-SocketIO** with `eventlet` as the async engine. All browser clients connect to the Socket.IO server at the Render backend URL.

### Server → Client Events

| Event Name | Payload | Summary |
|------------|---------|---------|
| `new_token` | `{"token": "008"}` | Fired when `POST /generate` creates a new token |
| `serving_changed` | `{"token": "004"}` | Fired when any token becomes the serving token |
| `queue_reset` | _(no payload)_ | Fired when `POST /reset` is called |

### How Each Client Reacts

| Event | Main Display (`display.html`) | Calling Unit (`operator.html`) | Kiosk (`kiosk.html`) |
|-------|------------------------------|-------------------------------|----------------------|
| `new_token` | Refreshes waiting queue | _(no listener)_ | Updates token display + opens print popup |
| `serving_changed` | Updates serving number, refreshes queue, triggers TTS | Updates current token display | _(no listener)_ |
| `queue_reset` | Clears display → "---", clears queue, shows toast | Clears display → "---" | Resets token → "000" |

### Socket.IO Configuration in `server.py`

```python
socketio = SocketIO(
    app,
    cors_allowed_origins=["https://gtm-tms.vercel.app"],
    async_mode="eventlet"
)
```

> To allow local development, temporarily add `"http://localhost:5000"` to `cors_allowed_origins`.

---

## 8. Database — Schema & Management

### Database File

- **Location:** Same directory as `server.py` at `./token.db`
- **Type:** SQLite (no external database server needed)
- **Auto-created:** `init_db()` is called on server start; if `token.db` doesn't exist, it is created automatically.

### Schema

```sql
CREATE TABLE IF NOT EXISTS tokens (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    token          TEXT NOT NULL,
    status         TEXT NOT NULL,
    generated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

| Column | Type | Description |
|--------|------|-------------|
| `id` | INTEGER | Auto-increment primary key |
| `token` | TEXT | Zero-padded 3-digit string, e.g. `"007"` |
| `status` | TEXT | One of: `"Waiting"`, `"Serving"`, `"Completed"` |
| `generated_time` | TIMESTAMP | UTC timestamp when the token was created |

### Token Status Lifecycle

```
INSERT → status = "Waiting"
           │
           ▼  (POST /next or /call_specific)
        status = "Serving"     ← only ONE row can have this status at any time
           │
           ▼  (next POST /next or /call_specific)
        status = "Completed"
```

### Viewing the Database (Developer)

You can inspect `token.db` using the SQLite command-line tool:

```bash
sqlite3 token.db
```

Useful queries:

```sql
-- View all tokens
SELECT * FROM tokens ORDER BY id DESC;

-- Count by status
SELECT status, COUNT(*) FROM tokens GROUP BY status;

-- See current serving token
SELECT * FROM tokens WHERE status = 'Serving';

-- Manually reset (same as /reset endpoint)
DELETE FROM tokens;
DELETE FROM sqlite_sequence WHERE name = 'tokens';
```

Type `.quit` to exit.

### Backing Up the Database

Simply copy the `token.db` file:

```bash
cp token.db token_backup_2026_10_13.db
```

---

## 9. Render.com — Backend Deployment

### Current Deployment

| Property | Value |
|----------|-------|
| Platform | Render.com |
| Service Type | Web Service (Python) |
| Live URL | `https://token-management-system-cvo8.onrender.com` |
| Start Command | `python server.py` |
| Region | (check your Render dashboard) |

### How to Access the Render Dashboard

1. Go to [https://render.com](https://render.com) and log in.
2. Click on your service: **token-management-system**.
3. From here you can:
   - View **live logs** (Logs tab)
   - Trigger a **manual redeploy** (Manual Deploy button)
   - View/edit **environment variables** (Environment tab)
   - Check **uptime and metrics**

### Understanding the Free Tier Sleep Behaviour

> ⚠️ **Important for all staff to know:**

On Render's **free tier**, web services **go to sleep** after **15 minutes of inactivity** (no HTTP requests). When a user visits the site after this period:

- The server takes **30–60 seconds to wake up**
- During this time, the browser may show a blank page or timeout
- **Solution:** Simply wait and refresh the page — it will load once the server wakes

To avoid this, you can:
- Upgrade to a **Render paid plan** (keeps service always-on)
- Use an uptime monitoring tool (e.g., [UptimeRobot](https://uptimerobot.com)) to ping `/health` every 10 minutes and keep the server awake

### Viewing Live Logs on Render

1. Go to your Render dashboard → click the service → click **Logs**
2. Live logs show every HTTP request, Socket.IO connection, and any errors in real time
3. Useful for debugging deployment issues or checking if API calls are reaching the server

### Manual Redeploy on Render

If you've pushed code to GitHub and want to redeploy:
1. Open the Render dashboard → your service
2. Click **Manual Deploy** → **Deploy latest commit**
3. Watch the logs — a successful deploy ends with the server starting message

Alternatively, enable **Auto-Deploy** in Render settings linked to your GitHub repo branch (usually `main`), so every `git push` triggers a redeploy automatically.

---

## 10. Vercel — Frontend Deployment

### Current Deployment

| Property | Value |
|----------|-------|
| Platform | Vercel |
| Live URL | `https://gtm-tms.vercel.app` |
| Framework | Other (static/Flask-served HTML) |

### How to Access the Vercel Dashboard

1. Go to [https://vercel.com](https://vercel.com) and log in.
2. Click on your project: **gtm-tms**
3. From here you can:
   - View **deployment history**
   - Check **build logs**
   - Trigger **redeployments**
   - Manage **custom domains**

### Important Note About This Deployment

The HTML files (`display.html`, `operator.html`, `kiosk.html`) are served by the **Flask backend on Render**, not directly by Vercel. Vercel in this project acts as a **domain/CDN layer** that proxies or redirects to the Render backend. The actual page logic and API calls all go to `token-management-system-cvo8.onrender.com`.

This means:
- **Deploying frontend changes** still requires pushing to the repo and redeploying on Render (the HTML templates are part of the Flask app)
- Vercel primarily provides the custom domain `gtm-tms.vercel.app`

---

## 11. Environment Variables

The server reads one environment variable:

| Variable | Default | Where Set | Description |
|----------|---------|-----------|-------------|
| `PORT` | `5000` | Render auto-injects this | The port Flask listens on |

In `server.py`:
```python
socketio.run(
    app,
    host="0.0.0.0",
    port=int(os.environ.get("PORT", 5000)),
    debug=False
)
```

To add or change environment variables on Render:
1. Render Dashboard → your service → **Environment** tab
2. Add key-value pairs
3. Redeploy for changes to take effect

---

## 12. CORS & Security Configuration

The backend restricts cross-origin access to only the Vercel frontend domain.

### Flask-CORS Configuration

```python
CORS(
    app,
    resources={
        r"/*": {
            "origins": [
                "https://gtm-tms.vercel.app"
            ]
        }
    }
)
```

### Socket.IO CORS Configuration

```python
socketio = SocketIO(
    app,
    cors_allowed_origins=["https://gtm-tms.vercel.app"],
    async_mode="eventlet"
)
```

### What This Means

- Only requests originating from `https://gtm-tms.vercel.app` are permitted by CORS policy
- Direct browser requests from any other domain will be blocked
- API calls using curl or Postman from a server are **not** blocked by CORS (CORS is a browser-side restriction)

### Adding a New Allowed Origin (e.g., for testing)

In `server.py`, add the new origin to both lists:

```python
"origins": [
    "https://gtm-tms.vercel.app",
    "http://localhost:5000"      # ← add for local testing
]
```

---

## 13. Making Code Changes & Redeploying

### Workflow for Code Changes

```
1. Make changes locally
2. Test locally (python server.py)
3. Commit and push to GitHub
4. Render auto-deploys (or trigger manually from Render dashboard)
5. Verify on live URL
```

### Step by Step

```bash
# 1. Pull the latest code
git pull origin main

# 2. Make your changes using your editor

# 3. Test locally
python server.py
# Visit http://localhost:5000 to verify

# 4. Commit
git add .
git commit -m "Description of what changed"

# 5. Push to GitHub
git push origin main

# 6. If Auto-Deploy is enabled on Render, deployment starts automatically
#    Otherwise, go to Render dashboard → Manual Deploy
```

### Checklist Before Deploying

- [ ] Reverted any `API_URL` changes from `localhost` back to the Render URL in HTML templates
- [ ] `requirements.txt` is updated if you added new packages
- [ ] `debug=False` in `server.py` (never deploy with debug mode on)
- [ ] Tested locally without errors

### Updating Python Packages

If you install a new package:
```bash
pip install new-package
pip freeze > requirements.txt
```

Then commit `requirements.txt`. Render will install the new package on next deploy.

---

## 14. Monitoring & Health Checks

### Health Check Endpoint

**URL:** `https://token-management-system-cvo8.onrender.com/health`

**Expected response:**
```json
{
  "status": "online",
  "service": "GTM4Health Token Management System"
}
```

You can ping this URL using tools like:
- [UptimeRobot](https://uptimerobot.com) — free uptime monitoring, sends alerts if the server goes down
- [Better Uptime](https://betteruptime.com)
- A simple `curl` from a cron job on another server

### Setting Up UptimeRobot (Recommended)

1. Create a free account at [uptimerobot.com](https://uptimerobot.com)
2. Click **Add New Monitor**
3. Set:
   - Monitor Type: **HTTP(s)**
   - Friendly Name: `TMS Backend`
   - URL: `https://token-management-system-cvo8.onrender.com/health`
   - Monitoring Interval: **5 minutes**
4. Add your email for alerts
5. This also keeps the Render free-tier server warm (prevents sleep)

### Render Logs

Access live server logs at:  
Render Dashboard → your service → **Logs tab**

Logs show:
- Every incoming HTTP request with status code
- Socket.IO connection and disconnection events
- Python errors and stack traces

---

## 15. Common Backend Issues & Fixes

### Issue: Server Not Responding / Timeout

**Cause:** Render free tier server is asleep (15 min inactivity timeout)  
**Fix:** Wait 30–60 seconds and refresh. Set up UptimeRobot to prevent sleep.

---

### Issue: `500 Internal Server Error` on an Endpoint

**Cause:** Unhandled exception in `server.py`  
**Fix:**
1. Check Render logs for the full Python traceback
2. Reproduce locally: `python server.py` and call the endpoint
3. Fix the bug, commit, and redeploy

---

### Issue: Socket.IO Not Connecting (CORS Error in Browser Console)

**Cause:** A device is connecting from an origin not in `cors_allowed_origins`  
**Fix:** Add the new origin to both the Flask-CORS and SocketIO configurations in `server.py`, redeploy.

---

### Issue: Token Numbers Didn't Reset After Calling `/reset`

**Cause:** Flask-SQLite connection caching or `sqlite_sequence` not being cleared  
**Fix:** The reset endpoint deletes from both `tokens` and `sqlite_sequence`. If this fails, check Render logs and run the reset query manually via SQLite CLI on the server.

---

### Issue: Database Lost After Redeploy

**Cause:** Render's free tier uses **ephemeral disk storage** — the filesystem resets on each new deployment  
**Fix Options:**
1. **Upgrade to a paid Render plan** with a persistent disk attached to the service
2. **Migrate to PostgreSQL** — Render has a managed PostgreSQL addon; update `server.py` to use `psycopg2` instead of `sqlite3`
3. **Accept it** — for a queue system, tokens resetting daily is often acceptable behaviour

---

### Issue: `eventlet` Import Error on Deploy

**Cause:** `eventlet` may conflict with certain Python versions  
**Fix:** Make sure `requirements.txt` lists `eventlet` explicitly. If the issue persists, pin the version:
```
eventlet==0.36.1
```

---

### Issue: Render Build Failing

**Cause:** Missing package in `requirements.txt`, or syntax error in `server.py`  
**Fix:**
1. Check Render build logs for the specific error
2. Reproduce locally: `pip install -r requirements.txt && python server.py`
3. Fix and push again

---

## 16. Important Limitations to Know

| Limitation | Detail | Mitigation |
|------------|--------|-----------|
| **Free tier sleep** | Server sleeps after 15 min of inactivity | Use UptimeRobot or upgrade Render plan |
| **Ephemeral database** | `token.db` is wiped on each redeploy | Use Render Disk or PostgreSQL for persistence |
| **Single server, no scaling** | Flask + eventlet runs as a single process | Sufficient for a small clinic; for multi-room scale, migrate to Redis + multiple workers |
| **No authentication** | All API endpoints are public — anyone with the URL can call `/reset` | Implement token-based auth or IP allowlisting for production |
| **Speech synthesis browser-only** | Text-to-speech is a browser Web Speech API feature — not server-side | Must use Chrome on the Main Display device |
| **SQLite concurrency** | SQLite has write-locking under very high concurrency | Acceptable for clinic use; switch to PostgreSQL for high volume |

---

*For staff-facing usage instructions, see **FRONTEND_GUIDE.md**.*

---

**© GTM4Health 2026 — Internal Technical Documentation**
