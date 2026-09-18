"""Build French pages from reviewed prose, never machine translation.

Run before build.py. Requires lxml. Internal identifiers and external URLs stay
unchanged. Missing prose fails the build instead of silently falling back.
"""
from pathlib import Path
from lxml import html, etree
import json
ROOT=Path(__file__).resolve().parent
PUBLIC=ROOT/'public'
CATALOG=ROOT.parent/'Localization/site.fr-CA.tsv'
UNCHANGED={'$XDG_DATA_HOME/zebesproject','--appimage-extract-and-run','.', '.run-<uuid>.json','/','01','02','03','>_','A','ALPHA-0.24','ALPHA-0.24 · 2026','B','CRATERIA','Cyb3R · The T · PopTracker','D63ED5F8','Discord','Discord ↗','Engine/Extras/Redist/en-us/vc_redist.x64.exe','F11','FAQ','GitHub','GitHub ↗','Linux','PROJECT','SMUnreal.exe','SMUnreal.log','Saved','Saved/Logs','Sekailink','Soundtracks/Remastered','Super Metroid (Japan, USA) (En,Ja).sfc','THE ZEBES','THE ZEBES PROJECT','The Zebes Project','TheLovenityJade','TheLovenityJade ↗','VARIA · dude & flo','Windows','~/.local/share/zebesproject','·','×','←','→','↓','↗','↻','⊞','✦','⤢','＋','https://www.youtube.com/watch?v=…'}
META={'description','og:title','og:description','og:image:alt','twitter:title','twitter:description'}
def generate():
    catalog={}
    for n,line in enumerate(CATALOG.read_text().splitlines(),1):
        if not line or line.startswith('#'):continue
        key,value=line.split('\t',1)
        assert key not in catalog, ('Duplicate translation',n,key)
        assert value, ('Empty translation',n,key)
        catalog[key]=value
    missing=set()
    def translate(text):
        if not text or not text.strip():return text
        key=text.strip()
        if key in catalog:return text[:len(text)-len(text.lstrip())]+catalog[key]+text[len(text.rstrip()):]
        if key not in UNCHANGED:missing.add(key)
        return text
    (PUBLIC/'fr').mkdir(exist_ok=True)
    for filename in ('index.html','help.html'):
        tree=html.parse(str(PUBLIC/filename))
        # Generated navigation may be present on subsequent runs.
        for element in tree.xpath('//*[@data-language-switch] | //link[@hreflang]'):element.getparent().remove(element)
        for element in tree.iter():
            if not isinstance(element.tag,str):continue
            if element.tag not in ('script','style'):element.text=translate(element.text)
            element.tail=translate(element.tail)
            for attr in ('alt','aria-label','placeholder','title'):
                if element.get(attr):element.set(attr,translate(element.get(attr)))
            if element.tag=='meta' and element.get('name',element.get('property','')) in META:
                element.set('content',translate(element.get('content')))
            for attr in ('href','content'):
                value=element.get(attr,'')
                for base in ('/zebes/','https://thelovenityjade.me/zebes/'):
                    if value==base or value.startswith(base+'#') or value.startswith(base+'help.html'):
                        element.set(attr,base+'fr/'+value[len(base):]);break
        tree.getroot().set('lang','fr-CA')
        add_languages(tree,filename,True)
        (PUBLIC/'fr'/filename).write_bytes(etree.tostring(tree,encoding='utf-8',method='html',doctype='<!doctype html>'))
        english=html.parse(str(PUBLIC/filename))
        for element in english.xpath('//*[@data-language-switch] | //link[@hreflang]'):element.getparent().remove(element)
        add_languages(english,filename,False)
        (PUBLIC/filename).write_bytes(etree.tostring(english,encoding='utf-8',method='html',doctype='<!doctype html>'))
    assert not missing, ('Untranslated website text', sorted(missing))
    (PUBLIC/'locale-fr.js').write_text('// Authored presentation catalog. Never applied to player data.\nwindow.ZebesFrench='+json.dumps(catalog,ensure_ascii=False)+';\n')
    print(f'French website: {len(catalog)} reviewed strings, both pages complete.')
def add_languages(tree,filename,french):
    suffix='' if filename=='index.html' else filename
    for code,prefix in (('en',''),('fr-CA','fr/')):
        node=etree.SubElement(tree.find('head'),'link',rel='alternate',hreflang=code,href='https://thelovenityjade.me/zebes/'+prefix+suffix)
    nav=tree.xpath('//*[@id="site-nav"]')[0]
    link=etree.SubElement(nav,'a',href='/zebes/'+('' if french else 'fr/')+suffix,lang='en' if french else 'fr-CA',hreflang='en' if french else 'fr-CA')
    link.set('data-language-switch','true');link.text='EN' if french else 'FR';link.set('aria-label','English' if french else 'Français (Canada)')
if __name__=='__main__':generate()
