# GTM4Health Token Management System
## Frontend User Guide

**Version:** 1.0 | **Last Updated:** October 2026  
**Live URL:** [https://gtm-tms.vercel.app](https://gtm-tms.vercel.app)

---

## Who This Document Is For

This guide is for **any staff member, receptionist, or new employee** who will be using the Token Management System (TMS) on a daily basis. No technical background is required. After reading this document, you will know how to:

- Set up and use all three interfaces (Main Display, Calling Unit, Token Dispenser)
- Run a full patient queue session from start to finish
- Handle common situations (calling again, skipping tokens, resetting the queue)
- Understand what each screen shows and why

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [What You Need](#2-what-you-need)
3. [The Three Interfaces](#3-the-three-interfaces)
4. [Daily Workflow — Step by Step](#4-daily-workflow--step-by-step)
5. [Main Display — Detailed Guide](#5-main-display--detailed-guide)
6. [Token Dispenser (Kiosk) — Detailed Guide](#6-token-dispenser-kiosk--detailed-guide)
7. [Calling Unit (Operator Panel) — Detailed Guide](#7-calling-unit-operator-panel--detailed-guide)
8. [Navigating Between Pages](#8-navigating-between-pages)
9. [Common Scenarios & How to Handle Them](#9-common-scenarios--how-to-handle-them)
10. [Troubleshooting](#10-troubleshooting)
11. [Do's and Don'ts](#11-dos-and-donts)

---

## 1. System Overview

The **GTM4Health Token Management System** is a digital queue management tool. Its purpose is to replace paper-based or manual queuing at healthcare facilities. Here is how the system works at a high level:

```
PATIENT ARRIVES
      │
      ▼
Goes to the KIOSK (Token Dispenser)
      │
      │  → System generates their token number (e.g., 004)
      │  → A printed slip is produced (if printer is connected)
      │
      ▼
Patient waits in the lobby, watching the MAIN DISPLAY screen
      │
      │  → Display shows the current token being served
      │  → Displays the waiting queue on the side
      │  → Announces the token number via voice (text-to-speech)
      │
      ▼
Staff member (receptionist) clicks NEXT on the CALLING UNIT
      │
      │  → The next patient's token number appears live on the Main Display
      │  → Voice announcement plays automatically
      │
      ▼
Patient hears their number, proceeds to the room
```

All three interfaces update **in real time** — when you click "Next" on the Calling Unit, every screen in the building updates within a second.

---

## 2. What You Need

### Devices Required

| Device | Interface to Open | Who Uses It |
|--------|------------------|-------------|
| Large TV or monitor (mounted in lobby) | Main Display | Patients watch this |
| Tablet or desktop (at reception desk) | Calling Unit | Receptionist / Staff |
| Tablet or ESP32 kiosk (at entrance) | Token Dispenser | Patients use this |

### Requirements

- A stable **Wi-Fi or wired internet connection** on all devices
- A modern web browser on each device (**Google Chrome** is strongly recommended for the Main Display due to voice announcement)
- No app installation required — everything runs in the browser

### URLs at a Glance

| Interface | URL |
|-----------|-----|
| **Main Display** | https://gtm-tms.vercel.app/ |
| **Token Dispenser (Kiosk)** | https://gtm-tms.vercel.app/kiosk |
| **Calling Unit (Operator)** | https://gtm-tms.vercel.app/operator |

---

## 3. The Three Interfaces

### Interface 1 — Main Display

> **Open on:** The lobby TV or large monitor  
> **URL:** `https://gtm-tms.vercel.app/`

This screen is **read-only** for patients. They cannot interact with it. It shows:
- The **token number currently being served** (large, centre of screen)
- A **"Please proceed to the room"** message under the token number
- A **Waiting Queue panel** on the right showing all tokens yet to be called
- It **announces the token number aloud** every time a new token is called

---

### Interface 2 — Token Dispenser (Kiosk)

> **Open on:** The tablet or physical kiosk at the entrance  
> **URL:** `https://gtm-tms.vercel.app/kiosk`

Patients use this to get their queue number. It shows:
- The **most recently generated token number** in large text
- When a new token is generated, the screen updates and a **print popup** appears (if a printer is connected)

---

### Interface 3 — Calling Unit (Operator Panel)

> **Open on:** The receptionist's tablet or computer  
> **URL:** `https://gtm-tms.vercel.app/operator`

This is the **staff control panel**. From here you can:
- See the **current token being served**
- Call the **next token** in the queue
- **Call again** (repeat the current token announcement)
- **Call a specific token** by number
- **Reset** the entire queue at the end of the day

---

## 4. Daily Workflow — Step by Step

Follow these steps at the **start of every working day:**

### Step 1 — Open All Screens

1. On the **lobby TV**, open Chrome and go to:  
   `https://gtm-tms.vercel.app/`  
   Leave this in **full-screen mode** (press `F11` on a keyboard or use Chrome's full-screen option).

2. On the **kiosk tablet**, open Chrome and go to:  
   `https://gtm-tms.vercel.app/kiosk`  
   Leave this screen open and visible to patients.

3. On the **receptionist's device**, open Chrome and go to:  
   `https://gtm-tms.vercel.app/operator`

### Step 2 — Reset the Queue (Start of Day)

> ⚠️ Only do this at the **very start** of the day, before any patients arrive.

On the **Calling Unit**:
1. Scroll down and click the **Reset Queue** button.
2. A confirmation dialog will appear — click **OK** to confirm.
3. The token counter resets to `000`. The next generated token will be `001`.

### Step 3 — Patient Arrives and Gets a Token

- The patient walks up to the **kiosk tablet** and presses the button (or the on-screen button if using the web-only kiosk).
- The screen shows their token number (e.g., `003`).
- If a thermal printer is connected, a slip is printed automatically.

### Step 4 — Staff Calls the Next Patient

When you are ready to serve the next patient:
1. On the **Calling Unit**, click the **Next Token** button.
2. The Main Display automatically updates to show the new token number.
3. The lobby TV announces the number via voice automatically.
4. The patient hears/sees their number and proceeds to the room.

### Step 5 — Repeat

Continue clicking **Next Token** for each patient until the queue is empty.

### Step 6 — End of Day

At the end of the working day, click **Reset Queue** on the Calling Unit to clear all records for the next day.

---

## 5. Main Display — Detailed Guide

**URL:** `https://gtm-tms.vercel.app/`

### Layout

```
┌──────────────────────────────────────────────────────────────┐
│  [Logo]          MAIN DISPLAY              [Logo]    [⊞ Menu]│   ← Header (dark blue)
├───────────────────────────────────┬──────────────────────────┤
│                                   │  WAITING QUEUE           │
│           TOKEN NO.               │  3 WAITING               │
│                                   │ ┌────────────────────┐   │
│              005                  │ │       002          │   │
│                                   │ │       003          │   │
│    Please proceed to the room     │ │       004          │   │
│                                   │ └────────────────────┘   │
├───────────────────────────────────┴──────────────────────────┤
│                    © GTM4Health 2026                         │   ← Footer
└──────────────────────────────────────────────────────────────┘
```

### What Each Part Does

| Element | Description |
|---------|-------------|
| **Large token number (centre-left)** | The token currently being served. Shows `---` if no token is active. |
| **"Please proceed to the room"** | Static message below the token number to guide patients. |
| **Waiting Queue (right panel, dark blue)** | Lists all tokens waiting to be called, in order. Updates live. |
| **Queue count badge** | Shows how many patients are still waiting (e.g., "3 WAITING"). |
| **Voice Announcement** | Every time the token changes, the browser automatically speaks it aloud (e.g., *"Token Zero, Zero, Five. Please proceed to room."*). |
| **Reset Toast** | When the queue is reset, a blue banner briefly appears reading "All Tokens have been Reset". |

### Tips for the Main Display

- **Always use Google Chrome** — voice announcements only work reliably in Chrome.
- The first time you open the page in Chrome, it may ask for microphone/speech permissions — click **Allow**.
- Set the browser to **full screen** (`F11`) so it fills the TV completely.
- The display **refreshes automatically every 2 seconds** even without an internet push event, so it stays in sync.
- If the display seems stuck, simply **refresh the page** (press `F5`).

---

## 6. Token Dispenser (Kiosk) — Detailed Guide

**URL:** `https://gtm-tms.vercel.app/kiosk`

### Layout

```
┌──────────────────────────────────────────────────────────────┐
│  [Logo]         TOKEN DISPENSER            [Logo]    [⊞ Menu]│
├──────────────────────────────────────────────────────────────┤
│                                                              │
│                    YOUR TOKEN NUMBER                         │
│                                                              │
│                          007                                 │
│                                                              │
│             Please wait for your token to be called.        │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                    © GTM4Health 2026                         │
└──────────────────────────────────────────────────────────────┘
```

### How It Works

1. When a patient's token is generated (via physical ESP32 button or the web system), the large number on screen updates to their token.
2. A **print popup window** automatically opens — this is the token slip for the printer.
3. The patient can see their number on the big screen and on their printed slip.

### The Print Slip

The print popup (`/print/<token>`) shows:
- GTM4Health branding
- The token number (large)
- Current date and time
- A message: *"Please wait until your token is announced."*

The popup prints automatically and then closes itself.

### Disabling the Print Popup

If no printer is connected and the popup is annoying, you can disable it:
1. Open the browser **Console** (press `F12` → click "Console" tab).
2. Type: `localStorage.setItem("printPopup", "0")` and press Enter.
3. Close the console. The popup will no longer appear.

To re-enable: type `localStorage.removeItem("printPopup")` in the console.

---

## 7. Calling Unit (Operator Panel) — Detailed Guide

**URL:** `https://gtm-tms.vercel.app/operator`

This is the most important interface for staff. Here is everything you can do from this page.

### Layout

```
┌──────────────────────────────────────────────────────────────┐
│  [Logo]          CALLING UNIT              [Logo]    [⊞ Menu]│
├──────────────────────────────────────────────────────────────┤
│                                                              │
│                     CURRENT TOKEN                            │
│                                                              │
│                          003                                 │
│                                                              │
│         [Enter Token Number]        [Call Token]             │
│                                                              │
│     [Next Token]   [Call Again]   [Reset Queue]              │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                    © GTM4Health 2026                         │
└──────────────────────────────────────────────────────────────┘
```

### Button Reference

| Button | What It Does | When to Use |
|--------|-------------|-------------|
| **Next Token** | Calls the next patient in the waiting queue (FIFO order). The current patient is marked as done. | Standard flow — use this for every new patient. |
| **Call Again** | Re-announces the currently serving token without advancing the queue. | Patient didn't hear their number. |
| **Call Token** (with number input) | Calls any specific token number you type in the box, regardless of queue order. | Patient was missed and needs to be called out of sequence. |
| **Reset Queue** | Clears all tokens and resets the counter to zero. Shows a confirmation dialog. | Start of day or end of day cleanup. |

### Important Behaviours

- **Only one token can be "Serving" at a time.** When you click Next Token, the previous one is automatically marked as Completed.
- **The Main Display and Kiosk update instantly** when you press any button — no need to refresh them.
- **The Calling Unit itself updates live** via WebSocket — if someone else resets the queue or generates a token, your page reflects it immediately.

---

## 8. Navigating Between Pages

Every page has an **Apps Menu** (the grid icon ⊞) in the top-right corner of the header. Click it to navigate between the three interfaces:

| Menu Item | Goes To |
|-----------|---------|
| Main Display | `https://gtm-tms.vercel.app/` |
| Calling Unit | `https://gtm-tms.vercel.app/operator` |
| Token Dispenser | `https://gtm-tms.vercel.app/kiosk` |

---

## 9. Common Scenarios & How to Handle Them

### Scenario A: Patient Didn't Hear Their Number

1. On the **Calling Unit**, click **Call Again**.
2. The Main Display will show the same token again and re-announce it via voice.
3. The queue does **not** advance — the same patient is still being served.

---

### Scenario B: A Patient Needs to Be Called Out of Turn

1. On the **Calling Unit**, type the patient's token number into the **"Enter Token Number"** input box.
2. Click **Call Token**.
3. That patient's token will immediately appear on the Main Display.

> ⚠️ The previously serving token will be marked as **Completed**, so only do this intentionally.

---

### Scenario C: Queue Gets Stuck / Token Is Wrong

1. Use the **Call Specific Token** feature (enter the correct token number and click Call Token).
2. If the queue is badly out of sync, use **Reset Queue** and start fresh — but only if no patients are actively waiting.

---

### Scenario D: Kiosk Print Popup Keeps Blocking the Screen

1. Open browser console (`F12`) on the kiosk device.
2. Type: `localStorage.setItem("printPopup", "0")` and press Enter.
3. Refresh the page. Popups are now disabled.

---

### Scenario E: Voice Announcements Stopped Working on the Main Display

1. Check that the browser is **Google Chrome**.
2. Click anywhere on the page — browsers block auto-speech until the user has interacted with the page at least once.
3. If it still doesn't work, check the browser's **sound/speech permissions** (click the lock icon in the address bar → check "Sound" is set to Allow).

---

### Scenario F: Display Shows "---" for the Current Token

This is **normal** — it means no token is currently being served. As soon as you click **Next Token** on the Calling Unit, a number will appear.

---

### Scenario G: The Page Says "Unable to Connect to Server"

The backend server may be starting up (Render free tier goes to sleep after inactivity). Wait **30–60 seconds** and refresh the page. The server will wake up and reconnect automatically.

---

## 10. Troubleshooting

| Problem | Likely Cause | Solution |
|---------|-------------|----------|
| Page won't load | No internet connection | Check Wi-Fi; try refreshing |
| Queue count is wrong | Slight sync delay | Wait 2–3 seconds; page auto-refreshes |
| Voice not announcing | Browser not Chrome, or no user interaction | Use Chrome; click anywhere on the display first |
| Print popup not opening | Browser blocked popups | In Chrome address bar, click the blocked popup icon and allow popups from this site |
| Token number jumped | Someone used "Call Specific Token" | Check with staff; use the Calling Unit to correct |
| Server error or blank page | Backend restarting | Wait 30–60 seconds and refresh |
| Reset button not responding | Server hasn't confirmed yet | Wait a moment and try again |

---

## 11. Do's and Don'ts

### ✅ Do

- Open **Main Display in full-screen** on the lobby TV every morning.
- **Reset the queue at the start of each day** before patients arrive.
- Use **Google Chrome** for the Main Display (voice announcements).
- Keep the **Calling Unit open at all times** during operating hours.
- Click **Call Again** if a patient didn't hear their number — don't click **Next**.

### ❌ Don't

- Don't **refresh the Calling Unit** mid-session unnecessarily — the page stays in sync automatically.
- Don't click **Reset Queue** while patients are actively waiting — this clears all tokens.
- Don't close the kiosk tab — it must stay open to generate tokens.
- Don't use multiple browser tabs for the Calling Unit on the same device — one tab is enough.

---

*For backend (server, database, deployment) documentation, see **BACKEND_GUIDE.md**.*

---

**© GTM4Health 2026 — Internal Documentation**
