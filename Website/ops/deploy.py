"""Run as root on nexus-vps with a hash-verified isolated upload directory.

Scope: /zebes static site, existing Zebes receiver, and its exact Caddy routes.
Preserves unrelated vhosts and takes an online SQLite backup before mutation.
"""
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import urllib.request

stage=Path(sys.argv[1]).resolve()
assert subprocess.check_output(['hostname'],text=True).strip()=='nexus'
assert os.geteuid()==0
manifest=json.loads((stage/'Website/build-manifest.json').read_text())
site=stage/'Website/dist'
assert set(manifest['files'])=={str(p.relative_to(site)) for p in site.rglob('*') if p.is_file()}
for name,digest in manifest['files'].items():
    assert hashlib.sha256((site/name).read_bytes()).hexdigest()==digest,name

caddy=Path('/etc/caddy/conf.d/public-sites.caddy')
old_caddy=caddy.read_text()
assert hashlib.sha256(caddy.read_bytes()).hexdigest()==(stage/'caddy-before.sha256').read_text().strip(), 'Caddy changed; inspect before applying.'
assert old_caddy.count('thelovenityjade.me {')==1
assert '/zebes/api/' not in old_caddy, 'Zebes routes already exist; inspect before updating.'
receiver=Path('/opt/zebes-receiver')
assert hashlib.sha256((receiver/'app.py').read_bytes()).hexdigest()=='c44ac931ade1a68b7702fbae6da3e0c12ffb757b26175376e7c1064ec501499b', 'Receiver changed; inspect before applying.'
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
backup=Path('/var/backups/zebes-website')/stamp
backup.mkdir(parents=True,mode=0o700)
shutil.copy2(caddy,backup/'public-sites.caddy')
shutil.copytree(receiver,backup/'receiver',ignore=shutil.ignore_patterns('venv','__pycache__'))
db=sqlite3.connect('/var/lib/zebes-receiver/runs.sqlite')
copy=sqlite3.connect(backup/'runs.sqlite')
db.backup(copy);copy.close();db.close()
os.chmod(backup/'runs.sqlite',0o600)
unchanged={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in [
    Path('/var/www/thelovenityjade.me/index.html'),Path('/var/www/thelovenityjade.me/tdng/index.html'),
    Path('/etc/caddy/conf.d/nexus-mhf-auth.caddy')]}
destination=Path('/var/www/thelovenityjade.me/zebes')
new_site=destination.parent/('.zebes-stage-'+stamp)
shutil.copytree(site,new_site)
for path in [new_site,*new_site.rglob('*')]:
    os.chmod(path,0o755 if path.is_dir() else 0o644)
subprocess.run(['restorecon','-RF',str(new_site)],check=True)

routes='''thelovenityjade.me {
    # The Zebes Project: expose only the public website boundary.
    @zebes_api path /zebes/api/leaderboard /zebes/api/submissions
    handle @zebes_api {
        request_body {
            max_size 10KB
        }
        uri path_regexp ^/zebes/api/ /public/
        reverse_proxy 127.0.0.1:28740 {
            header_up -Authorization
            header_up X-Zebes-Client-IP {remote_host}
        }
    }
    handle /zebes/api/* {
        respond "Not found" 404
    }
    header /zebes/* {
        Content-Security-Policy "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self'; font-src 'self'; connect-src 'self'; base-uri 'none'; form-action 'self'; object-src 'none'; frame-ancestors 'self'; upgrade-insecure-requests"
        Permissions-Policy "camera=(), microphone=(), geolocation=()"
        Cross-Origin-Resource-Policy "same-origin"
        Cache-Control "no-cache"
    }
    header /zebes/assets/* Cache-Control "public, max-age=604800"
'''
new_caddy=old_caddy.replace('thelovenityjade.me {\n',routes,1)
replaced_site=False
try:
    for name in ['app.py','schema.sql','public_api.py','review.py']:
        source=stage/'Server/receiver'/name
        tmp=receiver/(name+'.website-new')
        shutil.copyfile(source,tmp);os.chmod(tmp,0o644);tmp.replace(receiver/name)
    subprocess.run(['systemctl','restart','zebes-receiver'],check=True)
    import time
    for attempt in range(30):
        try:
            health=json.load(urllib.request.urlopen('http://127.0.0.1:28740/health',timeout=2))
            assert health['ok'];break
        except Exception:
            if attempt==29:raise
            time.sleep(.2)
    board=json.load(urllib.request.urlopen('http://127.0.0.1:28740/public/leaderboard',timeout=5))
    assert board['runs']==[] and board['total']==0, 'Unexpected production scores; inspect instead of overwriting.'
    caddy.write_text(new_caddy)
    subprocess.run(['caddy','fmt','--overwrite',str(caddy)],check=True)
    subprocess.run(['caddy','validate','--config','/etc/caddy/Caddyfile'],check=True)
    if destination.exists():destination.rename(backup/'previous-site')
    new_site.rename(destination);replaced_site=True
    subprocess.run(['restorecon','-RF',str(destination)],check=True)
    subprocess.run(['systemctl','reload','caddy'],check=True)
    for path,digest in unchanged.items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest,path
    for name,digest in manifest['files'].items():assert hashlib.sha256((destination/name).read_bytes()).hexdigest()==digest,name
except Exception:
    shutil.copy2(backup/'public-sites.caddy',caddy)
    for name in ['app.py','schema.sql']:
        shutil.copy2(backup/'receiver'/name,receiver/name)
    if replaced_site:
        destination.rename(backup/'failed-site')
        if (backup/'previous-site').exists():(backup/'previous-site').rename(destination)
    subprocess.run(['systemctl','restart','zebes-receiver'],check=False)
    subprocess.run(['caddy','validate','--config','/etc/caddy/Caddyfile'],check=False)
    subprocess.run(['systemctl','reload','caddy'],check=False)
    raise
receipt={'deployed_at':stamp,'url':'https://thelovenityjade.me/zebes/','backup':str(backup),
         'public_files':len(manifest['files']),'source_files_sha256':manifest['files'],
         'preserved':unchanged,'production_verified_runs':board['total'],'rom_or_save_files':False}
(backup/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt))
