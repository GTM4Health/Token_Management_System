from flask import Flask, jsonify, render_template, request
from flask_socketio import SocketIO
import sqlite3

app = Flask(__name__)
socketio = SocketIO(app)

DATABASE = "token.db"


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


# ==========================
# Kiosk Screen
# ==========================

@app.route('/')
def home():

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

    return render_template("kiosk.html", token=token)


# ==========================
# Generate Token
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
        number = int(last[0][1:]) + 1

    token = f"{number:03d}"

    cursor.execute(
        "INSERT INTO tokens (token, status) VALUES (?, ?)",
        (token, "Waiting")
    )

    conn.commit()
    conn.close()

    socketio.emit("new_token", {
        "token": token
    })

    return jsonify({
        "token": token
    })


# ==========================
# Call Next Token
# ==========================

@app.route('/next', methods=['POST'])
def next_token():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    # Complete previous serving token
    cursor.execute("""
        UPDATE tokens
        SET status='Completed'
        WHERE status='Serving'
    """)

    # Get next waiting token
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
            "message": "No waiting tokens"
        })

    token_id = row[0]
    token = row[1]

    cursor.execute("""
        UPDATE tokens
        SET status='Serving'
        WHERE id=?
    """, (token_id,))

    conn.commit()
    conn.close()

    socketio.emit("serving_changed", {
        "token": token
    })

    return jsonify({
        "serving": token
    })

# ==========================
# Call Again
# ==========================

@app.route('/call_again', methods=['POST'])
def call_again():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        WHERE status='Serving'
        LIMIT 1
    """)

    row = cursor.fetchone()

    conn.close()

    if row is None:

        return jsonify({
            "message": "No token serving"
        })

    socketio.emit("serving_changed", {
        "token": row[0]
    })

    return jsonify({
        "token": row[0]
    })
# ==========================
# Call Specific Token
# ==========================

@app.route("/call_specific", methods=["POST"])
def call_specific():

    data = request.get_json()

    token = data["token"].zfill(3)

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT token
        FROM tokens
        WHERE token=?
    """, (token,))

    row = cursor.fetchone()

    conn.close()

    if row is None:

        return jsonify({
            "message":"Invalid Token"
        })

    socketio.emit(
        "serving_changed",
        {
            "token":token
        }
    )

    return jsonify({
        "message":"Called"
    })

# ==========================
# Reset Queue
# ==========================

@app.route('/reset', methods=['POST'])
def reset():

    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()

    cursor.execute("DELETE FROM tokens")
    cursor.execute("DELETE FROM sqlite_sequence WHERE name='tokens'")

    conn.commit()
    conn.close()

    socketio.emit("queue_reset")

    return jsonify({
        "message": "Queue Reset Successfully"
    })


# ==========================
# Waiting Queue API
# ==========================

@app.route('/waiting')
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

    waiting = []

    for row in rows:
        waiting.append(row[0])

    return jsonify(waiting)


# ==========================
# Doctor Dashboard
# ==========================

@app.route('/operator')
def operator():

    return render_template("operator.html")


# ==========================
# Public Display
# ==========================

@app.route('/display')
def display():

    return render_template("display.html")

# ==========================
# Print Token
# ==========================

@app.route("/print/<token>")
def print_token(token):

    return render_template(
        "print.html",
        token=token
    )
# ==========================
# Main
# ==========================

if __name__ == '__main__':

    init_db()

    socketio.run(
        app,
        host="0.0.0.0",
        port=5000,
        debug=True
    )