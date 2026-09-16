CREATE TABLE IF NOT EXISTS schema_version(version INTEGER PRIMARY KEY CHECK(version=1));
INSERT OR IGNORE INTO schema_version VALUES(1);
CREATE TABLE IF NOT EXISTS players (
    player_id TEXT PRIMARY KEY,
    discord_id TEXT UNIQUE,
    display_name TEXT NOT NULL,
    created_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS runs (
    run_id TEXT PRIMARY KEY,
    player_id TEXT REFERENCES players(player_id),
    ruleset TEXT NOT NULL CHECK(ruleset='zebes-v1'),
    mode TEXT NOT NULL CHECK(mode IN ('vanilla','ngplus')),
    category TEXT NOT NULL CHECK(category IN ('noqol','qol')),
    igt_frames INTEGER NOT NULL CHECK(igt_frames>0),
    real_ms INTEGER NOT NULL CHECK(real_ms>=0),
    client_eligible INTEGER NOT NULL CHECK(client_eligible IN (0,1)),
    invalid_reason INTEGER NOT NULL,
    payload TEXT NOT NULL,
    payload_sha256 TEXT NOT NULL,
    received_at INTEGER NOT NULL,
    status TEXT NOT NULL CHECK(status IN ('pending','verified','rejected')),
    reviewed_at INTEGER,
    review_note TEXT,
    CHECK(status != 'verified' OR (client_eligible=1 AND player_id IS NOT NULL AND reviewed_at IS NOT NULL))
);
CREATE INDEX IF NOT EXISTS runs_ranking ON runs(ruleset,mode,category,status,igt_frames);
CREATE VIEW IF NOT EXISTS verified_rankings AS
SELECT run_id,player_id,ruleset,mode,category,igt_frames,real_ms FROM runs
WHERE status='verified';
