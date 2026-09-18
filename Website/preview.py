"""Isolated validation server. Loopback only; never deploy this file to production."""
import mimetypes
import os
import sys
from pathlib import Path
from wsgiref.simple_server import make_server
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent/'Server/receiver'))
from app import Receiver

DATA=Path(os.environ.get('ZEBES_PREVIEW_DATA', str(ROOT/'validation-data')))
DATA.mkdir(parents=True,exist_ok=True)
receiver=Receiver(str(DATA/'runs.sqlite'),'isolated-website-preview-token-not-a-production-secret')

def app(env,respond):
    path=env.get('PATH_INFO','')
    if path.startswith('/zebes/api/'):
        env['PATH_INFO']=path.replace('/zebes/api/','/public/',1)
        if env.get('HTTP_ORIGIN')=='http://127.0.0.1:28742':env['HTTP_ORIGIN']='https://thelovenityjade.me'
        return receiver(env,respond)
    if path in ('/zebes','/'):
        respond('302 Found',[('Location','/zebes/')]);return [b'']
    if not path.startswith('/zebes/'):
        respond('404 Not Found',[]);return [b'Not found']
    file=(ROOT/'public'/path.removeprefix('/zebes/')).resolve()
    if not file.is_relative_to(ROOT/'public'):
        respond('404 Not Found',[]);return [b'Not found']
    if file.is_dir():file=file/'index.html'
    if not file.is_file():
        respond('404 Not Found',[]);return [b'Not found']
    body=file.read_bytes();respond('200 OK',[('Content-Type',mimetypes.guess_type(file)[0] or 'application/octet-stream'),('Content-Length',str(len(body)))]);return [body]

if __name__=='__main__':
    with make_server('127.0.0.1',28742,app) as server:
        print('Website preview: http://127.0.0.1:28742/zebes/',flush=True);server.serve_forever()
