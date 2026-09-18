"""Read-only public deployment verification, plus rejected empty POSTs only."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
base='https://thelovenityjade.me'
report={}
with tempfile.TemporaryDirectory() as tmp:
    def request(path, data=None, origin=None):
        header=Path(tmp)/'headers';body=Path(tmp)/'body'
        args=['curl','-sS','-L','--max-time','25','-D',str(header),'-o',str(body),'-w','%{http_code}',base+path]
        if data is not None:
            args+=['-H','Content-Type: application/json','-H','Origin: '+origin,'--data',data]
        status=int(subprocess.check_output(args,text=True))
        return status,body.read_bytes(),header.read_text()
    for path in ['/','/tdng/','/zebes','/zebes/','/zebes/site.js?v=1',
                 '/zebes/assets/logo.webp','/zebes/sitemap.xml',
                 '/zebes/api/leaderboard?mode=vanilla&category=noqol',
                 '/zebes/api/leaderboard?mode=vanilla&category=qol',
                 '/zebes/api/leaderboard?mode=ngplus&category=noqol',
                 '/zebes/api/leaderboard?mode=ngplus&category=qol',
                 '/zebes/api/v1/runs','/zebes/api/health','/zebes/app.py','/zebes/runs.sqlite']:
        status,body,headers=request(path)
        expected=404 if path in ['/zebes/api/v1/runs','/zebes/api/health','/zebes/app.py','/zebes/runs.sqlite'] else 200
        assert status==expected,(path,status)
        report[path]={'status':status}
        if '/api/leaderboard?' in path:
            value=json.loads(body);assert value['runs']==[] and value['total']==0
            report[path]['data']=value
        if path=='/zebes/':
            assert 'content-security-policy:' in headers.lower()
            assert hashlib.sha256(body).digest()==hashlib.sha256((root/'public/index.html').read_bytes()).digest()
            report[path]['source_hash_matches']=True
    status,body,_=request('/zebes/api/submissions','{}',base)
    assert status==422 and 'error' in json.loads(body),(status,body)
    report['invalid_submission']={'status':status,'writes_to_database':False}
    status,body,_=request('/zebes/api/submissions','{}','https://elsewhere.example')
    assert status==403 and 'error' in json.loads(body),(status,body)
    report['foreign_origin']={'status':status}
print(json.dumps(report,indent=2))
