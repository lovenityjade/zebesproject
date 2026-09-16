"""Versioned door requirements shared by native PLMs and solver/tracker.

The native and Unreal slot contracts validate the same compiled location IDs.
"""
import copy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def capture(patcher):
    if not patcher.settings['doorsColorsRando']:return None
    from utils.doorsmanager import DoorsManager
    from rom.rom import snes_to_pc
    catalog=json.loads((ROOT/'native_door_colors.json').read_text())
    colors=[];doors={}
    # Capture the actual writer rather than deriving a second PLM policy.
    before=copy.deepcopy(patcher.romFile.data)
    try:
        patcher.romFile.data={}
        patcher.writeDoorsColor()
        written=copy.deepcopy(patcher.romFile.data)
    finally:patcher.romFile.data=before
    from utils.doorsmanager import colors2plm
    expected={}
    for loc in catalog['locations']:
        door=DoorsManager.doors[loc['name']]
        if snes_to_pc(door.address)!=loc['address'] or int(door.facing)!=loc['facing'] or door.id!=loc['openedBit'] or door.canRandom!=loc['canRandom']:
            raise ValueError('Native door location changed: '+loc['name'])
        value=0 if door.isBlue() or door.isRefillSave() else catalog['colors'].index(door.color)
        colors.append(value)
        doors[loc['name']]=dict(color=door.color,facing=int(door.facing),address=door.address,openedBit=door.id,hidden=False)
        if value:
            address=loc['address'];plm=colors2plm[door.color][door.facing]
            expected[address]=plm&255;expected[address+1]=plm>>8
            if door.color=='grey':expected[address+5]=0x90
    if written!=expected:raise ValueError('Native door writes disagree with VARIA')
    result=dict(schema=1,catalogSha256=catalog['sha256'],colors=colors,doors=doors)
    validate(result);return result

def validate(data):
    catalog=json.loads((ROOT/'native_door_colors.json').read_text())
    if not isinstance(data,dict) or data.get('schema')!=1 or data.get('catalogSha256')!=catalog['sha256']:
        raise ValueError('Unsupported native door-color contract')
    colors=data.get('colors');doors=data.get('doors')
    if not isinstance(colors,list) or len(colors)!=len(catalog['locations']) or not isinstance(doors,dict) or set(doors)!={d['name'] for d in catalog['locations']}:
        raise ValueError('Incomplete native door-color table')
    for loc,value in zip(catalog['locations'],colors):
        door=doors[loc['name']]
        if not isinstance(door,dict) or door.get('color') not in catalog['colors'] or type(value) is not int or not 0<=value<len(catalog['colors']):
            raise ValueError('Invalid door requirement')
        expected=0 if door['color']=='blue' or not loc['canRandom'] else catalog['colors'].index(door['color'])
        # Source addresses in the tracker are SNES addresses, not PC offsets.
        address=((loc['address']//0x8000|0x80)<<16)|(loc['address']%0x8000|0x8000)
        if value!=expected or door.get('facing')!=loc['facing'] or door.get('openedBit')!=loc['openedBit'] or door.get('address')!=address or door.get('hidden') is not False:
            raise ValueError('Tracker/native door requirements disagree: '+loc['name'])
    return data

def apply_context(context):
    data=(context or {}).get('doorColors')
    if data is None:return
    validate(data)
    from utils.doorsmanager import DoorsManager
    for name,door in data['doors'].items():
        DoorsManager.doors[name].setColor(door['color']);DoorsManager.doors[name].hidden=False
