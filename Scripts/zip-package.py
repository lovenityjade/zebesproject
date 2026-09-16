#!/usr/bin/env python3
"""ZIP a staged folder with canonical portable paths; refuse existing output."""
import argparse
import os
from pathlib import Path
import zipfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',type=Path);parser.add_argument('output',type=Path)
args=parser.parse_args();root=args.source.resolve(strict=True);output=args.output.resolve()
if output.exists() or output.is_relative_to(root):raise ValueError('Fresh output outside the staged folder required')
temporary=output.with_suffix(output.suffix+'.tmp')
with temporary.open('xb') as stream:
 with zipfile.ZipFile(stream,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
  for path in sorted(root.rglob('*')):
   if path.is_symlink():raise ValueError('Unreviewed archive symlink: '+str(path))
   if path.is_file():archive.write(path,path.relative_to(root).as_posix())
with zipfile.ZipFile(temporary) as archive:
 if archive.testzip() is not None:raise ValueError('ZIP verification failed')
os.link(temporary,output);temporary.unlink()
print(output)
