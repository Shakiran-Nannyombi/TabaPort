"""SQLite helpers. One file, WAL mode, safe for the reader thread + Flask threads."""
import os
import sqlite3
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.environ.get("TABAPORT_DB", os.path.join(HERE, "data", "tabaport.db"))
SCHEMA_PATH = os.path.join(HERE, "schema.sql")

_lock = threading.RLock()
_conn = None


def connect():
    global _conn
    with _lock:
        if _conn is None:
            os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
            _conn = sqlite3.connect(DB_PATH, check_same_thread=False, isolation_level=None)
            _conn.row_factory = sqlite3.Row
            with open(SCHEMA_PATH, encoding="utf-8") as f:
                _conn.executescript(f.read())
        return _conn


def execute(sql, params=()):
    with _lock:
        return connect().execute(sql, params)


def query(sql, params=()):
    with _lock:
        return [dict(r) for r in connect().execute(sql, params).fetchall()]


def query_one(sql, params=()):
    rows = query(sql, params)
    return rows[0] if rows else None


def transaction():
    """Usage: with db.transaction() as c: c.execute(...)"""
    return _Tx()


class _Tx:
    def __enter__(self):
        _lock.acquire()
        self.c = connect()
        self.c.execute("BEGIN")
        return self.c

    def __exit__(self, exc_type, exc, tb):
        try:
            if exc_type:
                self.c.execute("ROLLBACK")
            else:
                self.c.execute("COMMIT")
        finally:
            _lock.release()


# ---------- config ----------
def cfg(key, default=None):
    row = query_one("SELECT value FROM config WHERE key=?", (key,))
    return row["value"] if row else default


def cfg_float(key, default):
    try:
        return float(cfg(key, default))
    except (TypeError, ValueError):
        return float(default)


def set_cfg(key, value):
    execute("INSERT INTO config(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",
            (key, str(value)))


def all_cfg():
    return {r["key"]: r["value"] for r in query("SELECT key, value FROM config ORDER BY key")}
