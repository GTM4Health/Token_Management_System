from flask import Flask, jsonify, render_template, request
from flask_socketio import SocketIO
from flask_cors import CORS
import sqlite3
import os
from datetime import datetime

# ==========================
# Flask App Configuration
# ==========================

app = Flask(__name__)

# Allow Vercel frontend to communicate with Render
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

# Socket.IO configuration
socketio = SocketIO(
    app,
    cors_allowed_origins=[
        "https://gtm-tms.vercel.app"
    ],
    async_mode="eventlet"
)


# ==========================
# Database Configuration
# ==========================

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATABASE = os.path.join(BASE_DIR, "token.db")


# ==========================
# Database Initialization
# ==========================

def init_db():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS tokens(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            token TEXT NOT NULL,
            status TEXT NOT NULL,
            counter TEXT DEFAULT 'Counter 1',
            generated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS devices(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT UNIQUE NOT NULL,
            device_type TEXT NOT NULL DEFAULT 'calling_unit',
            unit_number INTEGER NOT NULL DEFAULT 1,
            unit_name TEXT NOT NULL,
            proceed_text TEXT NOT NULL,
            last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    """)

    # Safe migration: ensure counter column exists if table was created previously
    cursor.execute("PRAGMA table_info(tokens)")
    columns = [col[1] for col in cursor.fetchall()]
    if "counter" not in columns:
        cursor.execute("ALTER TABLE tokens ADD COLUMN counter TEXT DEFAULT 'Counter 1'")
    if "called_at" not in columns:
        cursor.execute("ALTER TABLE tokens ADD COLUMN called_at TIMESTAMP")

    conn.commit()
    conn.close()


init_db()


def resolve_counter(data):
    """
    Resolves counter name and unit name from request payload.
    Supports either 'device_id' (MAC) or explicit 'counter'.
    """
    device_id = (data.get("device_id") or "").strip().upper()
    explicit_counter = (data.get("counter") or "").strip()

    target_key = device_id or explicit_counter.upper()

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    cursor.execute("""
        SELECT unit_name, proceed_text
        FROM devices
        WHERE device_id=? OR unit_name=?
    """, (target_key, explicit_counter))
    row = cursor.fetchone()
    if row:
        unit_name, proceed_text = row
        cursor.execute("UPDATE devices SET last_seen=CURRENT_TIMESTAMP WHERE device_id=? OR unit_name=?", (target_key, explicit_counter))
        conn.commit()
        conn.close()
        return proceed_text, unit_name

    if target_key and not target_key.startswith("IP:"):
        cursor.execute("SELECT COUNT(*) FROM devices WHERE device_type='calling_unit' AND device_id NOT LIKE 'IP:%'")
        cnt = cursor.fetchone()[0] + 1
        unit_name = f"Calling Unit {cnt}"
        proceed_text = f"Room {cnt}"
        cursor.execute("""
            INSERT INTO devices (device_id, device_type, unit_number, unit_name, proceed_text)
            VALUES (?, 'calling_unit', ?, ?, ?)
        """, (target_key, cnt, unit_name, proceed_text))
        conn.commit()
        conn.close()
        socketio.emit("devices_updated")
        return proceed_text, unit_name

    conn.close()

    if explicit_counter:
        return explicit_counter, explicit_counter

    return "Counter 1", "Calling Unit 1"


# ==========================
# MAIN DISPLAY
# ==========================

@app.route('/')
def home():
    return render_template("display.html")


@app.route('/display')
def display():
    return render_template("display.html")


# ==========================
# CURRENT SERVING TOKEN
# ==========================

@app.route('/current', methods=['GET'])
def current_token():

    counter = request.args.get('counter')

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    if counter:
        cursor.execute("""
            SELECT token, counter
            FROM tokens
            WHERE status='Serving' AND counter=?
            ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
            LIMIT 1
        """, (counter,))
    else:
        cursor.execute("""
            SELECT token, counter
            FROM tokens
            WHERE status='Serving'
            ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
            LIMIT 1
        """)

    row = cursor.fetchone()

    # If no token is currently Serving, fall back to the most recent token served/completed
    # so the display preserves whatever token was last called (without jumping to waiting tokens)
    if row is None:
        if counter:
            cursor.execute("""
                SELECT token, counter
                FROM tokens
                WHERE status IN ('Serving', 'Completed') AND counter=?
                ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
                LIMIT 1
            """, (counter,))
        else:
            cursor.execute("""
                SELECT token, counter
                FROM tokens
                WHERE status IN ('Serving', 'Completed')
                ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
                LIMIT 1
            """)
        row = cursor.fetchone()

    conn.close()

    if row is None:
        return jsonify({
            "token": "---",
            "counter": counter or "---"
        })

    return jsonify({
        "token": row[0],
        "counter": row[1] or "Counter 1"
    })


# ==========================
# ALL SERVING TOKENS
# ==========================

@app.route('/serving_all', methods=['GET'])
def serving_all():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token, counter
        FROM tokens
        WHERE status='Serving'
        ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
    """)

    rows = cursor.fetchall()
    conn.close()

    return jsonify([
        {
            "token": row[0],
            "counter": row[1] or "Counter 1"
        }
        for row in rows
    ])


# ==========================
# DEVICE REGISTRATION (ESP32 Handshake)
# ==========================

@app.route('/api/device/register', methods=['POST'])
def register_device():

    data = request.get_json(silent=True) or {}
    device_id = (data.get("device_id") or "").strip().upper()
    device_type = (data.get("device_type") or "calling_unit").strip().lower()

    if not device_id:
        return jsonify({"error": "device_id is required"}), 400

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT unit_number, unit_name, proceed_text, device_type
        FROM devices
        WHERE device_id=?
    """, (device_id,))
    row = cursor.fetchone()

    if row:
        unit_number, unit_name, proceed_text, d_type = row
        cursor.execute("UPDATE devices SET last_seen=CURRENT_TIMESTAMP WHERE device_id=?", (device_id,))
        conn.commit()
        conn.close()
        return jsonify({
            "device_id": device_id,
            "unit_number": unit_number,
            "unit_name": unit_name,
            "proceed_text": proceed_text,
            "device_type": d_type,
            "is_new": False
        })

    # Auto-assign next sequential number for this device type
    cursor.execute("SELECT COUNT(*) FROM devices WHERE device_type=?", (device_type,))
    count = cursor.fetchone()[0]
    unit_number = count + 1

    if device_type == "calling_unit":
        unit_name = f"Calling Unit {unit_number}"
        proceed_text = f"Room {unit_number}"
    else:
        unit_name = f"Dispenser {unit_number}"
        proceed_text = f"Entrance {unit_number}"

    cursor.execute("""
        INSERT INTO devices (device_id, device_type, unit_number, unit_name, proceed_text)
        VALUES (?, ?, ?, ?, ?)
    """, (device_id, device_type, unit_number, unit_name, proceed_text))

    conn.commit()
    conn.close()

    socketio.emit("devices_updated")

    return jsonify({
        "device_id": device_id,
        "unit_number": unit_number,
        "unit_name": unit_name,
        "proceed_text": proceed_text,
        "device_type": device_type,
        "is_new": True
    })


# ==========================
# DEVICE MANAGEMENT APIS
# ==========================

# ==========================
# DEVICE HEARTBEAT PING
# ==========================

@app.route('/api/device/ping', methods=['GET', 'POST'])
def ping_device():

    data = request.get_json(silent=True) or {}
    device_id = (request.args.get("device_id") or data.get("device_id") or "").strip().upper()

    if not device_id:
        return jsonify({"error": "device_id is required"}), 400

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    cursor.execute("UPDATE devices SET last_seen=CURRENT_TIMESTAMP WHERE device_id=?", (device_id,))
    updated = cursor.rowcount > 0
    conn.commit()
    conn.close()

    if updated:
        return jsonify({"status": "ok", "device_id": device_id})
    return jsonify({"status": "unknown_device", "device_id": device_id}), 404





@app.route('/api/devices', methods=['GET'])
def list_devices():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    cursor.execute("""
        SELECT id, device_id, device_type, unit_number, unit_name, proceed_text, last_seen,
               ROUND((strftime('%s', 'now') - strftime('%s', last_seen))) as seconds_ago
        FROM devices
        WHERE device_id NOT LIKE 'IP:%'
        ORDER BY device_type ASC, id ASC
    """)
    rows = cursor.fetchall()
    conn.close()

    devices = []
    type_counts = {}
    for r in rows:
        d_type = r[2]
        type_counts[d_type] = type_counts.get(d_type, 0) + 1
        clean_num = type_counts[d_type]
        seconds_ago = r[7] if r[7] is not None else 999999
        # Online if heartbeat received in last 6 seconds
        is_online = seconds_ago is not None and 0 <= seconds_ago <= 6
        devices.append({
            "id": r[0],
            "device_id": r[1],
            "device_type": d_type,
            "unit_number": clean_num,
            "unit_name": r[4],
            "proceed_text": r[5],
            "last_seen": r[6],
            "seconds_ago": seconds_ago,
            "status": "Online" if is_online else "Offline"
        })
    return jsonify(devices)


@app.route('/api/device/update', methods=['POST'])
def update_device():

    data = request.get_json(silent=True) or {}
    device_id = (data.get("device_id") or "").strip().upper()
    unit_name = (data.get("unit_name") or "").strip()
    proceed_text = (data.get("proceed_text") or "").strip()

    if not device_id:
        return jsonify({"error": "device_id is required"}), 400

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("SELECT id FROM devices WHERE device_id=?", (device_id,))
    if not cursor.fetchone():
        conn.close()
        return jsonify({"error": "Device not found"}), 404

    cursor.execute("""
        UPDATE devices
        SET unit_name=COALESCE(NULLIF(?, ''), unit_name),
            proceed_text=COALESCE(NULLIF(?, ''), proceed_text)
        WHERE device_id=?
    """, (unit_name, proceed_text, device_id))

    conn.commit()
    conn.close()

    socketio.emit("devices_updated")

    return jsonify({
        "message": "Device updated successfully",
        "device_id": device_id,
        "unit_name": unit_name,
        "proceed_text": proceed_text
    })


@app.route('/api/device/delete', methods=['POST'])
def delete_device():

    data = request.get_json(silent=True) or {}
    device_id = (data.get("device_id") or "").strip().upper()

    if not device_id:
        return jsonify({"error": "device_id is required"}), 400

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    cursor.execute("DELETE FROM devices WHERE device_id=?", (device_id,))
    conn.commit()
    conn.close()

    socketio.emit("devices_updated")

    return jsonify({
        "message": "Device deleted successfully",
        "device_id": device_id
    })


# ==========================
# ADMIN PAGE
# ==========================

@app.route('/admin')
def admin_page():
    return render_template("admin.html")


# ==========================
# LATEST GENERATED TOKEN
# ==========================

@app.route('/latest', methods=['GET'])
def latest_token():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()

    conn.close()

    if row is None:
        return jsonify({
            "token": "000"
        })

    return jsonify({
        "token": row[0]
    })


# ==========================
# TOKEN DISPENSER
# ==========================

@app.route('/kiosk')
def kiosk():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        ORDER BY id DESC
        LIMIT 1
    """)

    last = cursor.fetchone()

    conn.close()

    if last is None:
        token = "000"
    else:
        token = last[0]

    return render_template(
        "kiosk.html",
        token=token
    )


