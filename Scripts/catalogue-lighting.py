#!/usr/bin/env python3
"""Create an offline, searchable audit with explicit review coverage."""
import html,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Docs/LightingAudit/Inventory'
m=json.loads((out/'manifest.json').read_text());rules=json.loads((root/'Config/lighting-sources.json').read_text())
body=['<!doctype html><meta charset="utf-8"><title>Super Metroid · Audit lumière</title><style>body{background:#10141b;color:#dbe5f1;font:16px system-ui;margin:32px}input{padding:12px;width:70%;background:#202a36;color:white;border:1px solid #526579}article{padding:20px;background:#19212c;margin:12px 0}img{image-rendering:pixelated;max-width:100%}a{color:#8fcaff}small{color:#b7c6d5}</style><h1>Audit des sources de lumière</h1><p>29 jeux de décors · 163 définitions d’ennemis · 28 fichiers graphiques complémentaires. Inventaire ≠ validation de chaque salle et animation.</p><p>Masques actifs : voyants et lampe de Cérès, huit planches de feu. Les tirs et fluides utilisent leurs positions natives. Les autres ressources restent à annoter.</p><input id="q" placeholder="Chercher : Phantoon, Lava, Ridley, needs-review…" oninput="for(let a of document.querySelectorAll(\'article\'))a.hidden=!a.textContent.toLowerCase().includes(this.value.toLowerCase())">']
for r in m['records']:
    active=r.get('definition') in rules['fire_sprite_definitions']
    status='Masque de feu actif — rendu en salle à valider' if active else 'needs-review — masque individuel à annoter'
    name=r.get('init_handler',r.get('name','Décor '+str(r.get('index',''))))
    body.append('<article><h2>'+html.escape(name)+'</h2><p>'+html.escape(status)+'</p><small>'+html.escape(r['definition']+' · '+r.get('graphics_pointer','')+' · '+str(r['bytes'])+' octets')+'</small>')
    if 'sheet' in r:body.append('<p><a href="'+r['sheet']+'"><img loading="lazy" src="'+r['sheet']+'"></a></p>')
    else:body.append('<p>Données extraites ; composition et palette à vérifier en salle.</p>')
    body.append('</article>')
body.append('<h2>Graphismes complémentaires du désassemblage</h2>')
for r in m.get('raw_graphics_files',[]):body.append('<article>'+html.escape(r['path'])+' · '+str(r['bytes'])+' octets · needs-format-review</article>')
(out/'index.html').write_text('\n'.join(body))
print(out/'index.html')
