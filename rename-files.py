#run:
#python rename_files.py *AHF_halos

import sys
import os
import subprocess


for f0 in sys.argv[1:]:
    path,fname = os.path.split(f0)
    f = fname.split('.')
    f = '.'.join(f[:4]) + '.' + f[-1]
    fo = os.path.join(path, f)
    cmd = 'ln -s %s %s' % (f0, f)
    subprocess.call(cmd.split())
    print cmd

