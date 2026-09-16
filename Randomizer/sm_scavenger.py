"""Exact source-written Scavenger order; no ROM patch or inferred item order."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def validate(data):
    catalog=json.loads((ROOT/'native_scavenger.json').read_text())
    if not isinstance(data,dict) or data.get('schema')!=1 or data.get('catalogSha256')!=catalog['sha256']:
        raise ValueError('Unsupported native Scavenger contract')
    words=data.get('words')
    known={loc['id']<<8|loc['hud']:loc['name'] for loc in catalog['locations']}
    if not isinstance(words,list) or not 1<=len(words)<=17 or any(type(w) is not int or w not in known for w in words) or len(set(words))!=len(words):
        raise ValueError('Invalid native Scavenger order')
    if data.get('locations')!=[known[w] for w in words]:
        raise ValueError('Scavenger names disagree with source location identities')
    return data

def capture(patcher):
    if patcher.settings['majorsSplit']!='Scavenger':return None
    from rom.addresses import Addresses
    catalog=json.loads((ROOT/'native_scavenger.json').read_text())
    start=Addresses.getOne('scavengerOrder')
    data=patcher.romFile.data
    # writeSplitLocs is the authoritative order writer. Every word, including
    # the terminal HUD row and all padding, must be present in its output.
    table=[data[start+i*2]|data[start+i*2+1]<<8 for i in range(19)]
    try:end=table.index(catalog['huntOver'])
    except ValueError:raise ValueError('Missing Scavenger end marker') from None
    if any(w!=catalog['terminator'] for w in table[end+1:]):
        raise ValueError('Invalid Scavenger table padding')
    words=table[:end]
    result=dict(schema=1,catalogSha256=catalog['sha256'],words=words,
                locations=[il.Location.Name for il in patcher.settings['progItemLocs']])
    return validate(result)
