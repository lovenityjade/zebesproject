"""Effective escape clocks from the unmodified VARIA writer.

This sub-contract does not authorize activation without native routing, room
patches and the corresponding solver/tracker completion semantics.
"""
import json
from pathlib import Path

def routing_catalog():return json.loads((Path(__file__).resolve().parent/'native_escape.json').read_text())

def validate_routing(data):
 c=routing_catalog()
 if not isinstance(data,dict) or data.get('schema')!=1 or data.get('catalogSha256')!=c['sha256']:raise ValueError('Unsupported native escape routing')
 pairs=data.get('pairs');animals=data.get('animals')
 if not isinstance(pairs,list) or not 6<=len(pairs)<=16 or any(not isinstance(p,list) or len(p)!=2 or any(type(n) is not int or not 0<=n<len(c['accessPoints']) for n in p) for p in pairs):raise ValueError('Invalid ordered escape connections')
 if type(animals) is not int or not 0<=animals<=4:raise ValueError('Invalid escape animal door')
 doors={c['accessPoints'][i]['door'] for i,j in pairs}
 if not {0xadac,0xadc4,0xaddc,0xadf4,0xae00}<=doors:raise ValueError('Incomplete animal escape cycle')
 return data

def capture_routing(patcher):
 s=patcher.settings
 if s.get('escapeAttr') is None:return None
 c=routing_catalog();ids={a['name']:a['id'] for a in c['accessPoints']};pairs=[]
 for actual in s['doors']:
  src,dst=actual['transition']
  if not src.Escape and not dst.Escape:continue
  if not src.Escape or not dst.Escape or src.Name not in ids or dst.Name not in ids:raise ValueError('Unknown escape endpoint')
  i,j=ids[src.Name],ids[dst.Name];expected=c['connections'][i][j]
  door=patcher.symbols.getAddress(actual['DoorPtrSym'])&65535 if 'DoorPtrSym' in actual else actual['DoorPtr']
  if door!=expected['door']:raise ValueError('Escape physical door drift')
  for a,b in [('room','RoomPtr'),('direction','direction'),('flag','bitFlag'),('asm','doorAsmPtr'),('distance','distanceToSpawn')]:
   if expected[a]!=actual[b]:raise ValueError('Escape descriptor drift: '+src.Name+' '+a)
  if list(actual['cap'])!=expected['cap'] or list(actual['screen'])!=expected['screen']:raise ValueError('Escape geometry drift')
  if actual.get('exitAsm')!=('rando_escape_common_setup_next_escape' if expected['cycle'] else None):raise ValueError('Escape exit routine drift')
  if ('SamusX' in actual)!=expected['incompatible'] or (expected['incompatible'] and (actual['SamusX']!=expected['x'] or actual['SamusY']!=expected['y'])):raise ValueError('Escape spawn drift')
  pairs.append([i,j])
 animal=s['escapeAttr']['Animals']
 kind=0 if animal is None else {'Green Brinstar Main Shaft Top Left':1,'Business Center Mid Left':2,'Crab Hole Bottom Right':3}.get(animal,4)
 return validate_routing(dict(schema=1,catalogSha256=c['sha256'],pairs=pairs,animals=kind))

def escape_pairs(routing):
 c=routing_catalog();validate_routing(routing)
 pairs=[p for p in routing['pairs'] if p[0]==0]
 if len(pairs)!=1:raise ValueError('Missing or repeated Tourian escape source')
 return [[c['accessPoints'][i]['name'],c['accessPoints'][j]['name']] for i,j in pairs]

def validate(data):
 if not isinstance(data,dict) or data.get('schema')!=1:raise ValueError('Unsupported escape contract')
 validate_clock(data.get('clock'));validate_routing(data.get('routing'));escape_pairs(data['routing'])
 return data

def capture(patcher):
 if patcher.settings.get('escapeAttr') is None:return None
 return validate(dict(schema=1,clock=capture_clock(patcher),routing=capture_routing(patcher)))

def map_offsets(minimizer=None):
 offsets=[0,0,4,0,1,0,1,0,0,1,0,0]
 return [n if minimizer is None or i in minimizer['regions'] else 0 for i,n in enumerate(offsets)]

def validate_clock(data):
 if not isinstance(data,dict) or data.get('schema')!=1:raise ValueError('Unsupported escape clock')
 flags=data.get('flags')
 if type(flags) is not int or flags<0 or flags&~7 or flags&3==3:raise ValueError('Invalid escape flags')
 def bcd(n):return type(n) is int and 0<=n<=0x9959 and all(((n>>s)&15)<limit for s,limit in [(0,10),(4,6),(8,10),(12,10)])
 timer=data.get('timer');half=data.get('halfTimer');regional=bool(flags&1)
 if not bcd(timer) or not bcd(half) or regional!=(timer==0) or half>timer:raise ValueError('Invalid escape clock')
 values=data.get('timers');halves=data.get('halfTimers')
 if not isinstance(values,list) or not isinstance(halves,list) or len(values)!=10 or len(halves)!=10:raise ValueError('Incomplete escape clocks')
 for t,h in zip(values,halves):
  if not bcd(t) or not bcd(h) or h>t or (regional and not t) or (not regional and (t or h)):raise ValueError('Invalid regional escape clock')
 return data

def capture_clock(patcher):
 s=patcher.settings
 if s['escapeAttr'] is None:return None
 from rom.addresses import Addresses
 spans={};had_override='applyIPSPatch' in patcher.__dict__;old=patcher.applyIPSPatch
 def record(name,patchDict=None):
  if name=='Escape_Timer':
   for a,b in patchDict[name].items():spans.update((a+i,v) for i,v in enumerate(b))
 try:
  patcher.applyIPSPatch=record
  patcher.applyEscapeAttributes(s['escapeAttr'],[])
 finally:
  if had_override:patcher.applyIPSPatch=old
  else:del patcher.applyIPSPatch
 def word(name,index=0,default=0):
  a=Addresses.getOne(name)+2*index
  return spans[a]|spans[a+1]<<8 if a in spans else default
 disabled=s['tourian']=='Disabled'
 result=dict(schema=1,flags=int(disabled)|int(s['escapeRandoRemoveEnemies'])*2|int(not s['isPlando'])*4,
  timer=word('escapeTimer',default=0x300),halfTimer=word('rando_escape_common_timer_half_value',default=0 if disabled else 0x130),
  timers=[word('escapeTimerTable',i) for i in range(10)],
  halfTimers=[word('rando_escape_common_timer_half_values_by_area_id',i) for i in range(10)])
 return validate_clock(result)