# ==========================
# GENERATE TOKEN
# ==========================

@app.route('/generate', methods=['POST'])
def generate_token():

    data = request.get_json(silent=True) or {}
    device_id = (data.get("device_id") or "").strip().upper()

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    if device_id and not device_id.startswith("IP:"):
        cursor.execute("SELECT id FROM devices WHERE device_id=?", (device_id,))
        if cursor.fetchone():
            cursor.execute("UPDATE devices SET last_seen=CURRENT_TIMESTAMP WHERE device_id=?", (device_id,))
        else:
            cursor.execute("SELECT COUNT(*) FROM devices WHERE device_type='dispenser'")
            cnt = cursor.fetchone()[0] + 1
            cursor.execute("""
                INSERT INTO devices (device_id, device_type, unit_number, unit_name, proceed_text)
                VALUES (?, 'dispenser', ?, ?, ?)
            """, (device_id, cnt, f"Dispenser {cnt}", f"Entrance {cnt}"))
            socketio.emit("devices_updated")

    cursor.execute("""
        SELECT token
        FROM tokens
        ORDER BY id DESC
        LIMIT 1
    """)

    last = cursor.fetchone()

    max_db_number = int(last[0]) if last and str(last[0]).isdigit() else 0
    client_last_number = 0
    try:
        client_last_number = int(data.get("last_token") or 0)
    except (ValueError, TypeError):
        client_last_number = 0

    number = max(max_db_number, client_last_number) + 1
    token = f"{number:03d}"

    cursor.execute(
        """
        INSERT INTO tokens (token, status)
        VALUES (?, ?)
        """,
        (token, "Waiting")
    )

    conn.commit()
    conn.close()

    # Notify connected clients
    socketio.emit(
        "new_token",
        {
            "token": token
        }
    )

    return jsonify({
        "token": token
    })


