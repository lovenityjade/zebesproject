#!/usr/bin/env python3
"""Create a NEW public source snapshot, without private history or reference media.
Does not mutate the source checkout, its submodules, saves, or GitHub repository.
"""
from pathlib import Path
import argparse,hashlib,importlib.util,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('stage',ROOT/'Scripts/stage-runtime.py')
stage=importlib.util.module_from_spec(spec);spec.loader.exec_module(stage)
def export(destination):
 subprocess.run(['python3',str(ROOT/'Scripts/check-distribution.py')],check=True)
 if destination.exists():raise ValueError('Fresh destination required')
 destination.mkdir(parents=True)
 paths=set(subprocess.check_output(['git','ls-files'],cwd=ROOT,text=True).splitlines())
 paths.update(subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines())
 excluded=[];copied=[]
 for name in sorted(paths):
  path=ROOT/name
  exclude=(not path.is_file() or path.is_symlink() or
   name.startswith(('.tmp/','Build/','Releases/','Runtime/','roms/','Soundtracks/','Docs/References/','titlescreen/','Labs/','Docs/Randomizer/FullOptions/Mirror/')) or
   name in {'gameover.mp3','Unreal/Content/GameOver/GameOver.pcm','ETAT.md','verification-supermetroid.json','gameover-16:9.png','gameover-4:3.png','titlescreen-16:9.png','titlescreen-4:3.png','zebes-project-logo.png'} or
   '/Proofs/' in name or path.suffix.lower() in {'.sfc','.smc','.sram','.srm','.sav','.snapshot','.pcm','.mp3','.bgra','.log'} or
   (name.startswith('Unreal/Build/') and name!='Unreal/Build/Windows/Application.ico'))
  if exclude:excluded.append(name);continue
  target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target);copied.append(name)
 # Keep the asset-free native source as its pinned gitlink. The disassembly is
 # research-only and includes original game banks; never clone it for players.
 (destination/'.gitmodules').write_text('[submodule "native-core"]\n\tpath = native-core\n\turl = https://github.com/snesrev/sm.git\n')
 # Vendor only the licensed runtime subset of VARIA. No submodule checkout can
 # silently fetch upstream screenshots, extracted sprites or IPS room streams.
 upstream={}
 for source,rel in stage.randomizer_files(ROOT):
  if rel.parts[0]!='upstream':continue
  target=destination/'Randomizer'/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
  upstream[str(rel.relative_to('upstream'))]=hashlib.sha256(source.read_bytes()).hexdigest()
 for flavor in ['common','vanilla']:
  folder=destination/'Randomizer/upstream/patches'/flavor/'ips';folder.mkdir(parents=True,exist_ok=True)
  (folder/'.gitkeep').write_text('')
 revision=subprocess.check_output(['git','-C',str(ROOT/'Randomizer/upstream'),'rev-parse','HEAD'],text=True).strip()
 provenance=dict(repository='https://github.com/theonlydude/RandomMetroidSolver',revision=revision,policy='Unmodified Python/JSON runtime subset; no upstream graphics, IPS, web assets, tests or recordings',files=upstream)
 (destination/'Randomizer/upstream-provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
 core=subprocess.check_output(['git','-C',str(ROOT/'native-core'),'rev-parse','HEAD'],text=True).strip()
 report=dict(nativeCoreRevision=core,upstreamRevision=revision,sourceFiles=len(copied),variaRuntimeFiles=len(upstream),excludedFiles=excluded)
 (destination/'Docs/Releases/ALPHA-0.24/public-source-export.json').write_text(json.dumps(report,indent=2)+'\n')
 subprocess.run(['git','init','-b','main'],cwd=destination,check=True)
 subprocess.run(['git','config','core.hooksPath','.githooks'],cwd=destination,check=True)
 subprocess.run(['git','add','.'],cwd=destination,check=True)
 subprocess.run(['git','update-index','--add','--cacheinfo',f'160000,{core},native-core'],cwd=destination,check=True)
 print('Public snapshot prepared; audit, commit and publication are separate steps:',destination)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('destination',type=Path)
 args=parser.parse_args();export(args.destination.resolve())
