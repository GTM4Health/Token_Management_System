from flask import Flask, jsonify, render_template, request
from flask_socketio import SocketIO
from flask_cors import CORS
import sqlite3
import os

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
            generated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    """)

    conn.commit()
    conn.close()


init_db()


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

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        WHERE status='Serving'
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()

    conn.close()

    if row is None:
        return jsonify({
            "token": "---"
        })

    return jsonify({
        "token": row[0]
    })


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

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        ORDER BY id DESC
        LIMIT 1
    """)

    last = cursor.fetchone()

    if last is None:
        number = 1
    else:
        number = int(last[0]) + 1

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

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    # Complete current serving token
    cursor.execute("""
        UPDATE tokens
        SET status='Completed'
        WHERE status='Serving'
    """)

    # Find oldest waiting token
    cursor.execute("""
        SELECT id, token
        FROM tokens
        WHERE status='Waiting'
        ORDER BY id ASC
        LIMIT 1
    """)

    row = cursor.fetchone()

    if row is None:

        conn.commit()
        conn.close()

        return jsonify({
            "message": "No waiting tokens",
            "serving": "---"
        })

    token_id = row[0]
    token = row[1]

    # Make token serving
    cursor.execute("""
        UPDATE tokens
        SET status='Serving'
        WHERE id=?
    """, (token_id,))

    conn.commit()
    conn.close()

    # Notify connected clients
    socketio.emit(
        "serving_changed",
        {
            "token": token
        }
    )

    return jsonify({
        "serving": token
    })


# ==========================
# CALL AGAIN
# ==========================

@app.route('/call_again', methods=['POST'])
def call_again():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        WHERE status='Serving'
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()

    conn.close()

    if row is None:

        return jsonify({
            "message": "No token serving",
            "token": "---"
        })

    token = row[0]

    # Notify connected clients
    socketio.emit(
        "serving_changed",
        {
            "token": token
        }
    )

    return jsonify({
        "token": token
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

    # Complete current serving token
    cursor.execute("""
        UPDATE tokens
        SET status='Completed'
        WHERE status='Serving'
    """)

    # Make requested token serving
    cursor.execute("""
        UPDATE tokens
        SET status='Serving'
        WHERE id=?
    """, (token_id,))

    conn.commit()
    conn.close()

    # Notify connected clients
    socketio.emit(
        "serving_changed",
        {
            "token": token
        }
    )

    return jsonify({
        "message": "Called",
        "token": token
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