# ==========================
# CALL NEXT TOKEN
# ==========================

@app.route('/next', methods=['POST'])
def next_token():

    data = request.get_json(silent=True) or {}
    counter, unit_name = resolve_counter(data)

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    # 1. Find oldest waiting token FIRST
    cursor.execute("""
        SELECT id, token
        FROM tokens
        WHERE status='Waiting'
        ORDER BY id ASC
        LIMIT 1
    """)

    row = cursor.fetchone()

    # If NO waiting tokens in queue:
    if row is None:
        # Keep the currently serving token active! DO NOT mark anything completed!
        cursor.execute("""
            SELECT token
            FROM tokens
            WHERE status='Serving' AND counter=?
            ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
            LIMIT 1
        """, (counter,))
        curr = cursor.fetchone()
        if curr is None:
            cursor.execute("""
                SELECT token
                FROM tokens
                WHERE counter=?
                ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
                LIMIT 1
            """, (counter,))
            curr = cursor.fetchone()

        current_token = curr[0] if curr else "---"
        conn.close()

        return jsonify({
            "message": "No waiting tokens",
            "serving": current_token,
            "token": current_token,
            "counter": counter,
            "unit_name": unit_name
        })

    token_id = row[0]
    token = row[1]
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")

    # ONLY when an actual new token exists:
    # 2. Mark previous serving token for this counter as Completed
    cursor.execute("""
        UPDATE tokens
        SET status='Completed'
        WHERE status='Serving' AND counter=?
    """, (counter,))

    # 3. Mark the new waiting token as Serving for this counter
    cursor.execute("""
        UPDATE tokens
        SET status='Serving', counter=?, called_at=?
        WHERE id=?
    """, (counter, now_str, token_id))

    conn.commit()
    conn.close()

    # 4. Notify connected clients
    socketio.emit(
        "serving_changed",
        {
            "token": token,
            "counter": counter,
            "unit_name": unit_name
        }
    )

    return jsonify({
        "serving": token,
        "token": token,
        "counter": counter,
        "unit_name": unit_name
    })


