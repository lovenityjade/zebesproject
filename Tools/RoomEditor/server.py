#!/usr/bin/env python3
"""Independent room-decoration editor. No game process or ROM writes."""
import argparse
import json
import os
import secrets
import shutil
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse,parse_qs
from assets import Assets,ROOT
from studio import store_image, validate_images, IMAGE_ID

class EditorServer(ThreadingHTTPServer):
    def __init__(self,address,patch_dir):
        super().__init__(address,Handler)
        self.assets=Assets();self.token=secrets.token_urlsafe(24)
        self.patch_dir=Path(patch_dir).resolve();self.patch_dir.mkdir(parents=True,exist_ok=True)
        self.write_lock=threading.Lock()

class Handler(BaseHTTPRequestHandler):
    def reply(self,data,status=200,kind='application/json'):
        if kind=='application/json':data=json.dumps(data,ensure_ascii=False).encode()
        self.send_response(status);self.send_header('Content-Type',kind)
        self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store')
        self.send_header('X-Content-Type-Options','nosniff');self.end_headers();self.wfile.write(data)
    def ids(self,query):
        return int(query['room'][0]),int(query['state'][0]) if 'state' in query else None
    def patch_path(self,room,state):return self.server.patch_dir/f'{room:04x}-{state:04x}.json'
    def do_GET(self):
        route=urlparse(self.path);query=parse_qs(route.query)
        try:
            if route.path in ('/','/index.html'):
                page=(Path(__file__).parent/'index.html').read_text().replace('__EDITOR_TOKEN__',self.server.token)
                return self.reply(page.encode(),kind='text/html; charset=utf-8')
            if route.path.startswith('/images/'):
                name=route.path[8:]
                if not name.endswith('.png') or not IMAGE_ID.fullmatch(name[:-4]):raise ValueError('Image inconnue.')
                return self.reply((self.server.patch_dir/'images'/name).read_bytes(),kind='image/png')
            if route.path=='/api/tilesets':return self.reply([dict(id=i,name=f'Tileset {i:02X}') for i in range(29)])
            if route.path in ('/editor.js','/studio.js','/editor.css'):
                file=Path(__file__).parent/route.path[1:]
                return self.reply(file.read_bytes(),kind='text/javascript' if file.suffix=='.js' else 'text/css')
            if route.path=='/api/rooms':return self.reply(list(self.server.assets.rooms.values()))
            if route.path=='/api/room':return self.reply(self.server.assets.room(*self.ids(query)))
            if route.path=='/api/background':return self.reply(self.server.assets.background_preview(*self.ids(query)),kind='image/png')
            if route.path=='/api/atlas':return self.reply(self.server.assets.tileset(int(query['tileset'][0]))['png'],kind='image/png')
            if route.path=='/api/patch':
                room=self.server.assets.room(*self.ids(query));path=self.patch_path(room['id'],room['state'])
                if path.exists():return self.reply(json.loads(path.read_text()))
                return self.reply(self.server.assets.validate_patch(dict(room=room['id'],state=room['state'],cells=[])))
            return self.reply({'error':'Page inconnue.'},404)
        except (ValueError,KeyError,TypeError,IndexError,OSError) as error:return self.reply({'error':str(error)},400)
    def do_POST(self):
        route=urlparse(self.path)
        if route.path not in ('/api/patch','/api/image'):return self.reply({'error':'Route inconnue.'},404)
        if self.headers.get('X-Editor-Token')!=self.server.token:return self.reply({'error':'Session éditeur requise.'},403)
        try:
            length=int(self.headers.get('Content-Length','0'))
            if not 0<length<=8_000_000:raise ValueError('Fichier trop volumineux.')
            body=self.rfile.read(length)
            if route.path=='/api/image':
                kind=parse_qs(route.query).get('kind',['background'])[0]
                with self.server.write_lock:result=store_image(self.server.patch_dir/'images',body,kind)
                return self.reply(result)
            patch=self.server.assets.validate_patch(json.loads(body))
            validate_images(patch,self.server.patch_dir/'images')
            path=self.patch_path(patch['room'],patch['state'])
            with self.server.write_lock:
                if path.exists():
                    history=self.server.patch_dir/'.history';history.mkdir(exist_ok=True)
                    shutil.copy2(path,history/f'{path.stem}-{time.time_ns()}.json')
                temporary=path.with_suffix('.json.tmp')
                temporary.write_text(json.dumps(patch,indent=2)+'\n');os.replace(temporary,path)
            self.reply(dict(saved=True,cells=len(patch['cells']),file=path.name))
        except (ValueError,KeyError,TypeError,IndexError,OSError) as error:self.reply({'error':str(error)},400)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=8766)
    parser.add_argument('--no-browser',action='store_true')
    parser.add_argument('--patch-dir',type=Path,default=ROOT/'Config/RoomDecorations')
    parser.add_argument('--token-file',type=Path,help='Reuse a private session token when restarting the local editor.')
    args=parser.parse_args();server=EditorServer(('127.0.0.1',args.port),args.patch_dir)
    if args.token_file:
        token=args.token_file.read_text().strip()
        if not 24<=len(token)<=128 or not all(c.isascii() and (c.isalnum() or c in '-_') for c in token):
            parser.error('Invalid editor session token.')
        server.token=token
    url=f'http://127.0.0.1:{server.server_port}'
    print(f'Éditeur externe : {url}\nRetouches : {server.patch_dir}',flush=True)
    if not args.no_browser:webbrowser.open(url)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()
