#!/usr/bin/env python3
"""Isolate Ceres' repeating checker panels behind their metal frames.
Only reviewed runtime tile identities are baked; no screen-wide color mask.
"""
import json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Docs/LightingAudit'
config_path=root/'Config/ceres-background.json'
if config_path.exists() and '--from-audit' not in sys.argv:
    config=json.loads(config_path.read_text());rules=config['tiles']
else:
    v=(out/'ceres-elevator-vram.bin').read_bytes()
    m=json.loads((out/'ceres-elevator.json').read_text())
    pattern=[v[(0xa5*64+j)*2+1] for j in range(64)]
    rules={}
    for r in m['tiles']:
        t=r['tile'];data=[v[(t*64+j)*2+1] for j in range(64)];mask=0
        # Matching 3x3 patches distinguish the checker plane from flat dark bars.
        for y in range(6):
            for x in range(6):
                cells=[(y+dy)*8+x+dx for dy in range(3) for dx in range(3)]
                if all(data[j]==pattern[j] for j in cells):
                    for j in cells:mask|=1<<j
        if mask:rules[r['fnv']]=f'{mask:016x}'
    config={'room':'df45','pattern_tile_hash':'a25639c5','pattern_tile':0xa5,'tiles':rules,
            'scope':'Decorative checker panels only; metal frames remain fixed. Identity Mode7 transform only.'}
    (root/'Config/ceres-background.json').write_text(json.dumps(config,indent=2)+'\n')
header=['/* Generated from Ceres runtime audit. */','typedef struct {uint32_t hash;uint64_t mask;} SmBackgroundRule;',
        'static const SmBackgroundRule background_rules[]={']
header += ['{0x%su,0x%sull},'%(h,mask) for h,mask in rules.items()]
header += ['};','#define SM_CERES_PATTERN_TILE 0xa5 /* Read from the ROM-loaded Mode 7 VRAM. */']
(root/'Native/sm_background.generated.h').write_text('\n'.join(header)+'\n')
print('Ceres decorative background:',len(rules),'tile identities')