# ==========================
# CALL AGAIN
# ==========================

@app.route('/call_again', methods=['POST'])
def call_again():

    data = request.get_json(silent=True) or {}
    counter, unit_name = resolve_counter(data)

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    # 1. Look for active Serving token for this counter
    cursor.execute("""
        SELECT id, token, counter
        FROM tokens
        WHERE status='Serving' AND counter=?
        ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
        LIMIT 1
    """, (counter,))
    row = cursor.fetchone()

    # 2. If none serving for this counter, look for last token called by this counter (even if Completed)
    if row is None:
        cursor.execute("""
            SELECT id, token, counter
            FROM tokens
            WHERE counter=?
            ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
            LIMIT 1
        """, (counter,))
        row = cursor.fetchone()

    # 3. Fall back to ANY active serving token across any counter
    if row is None:
        cursor.execute("""
            SELECT id, token, counter
            FROM tokens
            WHERE status='Serving'
            ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
            LIMIT 1
        """)
        row = cursor.fetchone()

    # 4. Fall back to most recent token called across any counter
    if row is None:
        cursor.execute("""
            SELECT id, token, counter
            FROM tokens
            WHERE status IN ('Serving', 'Completed')
            ORDER BY COALESCE(called_at, generated_time) DESC, id DESC
            LIMIT 1
        """)
        row = cursor.fetchone()

    # 5. Last fallback: most recent token in tokens table
    if row is None:
        cursor.execute("""
            SELECT id, token, counter
            FROM tokens
            ORDER BY id DESC
            LIMIT 1
        """)
        row = cursor.fetchone()

    if row is None:
        conn.close()
        return jsonify({
            "message": "No token serving",
            "token": "---",
            "serving": "---",
            "counter": counter,
            "unit_name": unit_name
        })

    token_id = row[0]
    token = row[1]
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")

    # Mark other serving tokens on this counter as Completed
    cursor.execute("""
        UPDATE tokens
        SET status='Completed'
        WHERE status='Serving' AND counter=? AND id != ?
    """, (counter, token_id))

    # Ensure this token is in Serving status for THIS counter
    cursor.execute("""
        UPDATE tokens
        SET status='Serving', counter=?, called_at=?
        WHERE id=?
    """, (counter, now_str, token_id))
    conn.commit()
    conn.close()

    # Notify connected clients
    socketio.emit(
        "serving_changed",
        {
            "token": token,
            "counter": counter,
            "unit_name": unit_name
        }
    )

    return jsonify({
        "token": token,
        "serving": token,
        "counter": counter,
        "unit_name": unit_name
    })


