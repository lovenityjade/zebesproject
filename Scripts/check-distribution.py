#!/usr/bin/env python3
"""Reject known extracted artwork and changed/unreviewed ROM reconstruction.
This is a targeted provenance gate, not a legal/copyright classifier.
"""
from pathlib import Path
import hashlib
import json
import re
import sys
root=Path(__file__).resolve().parents[1]
blockers=[]
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
for name,symbols in {
 'sm_tracker_assets.inc':['tracker_icons'],
 'sm_credits_assets.inc':['credits_font','credits_palette','credits_planet','credits_planet_palette'],
 'sm_varia_assets.inc':['varia_gfx'], 'sm_objective_pause_assets.inc':['objective_pause_gfx'],
 'sm_mirror_data.inc':['mirror_bytes'], 'sm_world_data.inc':['world_patch_bytes'],
 'sm_areas.inc':['area_data'], 'sm_escape_data.inc':['escape_room_data'],
 'sm_animals_data.inc':['animals_bytes'],
}.items():
 path=root/'Native'/name
 if path.exists():
  for symbol in symbols:
   if re.search(r'\b'+symbol+r'\s*(?:\[[^\]]*\]\s*)+=\s*\{',path.read_text()):
    blockers.append(f'{path.relative_to(root)}: embedded {symbol}')
for domain in ['world','areas','escape','animals','tracker','varia_gfx','objective_pause_gfx']:
 recipe=root/'Native'/({'varia_gfx':'sm_varia_assets.inc','objective_pause_gfx':'sm_objective_pause_assets.inc'}.get(domain,f'sm_{domain}_rom_assets.inc'))
 try:
  proof=json.loads((root/f'Docs/Releases/ALPHA-0.24/{domain}-rom-reconstruction.json').read_text())
  recipe_hash=digest(recipe)
  if domain in ('varia_gfx','objective_pause_gfx'):
   text=recipe.read_text();start=text.index('static uint8_t '+domain+'[')
   end=text.index('\n}\n',text.index('static void '+domain+'_load(void){',start))+3
   recipe_hash=hashlib.sha256(text[start:end].encode()).hexdigest()
  if proof.get('recipeSha256')!=recipe_hash:raise ValueError('recipe changed')
  if domain in ['world','areas','escape','animals'] and not proof.get('byteExactReconstruction'):raise ValueError('unverified room reconstruction')
  if domain=='tracker' and proof.get('visualReviewPending',True):raise ValueError('icons await review')
 except (OSError,ValueError) as e:blockers.append(f'{domain}: {e}')
try:
 proof=json.loads((root/'Unreal/Content/Splash/provenance.json').read_text())
 if proof['source']!='zebesproject-logo.png' or digest(root/proof['source'])!=proof['sourceSha256']:raise ValueError('startup logo source changed')
 for name in ['Unreal/Content/Splash/Icon.bmp','Unreal/Content/Splash/EdIcon.bmp','Unreal/Build/Windows/Application.ico']:
  if digest(root/name)!=proof['files'][name]:raise ValueError('startup icon changed: '+name)
except (OSError,ValueError,KeyError) as e:blockers.append(str(e))
if blockers:
 print('DISTRIBUTION BLOCKED:\n'+'\n'.join(' - '+b for b in blockers),file=sys.stderr);sys.exit(1)
print('Known runtime artwork gates passed; source/history and staged-file audit remain required.')
