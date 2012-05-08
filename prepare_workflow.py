import sys
import os
import glob
import getopt
import shlex
from itertools import izip

ahf2grp_xargs   = 'ahf2grp_xargs'
pfind_xargs     = 'pfind_xargs'
htrack_workfile = 'WORKFILE'

def help():
    print >>sys.stderr, \
        'Usage: prepare_workflow [OPTIONS] --tipsy-files=FILES --group-files=FILES --stat-files=FILES\n' \
        '\n' \
        'FILES should be given using wildcards and not a list of files.\n' \
        '\n' \
        'where OPTIONS is\n' \
        '    --convert-group-files="ahf2grp options"        Create a script to convert AHF_particles files\n' \
        '                                                   to group files supported by pfind and htrack.\n' \
        '                                                   If no additional options are needed then give "".\n' \
        '    --pfind-opts="pfind options"                   Pass options to pfind.\n' \
        '    --htrack-opts="pfind options"                  Pass options to htrack.\n' \
        '    --output-dir=DIR                               Write files to DIR and prepare the scripts\n' \
        '                                                   to reference files relative the DIR. Creates DIR\n' \
        '                                                   if it does not already exist.\n' \
        '    --bin-dir=DIR                                  Location of ahf2grp,pfind,htrack.\n'\
        'All options must be enclosed in quotes ("").\n' \
        '\n' \
        'Report problems to <jonathan@physik.uzh.ch>.'
    sys.exit(2)

def subpathext(f, new_path, new_ext):
    path,file = os.path.split(f)
    name,ext  = os.path.splitext(file)
    return os.path.join(new_path, name + new_ext)

if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hP:", 
            ["help", "tipsy-files=", 'group-files=', 'stat-files=', 'convert-group-files=', 'pfind-opts=', 'htrack-opts=', 'output-dir=', 'bin-dir='])
    except getopt.GetoptError, err:
        help()
        sys.exit(2)

    tipsy_files = None
    group_files = None
    ginfo_files = None
    convert_group_files = False
    pfind_opts = ''
    htrack_opts=''
    ahf2grp_opts=''
    nprocs = 1
    output_dir = os.curdir
    bin_dir = ''

    for o, a in opts:
        if o in ['--tipsy-files']:
            tipsy_files = glob.glob(os.path.expandvars(os.path.expanduser(a)))
        elif o in ['--group-files']:
            group_files = glob.glob(os.path.expandvars(os.path.expanduser(a)))
        elif o in ['--stat-files']:
            ginfo_files = glob.glob(os.path.expandvars(os.path.expanduser(a)))
        elif o in ['--convert-group-files']:
            convert_group_files = True
            ahf2grp_opts = a
        elif o in ['--pfind-opts']:
            pfind_opts = a
        elif o in ['--htrack-opts']:
            htrack_opts = a
        elif o in ['--output-dir']:
            output_dir = a
        elif o in ['--bin-dir']:
            bin_dir = a
        elif o in ['-P']:
            nprocs = int(a)
        elif o in ['--help', '-h']:
            help()

    if None in [tipsy_files, group_files, ginfo_files]:
        help()

    if 0 in [len(tipsy_files), len(group_files), len(ginfo_files)]:
        help()

    if len(tipsy_files) != len(group_files) and len(group_files) != len(ginfo_files):
        print >>sys.stderr, "ERROR: The number of files in each group is not the same."
        sys.exit(1)

    tipsy_files.sort(reverse=True)
    group_files.sort(reverse=True)
    ginfo_files.sort(reverse=True)

    if not os.path.exists(output_dir):
        try:
            os.makedirs(output_dir)
        except os.error:
            print >>sys.stderr, 'ERROR: Directory %s did not exist and could not be created.' % output_dir
            sys.exit(1)

    #ahf2grp_xargs   = os.path.join(output_dir, ahf2grp_xargs)
    #pfind_xargs     = os.path.join(output_dir, pfind_xargs)
    ##htrack_workfile = os.path.join(output_dir, htrack_workfile)

    htrack_bin  = os.path.join(bin_dir, 'htrack')
    pfind_bin   = os.path.join(bin_dir, 'pfind')
    ahf2grp_bin = os.path.join(bin_dir, 'ahf2grp')

    if convert_group_files:
        with open(os.path.join(output_dir, ahf2grp_xargs), 'w+') as fp:
            new_group_files = []
            for t,grp in izip(tipsy_files, group_files):
                t = os.path.relpath(t, output_dir)
                grp = os.path.relpath(grp, output_dir)
                out = subpathext(grp, '', '.grp')
                new_group_files.append(out)
                s = '-o "%s" --tipsy-file="%s" "%s"' % (out, t, grp)
                print >>fp, s
            nargs = len(shlex.split(s))
            print 'xargs -a "%s" -n%i -P%i %s %s' % (ahf2grp_xargs, nargs, nprocs, ahf2grp_bin, ahf2grp_opts)
        group_files = new_group_files


    pfind_files = []
    with open(os.path.join(output_dir,pfind_xargs), 'w+') as fp:
        for d,p in izip(izip(group_files[:-1], ginfo_files[:-1]), izip(group_files[1:], ginfo_files[1:])):
            dgrp = os.path.relpath(d[0], output_dir)
            dstat = os.path.relpath(d[1], output_dir)
            pgrp = os.path.relpath(p[0], output_dir)
            pstat = os.path.relpath(p[1], output_dir)
            out = subpathext(dgrp, '', '.pfind')
            pfind_files.append(out)
            s = '-o "%s" %s %s %s %s' % (out, dgrp, dstat, pgrp, pstat)
            print >>fp, s
        nargs = len(shlex.split(s))
        print 'xargs -a "%s" -n%i -P%i %s %s' % (pfind_xargs, nargs, nprocs, pfind_bin, pfind_opts)

    with open(os.path.join(output_dir,htrack_workfile), 'w+') as fp:
        for i,[stat,pf] in enumerate(izip(ginfo_files, pfind_files)):
            stat = os.path.relpath(stat, output_dir)
            print >>fp, '%5s %s %s' % (i, stat, pf)
        print '%s %s %s' % (htrack_bin, htrack_opts, htrack_workfile)