# ==========================
# CALL SPECIFIC TOKEN
# ==========================

@app.route('/call_specific', methods=['POST'])
def call_specific():

    data = request.get_json(silent=True)

    if not data or "token" not in data:

        return jsonify({
            "message": "Token is required"
        }), 400

    token = str(data["token"]).strip().zfill(3)
    counter, unit_name = resolve_counter(data)

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    # Find requested token
    cursor.execute("""
        SELECT id, token
        FROM tokens
        WHERE token=?
        LIMIT 1
    """, (token,))

    row = cursor.fetchone()

    if row is None:

        conn.close()

        return jsonify({
            "message": "Invalid Token"
        }), 404

    token_id = row[0]
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")

    # Complete current serving token ONLY for THIS counter
    cursor.execute("""
        UPDATE tokens
        SET status='Completed'
        WHERE status='Serving' AND counter=?
    """, (counter,))

    # Make requested token serving for THIS counter
    cursor.execute("""
        UPDATE tokens
        SET status='Serving', counter=?, called_at=?
        WHERE id=?
    """, (counter, now_str, token_id))

    conn.commit()
    conn.close()

    # Notify connected clients
    socketio.emit(
        "serving_changed",
        {
            "token": token,
            "counter": counter,
            "unit_name": unit_name
        }
    )

    return jsonify({
        "message": "Called",
        "token": token,
        "serving": token,
        "counter": counter,
        "unit_name": unit_name
    })


# ==========================
# RESET QUEUE
# ==========================

@app.route('/reset', methods=['POST'])
def reset():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    # Delete all tokens
    cursor.execute("""
        DELETE FROM tokens
    """)

    # Reset ID sequence
    cursor.execute("""
        DELETE FROM sqlite_sequence
        WHERE name='tokens'
    """)

    conn.commit()
    conn.close()

    # Notify connected clients
    socketio.emit("queue_reset")

    return jsonify({
        "message": "Queue Reset Successfully",
        "token": "000"
    })


# ==========================
# WAITING QUEUE API
# ==========================

@app.route('/waiting', methods=['GET'])
def waiting():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        WHERE status='Waiting'
        ORDER BY id ASC
    """)

    rows = cursor.fetchall()

    conn.close()

    waiting_tokens = [
        row[0]
        for row in rows
    ]

    return jsonify(waiting_tokens)


# ==========================
# CALLING UNIT
# ==========================

@app.route('/operator')
def operator():
    return render_template("operator.html")


# ==========================
# PRINT TOKEN
# ==========================

@app.route('/print/<token>')
def print_token(token):

    return render_template(
        "print.html",
        token=token
    )


# ==========================
# HEALTH CHECK
# ==========================

@app.route('/health')
def health():

    return jsonify({
        "status": "online",
        "service": "GTM4Health Token Management System"
    })


# ==========================
# MAIN
# ==========================

if __name__ == '__main__':

    socketio.run(
        app,
        host="0.0.0.0",
        port=int(os.environ.get("PORT", 5000)),
        debug=False
    )
