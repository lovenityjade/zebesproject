import io
import json
import sqlite3
import tempfile
import unittest
import uuid
from pathlib import Path
from app import Receiver

class Integration(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.path = str(Path(self.tmp.name)/'runs.sqlite')
        self.token = 'test-only-'+'x'*40
        self.app = Receiver(self.path,self.token)
        self.value = dict(schema=1,run_id=str(uuid.uuid4()),ruleset='zebes-v1',mode='vanilla',
                          category='noqol',igt_frames=216000,real_ms=3700000,eligible=True,invalid_reason=0)
    def tearDown(self): self.tmp.cleanup()
    def call(self, value=None, **overrides):
        raw = json.dumps(self.value if value is None else value).encode()
        env = dict(PATH_INFO='/v1/runs',REQUEST_METHOD='POST',CONTENT_TYPE='application/json',
                   CONTENT_LENGTH=str(len(raw)),HTTP_AUTHORIZATION='Bearer '+self.token)
        env['wsgi.input']=io.BytesIO(raw);env.update(overrides)
        status=[]
        body = b''.join(self.app(env,lambda s,h:status.append(s)))
        return int(status[0][:3]),json.loads(body)
    def test_pending_idempotency_conflict_and_persistence(self):
        self.assertEqual(self.call()[0],202)
        self.app=Receiver(self.path,self.token)
        status,result=self.call();self.assertEqual(status,200);self.assertTrue(result['duplicate'])
        self.value['igt_frames']+=1;self.assertEqual(self.call()[0],409)
        with sqlite3.connect(self.path) as db:
            self.assertEqual(db.execute('SELECT COUNT(*) FROM runs').fetchone()[0],1)
            self.assertEqual(db.execute('SELECT COUNT(*) FROM verified_rankings').fetchone()[0],0)
            with self.assertRaises(sqlite3.IntegrityError):db.execute("UPDATE runs SET status='verified'")
    def test_auth_and_limits(self):
        self.assertEqual(self.call(HTTP_AUTHORIZATION='')[0],401)
        self.assertEqual(self.call(CONTENT_LENGTH='9000')[0],413)
        self.assertEqual(self.call(CONTENT_LENGTH='')[0],411)
        self.assertEqual(self.call(CONTENT_TYPE='text/plain')[0],415)
        self.assertEqual(self.call(REQUEST_METHOD='GET')[0],405)
    def test_categories_and_bad_records(self):
        for mode in ('vanilla','ngplus'):
            for cat in ('noqol','qol'):
                self.value.update(run_id=str(uuid.uuid4()),mode=mode,category=cat)
                self.assertEqual(self.call()[0],202)
        for changes in ({'igt_frames':True},{'igt_frames':0},{'real_ms':-1},{'eligible':1},
                        {'category':'anything'},{'schema':2},{'invalid_reason':1},
                        {'run_id':'../file'},{'mode':[]},{'ruleset':None},{'extra':'value'}):
            self.assertEqual(self.call(dict(self.value,**changes))[0],422,changes)
    def test_health(self):self.assertEqual(self.call(PATH_INFO='/health',REQUEST_METHOD='GET')[0],200)

if __name__=='__main__':unittest.main()
