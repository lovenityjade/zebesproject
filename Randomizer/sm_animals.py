"""Capture VARIA's actual Animals Surprise draw; never redraw the result."""
import json
from pathlib import Path

def capture(patcher,rules):
    catalog=json.loads((Path(__file__).resolve().parent/'native_animals.json').read_text())
    names={v['name']:v['id'] for v in catalog['variants']}
    selected=[p for p in patcher.settings['optionalPatches'] if p in names]
    if len(selected)>1:raise ValueError('Multiple Animals Surprise variants')
    if not selected:
        if rules['options']['animals']=='on':
            rules['options']['animals']='off'
            rules['adjustments'].append('Animals Surprise ignored by VARIA for Mirror or escape randomization.')
        return None
    if rules['options']['animals']!='on' or 'Escape_Animals_Change_Event' not in patcher.settings['optionalPatches']:
        raise ValueError('Animals Surprise selection and source event patch disagree')
    return dict(schema=1,catalogSha256=catalog['sha256'],mode=names[selected[0]],patch=selected[0])
