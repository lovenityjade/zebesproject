"""Complete starting door bits, opt-in by version for newly generated seeds."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def validate(data,spawn):
    catalog=json.loads((ROOT/'native_initial_doors.json').read_text())
    entry=next((s for s in catalog['starts'] if s['spawn']==spawn),None)
    if not entry or not isinstance(data,dict) or data.get('schema')!=1 or data.get('opened')!=entry['opened'] or any(type(v) is not int for v in data.get('opened',[])):
        raise ValueError('Initial door bits disagree with the native start')
    return data

def capture(patcher):
    from graph.graph_utils import getAccessPoint
    from utils.doorsmanager import DoorsManager
    # getStartDoors also schedules blinking patches. Inspect its real result,
    # then restore that scheduling list; native area data has its own contract.
    previous=list(patcher.ipsPatches)
    try:
        doors=patcher.getStartDoors([],patcher.settings['area'],patcher.settings['minimizerN'])
        DoorsManager.getBlueDoors(doors)
    finally:patcher.ipsPatches=previous
    start=getAccessPoint(patcher.settings['startLocation']).Start
    doors+=start.get('doors',[])
    return validate(dict(schema=1,opened=sorted(set(doors))),start['spawn'])

def apply_context(context):
    data=(context or {}).get('world',{}).get('initialDoors')
    if data is None:return
    validate(data,context['spawn'])
    from utils.doorsmanager import DoorsManager
    for door in DoorsManager.doors.values():
        if door.id in data['opened']:door.setColor('blue')
