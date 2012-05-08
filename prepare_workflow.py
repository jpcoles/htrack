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
        'Usage: prepare_workflow [OPTIONS] GROUP-LIST --tipsy-files=FILES --stat-files=FILES\n' \
        '\n' \
        'Create a set of files containing the input to ahf2grp, pfind, and htrack. These files\n' \
        'will be created in the current directory unless otherwise specified. The necessary\n' \
        'commands to use these files will be displayed on the screen. They must be run within\n'\
        'the output directory. \n' \
        '\n' \
        'FILES should be given using wildcards and not a list of files.\n' \
        '\n' \
        'GROUP-LIST must be one of' \
        '\n' \
        '    --group-files=FILES               The group files which list the group each particle\n'\
        '                                      belongs to. If these do not exist use\n' \
        '                                      --ahf-particle-files to create them.\n' \
        '    --ahf-particle-files=FILES        Create a script to convert AHF_particles files\n' \
        '                                      to group files supported by pfind and htrack.\n' \
        'OPTIONS can be any of\n' \
        '    --ahf2grp-opts="options"          Pass options to ahf2grp.\n' \
        '    --pfind-opts="options"            Pass options to pfind.\n' \
        '    --htrack-opts="options"           Pass options to htrack.\n' \
        '    --output-dir=DIR                  Write files to DIR and prepare the scripts to\n' \
        '                                      reference files relative to DIR. Creates DIR if it\n' \
        '                                      does not already exist.\n' \
        '    --bin-dir=DIR                     Location of ahf2grp,pfind,htrack.\n'\
        '    -P #                              Let xargs use # processors.\n'\
        '    -h/--help                         Show this help screen.\n'\
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
        opts, args = getopt.getopt(sys.argv[1:], 'hP:', 
            ['help', 
             'tipsy-files=', 'group-files=', 'stat-files=', 'ahf-particle-files=',
             'ahf2grp-opts=', 'pfind-opts=', 'htrack-opts=', 
             'output-dir=', 'bin-dir='])
    except getopt.GetoptError, err:
        help()
        sys.exit(2)

    tipsy_files = None
    group_files = None
    ginfo_files = None
    ahf_particle_files = None
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
        elif o in ['--ahf-particle-files']:
            convert_group_files = True
            ahf_particle_files = glob.glob(os.path.expandvars(os.path.expanduser(a)))
        elif o in ['--ahf2grp-opts']:
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

    if None in [tipsy_files, ginfo_files]:
        help()

    if 0 in [len(tipsy_files), len(ginfo_files)]:
        help()

    if group_files and ahf_particle_files:
        print >>sys.stderr, 'ERROR: Only one of --group-files or --ahf-particle-files should be given'
        sys.exit(2)

    if len(tipsy_files) != len(ginfo_files):
        print >>sys.stderr, 'ERROR: The number of files in each group is not the same.'
        sys.exit(1)

    tipsy_files.sort(reverse=True)
    ginfo_files.sort(reverse=True)
    if group_files: 
        if len(group_files) != len(tipsy_files):
            print >>sys.stderr, 'ERROR: The number of files in each group is not the same.'
            sys.exit(1)
        group_files.sort(reverse=True)

    if ahf_particle_files: 
        if len(ahf_particle_files) != len(tipsy_files):
            print >>sys.stderr, 'ERROR: The number of files in each group is not the same.'
            sys.exit(1)
        ahf_particle_files.sort(reverse=True)

    if not os.path.exists(output_dir):
        try:
            os.makedirs(output_dir)
        except os.error:
            print >>sys.stderr, 'ERROR: Directory %s did not exist and could not be created.' % output_dir
            sys.exit(1)

    htrack_bin  = os.path.join(bin_dir, 'htrack')
    pfind_bin   = os.path.join(bin_dir, 'pfind')
    ahf2grp_bin = os.path.join(bin_dir, 'ahf2grp')

    if convert_group_files:
        group_files = []
        with open(os.path.join(output_dir, ahf2grp_xargs), 'w+') as fp:
            for t,grp in izip(tipsy_files, ahf_particle_files):
                t = os.path.relpath(t, output_dir)
                grp = os.path.relpath(grp, output_dir)
                out = subpathext(grp, '', '.grp')
                group_files.append(out)
                s = '-o "%s" --tipsy-file="%s" "%s"' % (out, t, grp)
                print >>fp, s
            nargs = len(shlex.split(s))
            print 'xargs -a "%s" -n%i -P%i %s %s' % (ahf2grp_xargs, nargs, nprocs, ahf2grp_bin, ahf2grp_opts)


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
