import sys
import os

for f in sys.argv[1:]:
    path,fname = os.path.split(f)
    f = fname.split('.')
    f = '.'.join(f[:4]) + '.' + f[-1]
    fo = os.path.join(path, f)
    print 'ln -s %s %s' % (fo, f)

