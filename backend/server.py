#!/usr/bin/env python3
"""Minecraft PE account server — Flask REST API with SQLite backend."""

import hashlib
import os
import re
import secrets
import sqlite3
import uuid as uuidlib

from flask import Flask, g, jsonify, request, send_file

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.join(BASE_DIR, "accounts.db")
SKINS_DIR = os.path.join(BASE_DIR, "skins")
# Curated cape images go here. Unlike skins, players don't upload these
# themselves — an admin drops a small set of approved cape PNGs into this
# folder by hand, and players just pick one (see GET /api/capes).
CAPES_DIR = os.path.join(BASE_DIR, "capes")

os.makedirs(SKINS_DIR, exist_ok=True)
os.makedirs(CAPES_DIR, exist_ok=True)

# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------
app = Flask(__name__)

# ---------------------------------------------------------------------------
# Database helpers
# ---------------------------------------------------------------------------
USERNAME_RE = re.compile(r"^[A-Za-z0-9_]{1,25}$")
MAX_SKIN_BYTES = 8 * 1024 * 1024  # 8 MB


def available_capes():
    """Names (without .png) of every cape file an admin has dropped into CAPES_DIR."""
    names = []
    for fn in os.listdir(CAPES_DIR):
        if fn.lower().endswith(".png"):
            names.append(fn[:-4])
    return sorted(names)


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
                token    TEXT    UNIQUE,
                uuid     TEXT
            )
            """
        )
        # Migrate databases created before the uuid/cape columns existed.
        existing_cols = {row["name"] for row in db.execute("PRAGMA table_info(accounts)")}
        if "uuid" not in existing_cols:
            db.execute("ALTER TABLE accounts ADD COLUMN uuid TEXT")
        if "cape" not in existing_cols:
            db.execute("ALTER TABLE accounts ADD COLUMN cape TEXT NOT NULL DEFAULT ''")
        db.execute("CREATE UNIQUE INDEX IF NOT EXISTS idx_accounts_uuid ON accounts(uuid)")
        db.commit()


def ensure_uuid(db: sqlite3.Connection, row: sqlite3.Row) -> str:
    """Every account gets a UUID that never changes even if the username does.
    Accounts created before this existed get one lazily, the moment they next
    contact the server (login or verify)."""
    if row["uuid"]:
        return row["uuid"]
    new_uuid = str(uuidlib.uuid4())
    db.execute("UPDATE accounts SET uuid = ? WHERE id = ?", (new_uuid, row["id"]))
    db.commit()
    return new_uuid


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
    account_uuid = str(uuidlib.uuid4())
    db.execute(
        "INSERT INTO accounts (username, password, token, uuid) VALUES (?, ?, ?, ?)",
        (username, hash_password(password), token, account_uuid),
    )
    db.commit()

    return jsonify({"token": token, "username": username, "uuid": account_uuid}), 201


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
    account_uuid = ensure_uuid(db, row)

    return jsonify({"token": token, "username": username, "uuid": account_uuid}), 200


# ---------------------------------------------------------------------------
# GET /api/verify
# ---------------------------------------------------------------------------
@app.route("/api/verify", methods=["GET"])
def verify():
    row = _get_user_from_token()
    if row is None:
        return jsonify({"error": "invalid or missing token"}), 401
    db = get_db()
    account_uuid = ensure_uuid(db, row)
    return jsonify({"username": row["username"], "uuid": account_uuid}), 200


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
# GET /api/capes — list admin-provided cape names (no images, just the catalog)
# ---------------------------------------------------------------------------
@app.route("/api/capes", methods=["GET"])
def list_capes():
    return jsonify({"capes": available_capes()}), 200


# ---------------------------------------------------------------------------
# GET /api/capes/<name> — serve one cape's image
# ---------------------------------------------------------------------------
@app.route("/api/capes/<name>", methods=["GET"])
def get_cape_image(name):
    # Only serve names from the folder listing itself: cape files keep their
    # original names (spaces, long ids), and this also rules out any path
    # tricks since the name must exactly match an existing file.
    if name not in available_capes():
        return jsonify({"error": "no such cape"}), 404
    cape_path = os.path.join(CAPES_DIR, f"{name}.png")
    return send_file(cape_path, mimetype="image/png")


# ---------------------------------------------------------------------------
# PUT /api/cape — set the authenticated account's selected cape ("" = none)
# ---------------------------------------------------------------------------
@app.route("/api/cape", methods=["PUT"])
def set_cape():
    row = _get_user_from_token()
    if row is None:
        return jsonify({"error": "invalid or missing token"}), 401

    data = request.get_json(silent=True) or {}
    cape = data.get("cape", "").strip()

    if cape and cape not in available_capes():
        return jsonify({"error": "no such cape"}), 400

    db = get_db()
    db.execute("UPDATE accounts SET cape = ? WHERE id = ?", (cape, row["id"]))
    db.commit()

    return jsonify({"cape": cape}), 200


# ---------------------------------------------------------------------------
# GET /api/cape/<username> — which cape (if any) a player has selected.
# Public/unauthenticated: other players need this to render your cape.
# ---------------------------------------------------------------------------
@app.route("/api/cape/<username>", methods=["GET"])
def get_player_cape(username):
    db = get_db()
    row = db.execute(
        "SELECT cape FROM accounts WHERE username = ?", (username,)
    ).fetchone()
    return jsonify({"cape": row["cape"] if row and row["cape"] else ""}), 200


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
