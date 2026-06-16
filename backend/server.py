#!/usr/bin/env python3
"""Minecraft PE account server — Flask REST API with SQLite backend."""

import hashlib
import os
import re
import secrets
import sqlite3

from flask import Flask, g, jsonify, request, send_file

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.join(BASE_DIR, "accounts.db")
SKINS_DIR = os.path.join(BASE_DIR, "skins")

os.makedirs(SKINS_DIR, exist_ok=True)

# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------
app = Flask(__name__)

# ---------------------------------------------------------------------------
# Database helpers
# ---------------------------------------------------------------------------
USERNAME_RE = re.compile(r"^[A-Za-z0-9_]{1,25}$")
MAX_SKIN_BYTES = 8 * 1024 * 1024  # 8 MB


def get_db() -> sqlite3.Connection:
    if "db" not in g:
        conn = sqlite3.connect(DB_PATH)
        conn.row_factory = sqlite3.Row
        g.db = conn
    return g.db


@app.teardown_appcontext
def close_db(exc=None):
    db = g.pop("db", None)
    if db is not None:
        db.close()


def init_db():
    with app.app_context():
        db = get_db()
        db.execute(
            """
            CREATE TABLE IF NOT EXISTS accounts (
                id       INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT    NOT NULL UNIQUE,
                password TEXT    NOT NULL,
                token    TEXT    UNIQUE
            )
            """
        )
        db.commit()


def hash_password(password: str) -> str:
    return hashlib.sha256(password.encode()).hexdigest()


# ---------------------------------------------------------------------------
# Auth helper
# ---------------------------------------------------------------------------
def _get_user_from_token():
    """Return the accounts row for the Bearer token, or None."""
    auth = request.headers.get("Authorization", "")
    if not auth.startswith("Bearer "):
        return None
    token = auth[len("Bearer "):]
    db = get_db()
    row = db.execute(
        "SELECT * FROM accounts WHERE token = ?", (token,)
    ).fetchone()
    return row


# ---------------------------------------------------------------------------
# CORS — allow all origins on every response
# ---------------------------------------------------------------------------
@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, OPTIONS"
    return response


@app.route("/api/<path:path>", methods=["OPTIONS"])
def handle_options(path):
    return jsonify({}), 200


# ---------------------------------------------------------------------------
# POST /api/register
# ---------------------------------------------------------------------------
@app.route("/api/register", methods=["POST"])
def register():
    data = request.get_json(silent=True) or {}
    username = data.get("username", "").strip()
    password = data.get("password", "")

    if not username or not password:
        return jsonify({"error": "username and password required"}), 400

    if not USERNAME_RE.match(username):
        return jsonify(
            {"error": "username must be 1-25 alphanumeric/underscore characters"}
        ), 400

    db = get_db()
    existing = db.execute(
        "SELECT id FROM accounts WHERE username = ?", (username,)
    ).fetchone()
    if existing:
        return jsonify({"error": "username already taken"}), 409

    token = secrets.token_hex(32)
    db.execute(
        "INSERT INTO accounts (username, password, token) VALUES (?, ?, ?)",
        (username, hash_password(password), token),
    )
    db.commit()

    return jsonify({"token": token, "username": username}), 201


# ---------------------------------------------------------------------------
# POST /api/login
# ---------------------------------------------------------------------------
@app.route("/api/login", methods=["POST"])
def login():
    data = request.get_json(silent=True) or {}
    username = data.get("username", "").strip()
    password = data.get("password", "")

    if not username or not password:
        return jsonify({"error": "username and password required"}), 400

    db = get_db()
    row = db.execute(
        "SELECT * FROM accounts WHERE username = ?", (username,)
    ).fetchone()

    if row is None or row["password"] != hash_password(password):
        return jsonify({"error": "invalid credentials"}), 401

    token = secrets.token_hex(32)
    db.execute(
        "UPDATE accounts SET token = ? WHERE id = ?", (token, row["id"])
    )
    db.commit()

    return jsonify({"token": token, "username": username}), 200


# ---------------------------------------------------------------------------
# GET /api/verify
# ---------------------------------------------------------------------------
@app.route("/api/verify", methods=["GET"])
def verify():
    row = _get_user_from_token()
    if row is None:
        return jsonify({"error": "invalid or missing token"}), 401
    return jsonify({"username": row["username"]}), 200


# ---------------------------------------------------------------------------
# PUT /api/username
# ---------------------------------------------------------------------------
@app.route("/api/username", methods=["PUT"])
def change_username():
    row = _get_user_from_token()
    if row is None:
        return jsonify({"error": "invalid or missing token"}), 401

    data = request.get_json(silent=True) or {}
    new_username = data.get("username", "").strip()

    if not new_username:
        return jsonify({"error": "username required"}), 400

    if not USERNAME_RE.match(new_username):
        return jsonify(
            {"error": "username must be 1-25 alphanumeric/underscore characters"}
        ), 400

    db = get_db()
    existing = db.execute(
        "SELECT id FROM accounts WHERE username = ? AND id != ?",
        (new_username, row["id"]),
    ).fetchone()
    if existing:
        return jsonify({"error": "username already taken"}), 409

    # Rename skin file if it exists
    old_skin = os.path.join(SKINS_DIR, f"{row['username']}.png")
    new_skin = os.path.join(SKINS_DIR, f"{new_username}.png")
    if os.path.isfile(old_skin):
        os.rename(old_skin, new_skin)

    db.execute(
        "UPDATE accounts SET username = ? WHERE id = ?", (new_username, row["id"])
    )
    db.commit()

    return jsonify({"username": new_username}), 200


# ---------------------------------------------------------------------------
# POST /api/skin
# ---------------------------------------------------------------------------
@app.route("/api/skin", methods=["POST"])
def upload_skin():
    row = _get_user_from_token()
    if row is None:
        return jsonify({"error": "invalid or missing token"}), 401

    if "skin" not in request.files:
        return jsonify({"error": "skin file required (multipart field: skin)"}), 400

    file = request.files["skin"]
    if file.filename == "":
        return jsonify({"error": "no file selected"}), 400

    # Basic PNG magic-byte check
    header = file.read(8)
    file.seek(0)
    PNG_MAGIC = b"\x89PNG\r\n\x1a\n"
    if header != PNG_MAGIC:
        return jsonify({"error": "skin must be a PNG file"}), 400

    # Size check — read entire file into memory to measure, then reject if too large
    data = file.read()
    if len(data) > MAX_SKIN_BYTES:
        return jsonify({"error": f"skin file too large (max {MAX_SKIN_BYTES // (1024*1024)} MB)"}), 413

    skin_path = os.path.join(SKINS_DIR, f"{row['username']}.png")
    with open(skin_path, "wb") as f:
        f.write(data)

    return jsonify({"ok": True}), 200


# ---------------------------------------------------------------------------
# GET /api/skin/<username>
# ---------------------------------------------------------------------------
@app.route("/api/skin/<username>", methods=["GET"])
def get_skin(username):
    skin_path = os.path.join(SKINS_DIR, f"{username}.png")
    if not os.path.isfile(skin_path):
        return jsonify({"error": "no skin"}), 404
    return send_file(skin_path, mimetype="image/png")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=8080, debug=False)
