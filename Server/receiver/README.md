# Run receiver foundation

A private receiving service for future in-game and website leaderboards. The
native game writes completed Vanilla/NG+ speedrun records locally; it does not
upload yet. Discord identity, player accounts, result verification and public
rankings remain future integration work, as requested.

## Deployed service

- Host: SSH alias `nexus-vps`, verified hostname `nexus`.
- Unit: `zebes-receiver.service`; dedicated `zebes-receiver` system account.
- Code/venv: `/opt/zebes-receiver`.
- SQLite database: `/var/lib/zebes-receiver/runs.sqlite`.
- Private environment/token: `/etc/zebes-receiver.env` (root, mode 0600).
- Listen: **127.0.0.1:28740**, not publicly exposed; no Caddy changes.
- Health: `GET /health`.
- Ingestion: `POST /v1/runs`, `application/json`, private Bearer token.
- Body limit: 8 KiB. The token belongs only to server-side administration or a
  future trusted authentication gateway, never a shipped game binary.

`requirements.txt` pins and hashes Gunicorn. The service uses a single worker
with two threads, an independent connection per request, SQLite WAL, foreign keys
and a five-second lock timeout. References: [Gunicorn settings](https://docs.gunicorn.org/en/stable/settings.html),
[SQLite WAL](https://www.sqlite.org/wal.html), [foreign keys](https://www.sqlite.org/foreignkeys.html).

## Record contract (version 1)

```json
{"schema":1,"run_id":"f0aa0ab1-bacf-4b80-b407-b2d246dfc38f","ruleset":"zebes-v1","mode":"vanilla","category":"noqol","igt_frames":216000,"real_ms":3700000,"eligible":true,"invalid_reason":0}
```

Modes: `vanilla`, `ngplus`. Categories: `noqol`, `qol`. Ranking uses **native
in-game frames at 60 Hz**; real time is supplementary. Unknown/duplicate fields,
invalid types, unsupported rules, inconsistent eligibility and invalid IDs are
rejected. Exact retries return the existing result; changed content under an
existing UUID returns 409. Database uniqueness makes this atomic.

Every accepted record is **pending**, even when the client claims eligibility.
The API cannot mark a result verified. `verified_rankings` only selects records
with a linked player and reviewed status; no public query endpoint exists yet.
The nullable player link and separate players table prepare future Discord login.
Client-side rule enforcement and local checksums are not an anti-cheat system.

## Operations and validation

`sudo systemctl status zebes-receiver` and `sudo journalctl -u zebes-receiver`.
Back up with SQLite's online backup API, not a copy of only the main file while
WAL is active. Restore into a stopped service; preserve its account permissions.
Future schema changes require explicit migrations; `schema_version` is version 1.
Do not install this service over unrelated Nexus applications.

Tests: `python3 -m unittest discover -s Server/receiver -v` from the repository.
Live HTTP validation on Nexus covered authentication rejection, accepted pending
record, idempotent retry, conflicting UUID, persistence and service restart.
Only the test's own UUID was removed afterward; the deployed database is empty.
