"""Source-written mixed routing and effective location membership for Minimizer."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def catalog():return json.loads((ROOT/'native_minimizer.json').read_text())
def validate(data):
 c=catalog()
 if not isinstance(data,dict) or data.get('schema')!=1 or data.get('catalogSha256')!=c['sha256']:raise ValueError('Unsupported Minimizer contract')
 dest=data.get('destinations')
 if not isinstance(dest,list) or len(dest)!=40 or any(type(d) is not int or not 0<=d<40 for d in dest) or len(set(dest))!=40 or any(dest[d]!=i for i,d in enumerate(dest)):raise ValueError('Invalid mixed door permutation')
 regions=data.get('regions');mask=data.get('checks')
 if not isinstance(regions,list) or not regions or regions!=sorted(set(regions)) or any(type(i) is not int or not 1<=i<=10 for i in regions):raise ValueError('Invalid Minimizer regions')
 if not isinstance(mask,list) or len(mask)!=100 or any(type(i) is not int or i not in (0,1) for i in mask):raise ValueError('Invalid Minimizer location membership')
 for loc,enabled in zip(c['locations'],mask):
  if enabled and loc['region'] not in regions and not loc['postBoss']:raise ValueError('Check outside retained regions')
 if data.get('locations')!=[loc['name'] for loc,on in zip(c['locations'],mask) if on]:raise ValueError('Minimizer check identities disagree')
 return data

def capture(patcher):
 settings=patcher.settings
 if settings.get('minimizerN') is None:return None
 if not settings['area'] or not settings['boss']:raise ValueError('Minimizer requires mixed area and boss routing')
 c=catalog();ids={a['name']:a['id'] for a in c['accessPoints']};mapping={}
 for actual in settings['doors']:
  src,dst=actual['transition']
  if src.Escape and dst.Escape:continue
  i,j=ids[src.Name],ids[dst.Name]
  if i in mapping:raise ValueError('Duplicate mixed source endpoint')
  expected=c['connections'][i][j]
  for a,b in [('room','RoomPtr'),('door','DoorPtr'),('direction','direction'),('bitFlag','bitFlag'),('asm','doorAsmPtr'),('distance','distanceToSpawn')]:
   if actual[b]!=expected[a]:raise ValueError('Mixed door descriptor drift: '+src.Name+' '+a)
  if list(actual['cap'])!=expected['cap'] or list(actual['screen'])!=expected['screen'] or actual.get('exitAsm')!=expected['exitAsm']:raise ValueError('Mixed door geometry drift')
  if expected['incompatible'] and (actual.get('SamusX')!=expected['x'] or actual.get('SamusY')!=expected['y']):raise ValueError('Mixed door spawn drift')
  mapping[i]=j
 if set(mapping)!=set(range(40)):raise ValueError('Incomplete mixed world')
 byaddr={il.Location.Address:il for il in settings['itemLocs'] if not il.Location.isBoss()}
 if set(byaddr)!={l['address'] for l in c['locations']}:raise ValueError('Minimizer lost original placement identities')
 checks=[int(not byaddr[l['address']].Location.restricted) for l in c['locations']]
 for l,on in zip(c['locations'],checks):
  if not on and byaddr[l['address']].Item.Category!='Nothing':raise ValueError('Excluded check contains a real item')
 regions=sorted(c['regions'].index(a) for a in patcher._getAccessibleAreasNoBoss(settings['itemLocs']))
 result=dict(schema=1,catalogSha256=c['sha256'],destinations=[mapping[i] for i in range(40)],regions=regions,checks=checks,
             locations=[l['name'] for l,on in zip(c['locations'],checks) if on])
 return validate(result)

def topology(contract):
 from sm_topology import vanilla
 validate(contract);c=catalog();d=contract['destinations'];aps=c['accessPoints']
 result=vanilla();result.update(mode='minimizer',areaPairs=[],bossPairs=[],nativeMinimizer=contract,
     mixedPairs=[[aps[i]['name'],aps[j]['name']] for i,j in enumerate(d) if i<=j])
 return result

def totals(contract):
 from logic.logic import Logic
 c=catalog();retained=validate(contract)['regions']
 maps=[Logic.map_tilecount['area_rando'][r] if i in retained else c['bossTileTotals'][i] for i,r in enumerate(c['regions'])]
 enemies=[sum(row[i] for i in retained) for row in c['enemyRegionTotals']]
 return maps,enemies
