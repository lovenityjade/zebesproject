"""Private ingestion boundary. Client claims are never verified leaderboard results."""
import hashlib
import hmac
import json
import os
import sqlite3
import time
import uuid
from pathlib import Path

SCHEMA = Path(__file__).with_name('schema.sql').read_text()
MAX_BODY = 8192


def connect(path):
    db = sqlite3.connect(path, timeout=5)
    db.execute('PRAGMA foreign_keys=ON')
    db.execute('PRAGMA journal_mode=WAL')
    return db


def initialize(path):
    with connect(path) as db:
        db.executescript(SCHEMA)


def validate(value):
    fields = {'schema', 'run_id', 'ruleset', 'mode', 'category', 'igt_frames',
              'real_ms', 'eligible', 'invalid_reason'}
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError('Expected exactly the version 1 run fields')
    if type(value['schema']) is not int or value['schema'] != 1:
        raise ValueError('Unsupported schema')
    if not isinstance(value['run_id'], str):
        raise ValueError('Invalid run_id')
    run_id = uuid.UUID(value['run_id'])
    if run_id.version != 4 or str(run_id) != value['run_id']:
        raise ValueError('run_id must be a canonical UUID v4')
    if value['ruleset'] != 'zebes-v1':
        raise ValueError('Unknown ruleset')
    if value['mode'] not in ('vanilla', 'ngplus') or value['category'] not in ('noqol', 'qol'):
        raise ValueError('Unknown mode or category')
    for key, maximum in [('igt_frames', 99*3600*60+59*60*60+59*60+59),
                         ('real_ms', 365*24*3600*1000), ('invalid_reason', 65535)]:
        if type(value[key]) is not int or not 0 <= value[key] <= maximum:
            raise ValueError('Invalid ' + key)
    if value['igt_frames'] == 0 or type(value['eligible']) is not bool:
        raise ValueError('A completed run needs a positive in-game time and boolean eligibility')
    if value['eligible'] and value['invalid_reason'] != 0:
        raise ValueError('Eligible run has an invalidation reason')
    return json.dumps(value, sort_keys=True, separators=(',', ':'))


class Receiver:
    def __init__(self, path, token):
        if len(token) < 32:
            raise ValueError('A private receiver token of at least 32 characters is required')
        self.path, self.token = path, token
        initialize(path)

    def __call__(self, env, respond):
        def reply(status, value):
            body = json.dumps(value, separators=(',', ':')).encode()
            respond(status, [('Content-Type', 'application/json'),
                             ('Content-Length', str(len(body))), ('Cache-Control', 'no-store')])
            return [body]
        if env.get('PATH_INFO') == '/health' and env['REQUEST_METHOD'] == 'GET':
            try:
                with connect(self.path) as db:
                    db.execute('SELECT 1 FROM schema_version').fetchone()
            except sqlite3.Error:
                return reply('503 Service Unavailable', {'ok': False})
            return reply('200 OK', {'ok': True, 'schema': 1})
        if env.get('PATH_INFO') != '/v1/runs':
            return reply('404 Not Found', {'error': 'Not found'})
        if env['REQUEST_METHOD'] != 'POST':
            return reply('405 Method Not Allowed', {'error': 'POST required'})
        supplied = env.get('HTTP_AUTHORIZATION', '').encode()
        if not hmac.compare_digest(supplied, ('Bearer ' + self.token).encode()):
            return reply('401 Unauthorized', {'error': 'Receiver authentication required'})
        if env.get('CONTENT_TYPE', '').split(';')[0] != 'application/json':
            return reply('415 Unsupported Media Type', {'error': 'application/json required'})
        try:
            size = int(env.get('CONTENT_LENGTH', ''))
        except ValueError:
            return reply('411 Length Required', {'error': 'Content-Length required'})
        if size < 1 or size > MAX_BODY:
            return reply('413 Content Too Large', {'error': 'Invalid body size'})
        try:
            raw = env['wsgi.input'].read(size)
            if len(raw) != size:
                raise ValueError('Incomplete body')
            # Reject duplicate keys rather than allowing ambiguous signed data later.
            def unique(pairs):
                value = {}
                for key, item in pairs:
                    if key in value:
                        raise ValueError('Duplicate field')
                    value[key] = item
                return value
            value = json.loads(raw, object_pairs_hook=unique)
            canonical = validate(value)
        except (ValueError, TypeError, UnicodeError, RecursionError, AttributeError):
            return reply('422 Unprocessable Entity', {'error': 'Invalid run record'})
        digest = hashlib.sha256(canonical.encode()).hexdigest()
        try:
            with connect(self.path) as db:
                db.execute('BEGIN IMMEDIATE')
                existing = db.execute('SELECT payload_sha256,status FROM runs WHERE run_id=?',
                                      (value['run_id'],)).fetchone()
                if existing:
                    if existing[0] != digest:
                        return reply('409 Conflict', {'error': 'run_id already has different contents'})
                    return reply('200 OK', {'run_id': value['run_id'], 'status': existing[1], 'duplicate': True})
                db.execute('''INSERT INTO runs
                    (run_id,ruleset,mode,category,igt_frames,real_ms,client_eligible,
                     invalid_reason,payload,payload_sha256,received_at,status)
                    VALUES (?,?,?,?,?,?,?,?,?,?,?,'pending')''',
                    (value['run_id'],value['ruleset'],value['mode'],value['category'],
                     value['igt_frames'],value['real_ms'],int(value['eligible']),
                     value['invalid_reason'],canonical,digest,int(time.time())))
        except sqlite3.Error:
            return reply('503 Service Unavailable', {'error': 'Storage unavailable; retry later'})
        return reply('202 Accepted', {'run_id': value['run_id'], 'status': 'pending', 'duplicate': False})


def create_app():
    return Receiver(os.environ['ZEBES_DB'], os.environ['ZEBES_RECEIVER_TOKEN'])
