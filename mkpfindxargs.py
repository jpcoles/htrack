# Generate a WORKFILE for pfind

import sys
import os.path 
from string import split

def help():
    print >>sys.stderr, 'Usage: mkpfindxargs FILES | xargs -P# -n4'
    sys.exit(2)

if __name__ == '__main__':

    files = sys.argv[1:]

    if len(files) == 0: help()

    files.sort(reverse=True)

    for i in xrange(len(files)-1):
        z = i
        p0,f0 = os.path.split(files[i])
        p1,f1 = os.path.split(files[i+1])
	

        f0 = split(f0, '.')
        f1 = split(f1, '.')

	#print f0
	#print f1

        grp0 = '%s/%s' % (p0, '.'.join(f0[:4]))
        stat0 = '%s/%s' % (p0, '.'.join(f0[:-1]))

        grp1 = '%s/%s' % (p1, '.'.join(f1[:4]))
        stat1 = '%s/%s' % (p1, '.'.join(f1[:-1]))

        path,out = os.path.split(grp0)

        print '--ahf -o %s.pfind %s.grp %s.AHF_halos %s.grp %s.AHF_halos' % (out,grp0, stat0, grp1, stat1)


