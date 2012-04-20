# Generate a WORKFILE for htrack

import sys
import os.path 

def help():
    print >>sys.stderr, 'Usage: mkpfindxargs FILES | xargs -P# -n4'
    sys.exit(2)

if __name__ == '__main__':

    files = sys.argv[1:]

    if len(files) == 0: help()

    files.sort(reverse=True)

    for i in xrange(len(files)-1):
        z = i
        f0 = split(files[i], '.')
        f1 = split(files[i+1], '.')

        fname0 = '.'.join(f0[:4])
        fname1 = '.'.join(f1[:4])

        path,out = os.path.split(fname1)

        print '--ahf -o %s.pfind %s.grp %s.stat %s.grp %s.stat' % (out,fname0, fname0, fname1, fname1)


