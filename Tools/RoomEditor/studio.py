"""Versioned studio metadata and content-addressed, local PNG imports."""
import hashlib,io,re,struct,warnings
from pathlib import Path
from PIL import Image
IMAGE_ID=re.compile(r'^[0-9a-f]{64}$')

def planes(room):
    if room['area']==6:return []
    if room['id']==0x91f8:
        return [dict(id=0,name='Ciel',start=0,end=1168),dict(id=1,name='Montagnes',start=1168,end=1192),dict(id=2,name='Végétation',start=1192,end=8192)]
    # These are presentation planes, not arbitrary new SNES background layers.
    if room['id'] in (0xa59f,0xdd58):return [] # Kraid / Mother Brain BG2 bodies.
    return [dict(id=0,name='Fond BG2',start=0,end=8192)]

def integer(value,lo,hi):
    if type(value) is not int or not lo<=value<=hi:raise ValueError(f'Entier attendu entre {lo} et {hi}.')
    return value

def image_id(value):
    if value is None:return None
    if not isinstance(value,str) or not IMAGE_ID.fullmatch(value):raise ValueError('Référence PNG invalide.')
    return value

def store_image(folder,data,kind):
    if kind not in ('tileset','background') or not data or len(data)>8_000_000:raise ValueError('PNG trop volumineux ou type invalide.')
    with warnings.catch_warnings():
        warnings.simplefilter('error',Image.DecompressionBombWarning)
        try:
            with Image.open(io.BytesIO(data)) as source:
                if source.format!='PNG':raise ValueError('Un fichier PNG est requis.')
                w,h=source.size
                if not 1<=w<=4096 or not 1<=h<=4096 or w*h>4194304:raise ValueError('Maximum : 4096 pixels par côté et 4 millions de pixels.')
                if kind=='tileset' and (w%16 or h%16 or w*h>262144):raise ValueError('Tileset : dimensions multiples de 16, maximum 1024 tiles.')
                image=source.convert('RGBA')
        except (OSError,Image.DecompressionBombError,Image.DecompressionBombWarning) as error:raise ValueError('PNG illisible ou trop grand.') from error
    stream=io.BytesIO();image.save(stream,format='PNG');png=stream.getvalue();identity=hashlib.sha256(png).hexdigest()
    folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
    # Canonical images contain only decoded pixels: no source metadata or paths.
    for suffix,payload in [('.png',png),('.bgra',b'ZBG1'+struct.pack('<II',w,h)+image.tobytes('raw','BGRA'))]:
        dest=folder/(identity+suffix)
        if not dest.exists():
            temp=dest.with_suffix(suffix+'.tmp');temp.write_bytes(payload);temp.replace(dest)
    return dict(id=identity,width=w,height=h,url='/images/'+identity+'.png')

def validate_studio(data,room):
    allowed={'room','state','cells','parallax','tilesetImage','tilesetCount','events'}
    if set(data)-allowed:raise ValueError('Propriété inconnue; les collisions restent en lecture seule.')
    result=dict(tilesetCount=integer(data.get('tilesetCount',0),0,1024),tilesetImage=image_id(data.get('tilesetImage')),parallax=[],events=[])
    seen=set();available={p['id'] for p in planes(room)}|{-1}
    for row in data.get('parallax',[]):
        if not isinstance(row,dict) or set(row)-{'id','image','speedX','speedY','offsetX','offsetY','repeatX','repeatY'}:raise ValueError('Plan invalide.')
        ident=integer(row.get('id'),-1,2)
        if ident not in available or ident in seen or not planes(room):raise ValueError('Plan absent ou répété.')
        seen.add(ident)
        p=dict(id=ident,image=image_id(row.get('image')))
        for k in ('speedX','speedY'):p[k]=integer(row.get(k,100),0,200)
        for k in ('offsetX','offsetY'):p[k]=integer(row.get(k,0),-8192,8192)
        for k in ('repeatX','repeatY'):
            p[k]=row.get(k,True)
            if type(p[k]) is not bool:raise ValueError('Répétition invalide.')
        result['parallax'].append(p)
    if -1 in seen and len(seen)>1:raise ValueError('Choisir le fond complet ou les plans séparés.')
    events=data.get('events',[])
    if not isinstance(events,list) or len(events)>1024:raise ValueError('Trop de zones.')
    for event in events:
        if not isinstance(event,dict) or set(event)!={'x','y','width','height','note'}:raise ValueError('Zone invalide.')
        e={k:integer(event[k],-128,511) for k in ('x','y')}
        for k in ('width','height'):e[k]=integer(event[k],1,640)
        if e['x']+e['width']>512 or e['y']+e['height']>512:raise ValueError('Zone hors limites.')
        if not isinstance(event['note'],str) or not 1<=len(event['note'].strip())<=2000:raise ValueError('Décrire la zone (1 à 2000 caractères).')
        e['note']=event['note'].strip();result['events'].append(e)
    return result

def validate_images(patch,folder):
    ids={p['image'] for p in patch.get('parallax',[]) if p.get('image')}
    atlas=patch.get('tilesetImage')
    if atlas:ids.add(atlas)
    count=0
    for ident in ids:
        p=Path(folder)/(ident+'.png');raw=Path(folder)/(ident+'.bgra')
        if not p.is_file() or not raw.is_file():raise ValueError('Un PNG manque; importer son image avant d’enregistrer.')
        if atlas==ident:
            with Image.open(p) as img:
                w,h=img.size
                if w%16 or h%16 or w*h>262144:raise ValueError('Atlas personnalisé invalide.')
                count=(w//16)*(h//16)
    if patch.get('tilesetCount',0)>count:raise ValueError('Nombre de tiles incohérent.')
    for cell in patch['cells']:
        if cell.get('custom',False) and (not atlas or (cell['tile']&1023)>=count):raise ValueError('Tile personnalisée absente de l’atlas.')
