"""Shared, validated world connections for generation, solver and live oracle.

Area and boss routing use separate immutable native domains. Escape routing
remains gated until its native implementation is complete.
"""
import copy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def vanilla():
    from graph.graph_utils import vanillaTransitions,vanillaBossesTransitions,vanillaEscapeTransitions
    return dict(mode='vanilla',areaPairs=[list(p) for p in vanillaTransitions],
                bossPairs=[list(p) for p in vanillaBossesTransitions],escapePairs=[list(vanillaEscapeTransitions[0])])

def _pairs(value):
    if not isinstance(value,list) or any(not isinstance(p,(list,tuple)) or len(p)!=2 or any(not isinstance(n,str) for n in p) for p in value):
        raise ValueError('Malformed world connection list')
    pairs=[tuple(sorted(p)) for p in value]
    if len(set(pairs))!=len(pairs):raise ValueError('Duplicate world connection')
    return set(pairs)

def _catalog(area):
    return json.loads((ROOT/('native_areas.json' if area else 'native_connections.json')).read_text())

def _validate_domain(topology,area):
    kind='areaPairs' if area else 'bossPairs'
    field='nativeAreas' if area else 'native'
    catalog=_catalog(area);aps=catalog['accessPoints'];ids={ap['name']:ap['id'] for ap in aps}
    pairs=topology.get(kind);_pairs(pairs)
    names=[n for p in pairs for n in p]
    if len(names)!=len(ids) or len(names)!=len(set(names)) or set(names)!=set(ids):
        raise ValueError('Topology must connect every endpoint exactly once: '+kind)
    if not area and any(a.endswith('In')==b.endswith('In') for a,b in pairs):
        raise ValueError('Boss connection must join an entrance to an arena')
    mapping=[None]*len(ids)
    for a,b in pairs:mapping[ids[a]]=ids[b];mapping[ids[b]]=ids[a]
    native=topology.get(field)
    if not isinstance(native,dict) or native.get('catalogSha256')!=catalog['sha256'] or native.get('destinations')!=mapping or any(type(i) is not int for i in native.get('destinations',[])):
        raise ValueError('Graph and native routing disagree: '+kind)

def validate(topology):
    default=vanilla()
    if not isinstance(topology,dict) or topology.get('mode') not in ('vanilla','boss','area','area-boss','minimizer'):
        raise ValueError('Unsupported native world topology')
    if 'nativeEscape' in topology:
        from sm_escape import validate as validate_escape,escape_pairs
        escape=validate_escape(topology['nativeEscape'])
        if topology.get('escapePairs')!=escape_pairs(escape['routing']):raise ValueError('Escape graph and native routing disagree')
    elif _pairs(topology.get('escapePairs'))!=_pairs(default['escapePairs']):
        raise ValueError('Escape connections require their native contract')
    if topology['mode']=='minimizer':
        from sm_minimizer import topology as expected_topology
        expected=expected_topology(topology.get('nativeMinimizer'))
        if any(key in topology for key in ('native','nativeAreas')) or topology.get('areaPairs')!=[] or topology.get('bossPairs')!=[] or _pairs(topology.get('mixedPairs'))!=_pairs(expected['mixedPairs']):
            raise ValueError('Mixed graph and native Minimizer routing disagree')
        return [tuple(p) for p in topology['mixedPairs']+topology['escapePairs']]
    if 'nativeMinimizer' in topology or 'mixedPairs' in topology:raise ValueError('Minimizer requires mixed topology')
    for area in (False,True):
        enabled=topology['mode'] in (('area','area-boss') if area else ('boss','area-boss'))
        kind='areaPairs' if area else 'bossPairs';field='nativeAreas' if area else 'native'
        if enabled:_validate_domain(topology,area)
        elif field in topology or _pairs(topology.get(kind))!=_pairs(default[kind]):
            raise ValueError('Unchanged topology contains native routing: '+kind)
    return [tuple(p) for kind in ('areaPairs','bossPairs','escapePairs') for p in topology[kind]]

def capture(settings,escape=None):
    topology=vanilla()
    if (settings.get('escapeAttr') is not None)!=(escape is not None):raise ValueError('Missing native escape contract')
    if escape is not None:
        from sm_escape import escape_pairs
        topology.update(nativeEscape=escape,escapePairs=escape_pairs(escape['routing']))
    area_enabled=bool(settings.get('area'));boss_enabled=bool(settings.get('boss'))
    allowed=set()
    for area,enabled in ((False,boss_enabled),(True,area_enabled)):
        if not enabled:continue
        catalog=_catalog(area);aps=catalog['accessPoints'];ids={ap['name']:ap['id'] for ap in aps};mapping={}
        allowed.update(ids)
        for connection in settings['doors']:
            source,destination=connection['transition'];a,b=source.Name,destination.Name
            if a not in ids:continue
            if b not in ids or a in mapping:raise ValueError('Unexpected native door connection')
            i,j=ids[a],ids[b];expected=catalog['connections'][i][j]
            if expected is None:raise ValueError('Invalid native endpoint pairing')
            for key,field in [('room','RoomPtr'),('door','DoorPtr'),('direction','direction'),('bitFlag','bitFlag'),('asm','doorAsmPtr'),('distance','distanceToSpawn')]:
                if expected[key]!=connection[field]:raise ValueError('Native descriptor drift: '+a+' '+field)
            for key in ('cap','screen'):
                if expected[key]!=list(connection[key]):raise ValueError('Native door geometry drift: '+a)
            if expected['exitAsm']!=connection.get('exitAsm'):raise ValueError('Native exit routine drift')
            if expected['incompatible'] and (expected['x']!=connection.get('SamusX') or expected['y']!=connection.get('SamusY')):
                raise ValueError('Native spawn position drift')
            mapping[a]=b
        if set(mapping)!=set(ids) or any(mapping.get(b)!=a for a,b in mapping.items()):
            raise ValueError('Incomplete or asymmetric native mapping')
        topology['areaPairs' if area else 'bossPairs']=[[a,mapping[a]] for a in sorted(mapping) if (a<mapping[a] if area else a.endswith('Out'))]
        topology['nativeAreas' if area else 'native']=dict(catalogSha256=catalog['sha256'],destinations=[ids[mapping[ap['name']]] for ap in aps])
    if any(c['transition'][0].Name not in allowed and not (escape is not None and all(a.Escape for a in c['transition'])) for c in settings.get('doors',[])):
        raise ValueError('Unexpected world connection outside enabled native domains')
    topology['mode']='area-boss' if area_enabled and boss_enabled else 'area' if area_enabled else 'boss' if boss_enabled else 'vanilla'
    validate(topology);return topology

def from_context(context):
    topology=copy.deepcopy((context or {}).get('topology') or vanilla())
    validate(topology);return topology
