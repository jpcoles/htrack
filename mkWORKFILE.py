# Generate a WORKFILE for htrack

import sys
from os.path import split, splitext, join

def help():
    print >>sys.stderr, 'Usage: mkWORKFILE [--statdir DIR] [--dir DIR] FILES'
    print >>sys.stderr, 'Generate a WORKFILE for htrack using FILES from pfind'
    sys.exit(2)

def mkpath(path0, path1, fname):
    return join(path0, fname) if path is not None else join(path1, fname)

if __name__ == '__main__':

    statdir = None
    dir = None

    files = sys.argv[1:]

    if len(files) == 0: help()

    if files[0] == '--statdir':
        files = files[1:]
        if len(files) == 0: help()
        statdir = files[0]
        files = files[1:]
        if len(files) == 0: help()

    if files[0] == '--dir':
        files = files[1:]
        if len(files) == 0: help()
        dir = files[0]
        files = files[1:]
        if len(files) == 0: help()

    files.sort(reverse=True)

    for i in xrange(len(files)):
        z = i
        path,fname = split(files[i])
        fname,ext = splitext(fname)
        if ext == '.stat':
            pfind = mkpath(dir, path, fname+'.pfind')
            stat  = files[i]
        elif ext == '.pfind':
            pfind = files[i]
            stat  = mkpath(statdir, path, fname+'.stat')
        else:
            pfind = mkpath(dir, path, fname+'.pfind')
            stat  = mkpath(statdir, path, fname+'.stat')

        print z,stat,pfind
