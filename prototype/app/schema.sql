-- TabaPort hub schema. Sharif's four tables plus config. SQLite, server-less, offline.
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

-- Node readings as they arrive over serial.
CREATE TABLE IF NOT EXISTS node_reading (
    reading_id          INTEGER PRIMARY KEY AUTOINCREMENT,
    node_id             TEXT    NOT NULL,
    timestamp           DATETIME DEFAULT CURRENT_TIMESTAMP,
    water_level_cm      REAL    NOT NULL,
    rise_rate_cm_per_min REAL,
    filtered_flag       BOOLEAN DEFAULT 0,
    raw_ultrasonic_cm   REAL
);
CREATE INDEX IF NOT EXISTS idx_reading_node_time ON node_reading(node_id, timestamp);

-- One row per road segment. Nullable node_id: a segment may only have human reports.
CREATE TABLE IF NOT EXISTS road_segment_state (
    segment_id   TEXT PRIMARY KEY,
    node_id      TEXT,
    state        TEXT NOT NULL CHECK (state IN ('green','yellow','red','unknown')) DEFAULT 'unknown',
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP,
    source       TEXT CHECK (source IN ('sensor','human_report','dispatcher')) DEFAULT 'sensor',
    notes        TEXT
);

-- An alert is the signal. A job is the response (work order).
CREATE TABLE IF NOT EXISTS alert (
    alert_id     INTEGER PRIMARY KEY AUTOINCREMENT,
    segment_id   TEXT NOT NULL REFERENCES road_segment_state(segment_id),
    node_id      TEXT,
    from_state   TEXT,
    to_state     TEXT NOT NULL,
    level_cm     REAL,
    created_at   DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS job (
    job_id             INTEGER PRIMARY KEY AUTOINCREMENT,
    segment_id         TEXT NOT NULL REFERENCES road_segment_state(segment_id),
    alert_id           INTEGER REFERENCES alert(alert_id),
    action             TEXT,                 -- Joel's wording: Send drainage crew, etc.
    assigned_crew      TEXT,
    status             TEXT NOT NULL CHECK (status IN ('open','acked','en_route','on_site','closed','false_alarm')) DEFAULT 'open',
    created_at         DATETIME DEFAULT CURRENT_TIMESTAMP,
    acked_at           DATETIME,
    closed_at          DATETIME,
    false_alarm_reason TEXT
);
CREATE INDEX IF NOT EXISTS idx_job_status ON job(status);

-- Field reports from boda riders / staff. Phone numbers are hashed at the gateway, never stored raw.
CREATE TABLE IF NOT EXISTS human_report (
    report_id    INTEGER PRIMARY KEY AUTOINCREMENT,
    segment_id   TEXT REFERENCES road_segment_state(segment_id),
    reported_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
    report_type  TEXT CHECK (report_type IN ('blockage','flooding','road_impassable','clear')),
    note         TEXT,
    reporter_hash TEXT
);

-- Key/value config: thresholds, sensor height, node->segment map. Field numbers replace tank numbers here.
CREATE TABLE IF NOT EXISTS config (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

INSERT OR IGNORE INTO config(key, value) VALUES
    ('node_segment.drain_01', 'campus_drain_01'),
    ('h_sensor_cm.drain_01', '50'),       -- probe face -> invert, measured dry on Tue
    ('h_road_cm.drain_01',   '60'),       -- road/path edge -> invert
    ('yellow_cm.drain_01',   '8'),        -- rain-day tank default; set from drain depth on Tue
    ('red_cm.drain_01',      '15'),
    ('unknown_after_s',      '10'),
    ('escalate_after_s',     '180'),
    ('crews', 'Team A - Central,Team B - Kawempe,Team C - Rubaga,Traffic Police');
