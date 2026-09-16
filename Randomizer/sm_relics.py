"""Relics replace surplus ammo only; final placements are re-solved afterward."""
from collections import Counter
import random


def register_relic():
    from rando.Items import Item, ItemManager
    from logic.smboolmanager import SMBoolManager
    ItemManager.Items['ChozoRelic'] = Item(Category='Minor',Class='Minor',Code=0xf200,
        Name='Chozo Tablet',Type='ChozoRelic')
    if 'ChozoRelic' not in SMBoolManager.items:SMBoolManager.items.append('ChozoRelic')
    if 'ChozoRelic' not in SMBoolManager.countItems:SMBoolManager.countItems.append('ChozoRelic')


def place_relics(placements, settings, seed, progression=()):
    if not settings['enabled']:return
    register_relic()
    from rando.Items import ItemManager
    count=Counter(p['item'] for p in placements)
    # Keep a conservative minimum; the solver verifies the actual remainder,
    # including boss energy/ammo requirements, rather than trusting this floor.
    minimum={'Missile':8,'Super':4,'PowerBomb':4}
    # A pool-wide ammo floor does not protect the first pack needed to leave
    # the starting area. Preserve VARIA's progression placements, including
    # later ammo packs needed for doors and boss fights. The final independent
    # solver remains authoritative: this list alone is not a reachability proof.
    protected={p['location'] for p in progression}
    candidates=[p for p in placements if p['location'] not in protected and
                (p['item'] in minimum or p['item'] in ('Nothing','NoEnergy'))]
    random.Random(f'ChozoRelic-v2:{seed}').shuffle(candidates)
    chosen=[]
    for p in candidates:
        if p['item'] in minimum and count[p['item']]<=minimum[p['item']]:continue
        count[p['item']]-=1;chosen.append(p)
        if len(chosen)==settings['placed']:break
    if len(chosen)!=settings['placed']:
        raise ValueError(f"This pool has room for only {len(chosen)} tablets while preserving progression and minimum ammunition; lower Relics placed or increase the ammo pool")
    for p in chosen:
        p['item']='ChozoRelic';p['kind']=1
        p['plm']=ItemManager.Items['Reserve'].Code+{'Visible':0,'Chozo':84,'Hidden':168}[p['visibility']]
