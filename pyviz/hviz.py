import sys
from sqlite3 import dbapi2 as sql

from environment import *
from display import display_start

def load_halos(db):

    connection = sql.connect(db)
    connection.row_factory = sql.Row
       
    cursor = connection.cursor()

    cursor.execute('SELECT format FROM header')
    format = cursor.fetchone()[0]

    env.mt = None

    if format == 'skid':
        load_skid(cursor)
    elif format == 'AHF':
        load_ahf(cursor)

def load_skid(cursor):

    cursor.execute('SELECT min(X),max(X),min(Y),max(Y),min(Z),max(Z), max(GlobalId) FROM stat')
    row = cursor.fetchone()
    xmin,xmax, ymin,ymax, zmin,zmax = row[0],row[1], row[2],row[3], row[4],row[5]
    env.max_x = max(max(xmax-xmin, ymax-ymin), zmax-zmin)

    cursor.execute('SELECT min(VX),max(VX),min(VY),max(VY),min(VZ),max(VZ) FROM stat')
    row = cursor.fetchone()
    vxmin,vxmax, vymin,vymax, vzmin,vzmax = row[0],row[1], row[2],row[3], row[4],row[5]
    env.max_v = max(max(vxmax-vxmin, vymax-vymin), vzmax-vzmin)

    if env.max_x == 0: env.max_x = 1;
    if env.max_v == 0: env.max_v = 1;

    cx = (xmax+xmin) / 2
    cy = (ymax+ymin) / 2
    cz = (zmax+zmin) / 2

          #'Radius/%(maxx)f  AS Radius,'   + \
          #'Radius  AS Radius,'   + \
    cmd =('SELECT '                           +
          'snap_id,'                          +
          'GlobalId             AS gid,'      +
          'nTotal               AS npart,'    +
          'Mass,'                             +
          'GasMass,'                          +
          'StarMass,'                         +
          'AvgDens,'                          +
          'Radius/%(maxx)f    AS Radius,'   +
          'DeltaR2,'                          +
          'VelDisp,'                          +
          'VelSigma2X,'                       +
          'VelSigma2Y,'                       +
          'VelSigma2Z,'                       +
          '(X - %(cx)f)/%(maxx)f  AS X,'      +
          '(Y - %(cy)f)/%(maxx)f  AS Y,'      +
          '(Z - %(cz)f)/%(maxx)f  AS Z,'      +
          'RpotminX,'                         +
          'RpotminY,'                         +
          'RpotminZ,'                         +
          'RdenmaxX,'                         +
          'RdenmaxY,'                         +
          'RdenmaxZ,'                         +
          'VX/%(maxv)f  AS VX,'      +
          'VY/%(maxv)f  AS VY,'      +
          'VZ/%(maxv)f  AS VZ,'      +
          'VcircMax,'                         +
          'RVcircMax,'                        +
          'Rvir,'                             +
          'Mvir,'                             +
          'lambda               AS Lambda '   +
          'FROM stat ORDER BY snap_id ASC') % {'cx':cx, 'cy':cy, 'cz':cz, 'maxx':env.max_x, 'maxv':env.max_v}
          #'FROM stat where Mass between 8e-7 and 2e-6 ORDER BY snap_id ASC') % {'cx':cx, 'cy':cy, 'cz':cz, 'maxx':env.max_x, 'maxv':env.max_v}
          #'FROM stat where Mass > 2e-8 ORDER BY snap_id ASC') % {'cx':cx, 'cy':cy, 'cz':cz, 'maxx':env.max_x}
    cmd =('SELECT '                           +
          'snap_id,'                          +
          'GlobalId             AS gid,'      +
          'nTotal               AS npart,'    +
          'Mass,'                             +
          'Radius/%(maxx)f    AS Radius,'   +
          '(X - %(cx)f)/%(maxx)f  AS X,'      +
          '(Y - %(cy)f)/%(maxx)f  AS Y,'      +
          '(Z - %(cz)f)/%(maxx)f  AS Z,'      +
          'VX/%(maxv)f  AS VX,'      +
          'VY/%(maxv)f  AS VY,'      +
          'VZ/%(maxv)f  AS VZ '      +
          'FROM stat ORDER BY snap_id ASC') % {'cx':cx, 'cy':cy, 'cz':cz, 'maxx':env.max_x, 'maxv':env.max_v}
          #'FROM stat where snap_id < 5 ORDER BY snap_id ASC') % {'cx':cx, 'cy':cy, 'cz':cz, 'maxx':env.max_x, 'maxv':env.max_v}
    print cmd

    print "Selecting halos..."
    o = cursor.execute(cmd)

    columns = map(lambda x: x[0], o.description)

    oldt = 0 
    env.halos.append([None])
#    s = '''\
#class Halo
#    def __init__(self, row):
#        self.row = row
#    def __getattribute__(self, name):
#        return self.row[name]
#'''

#    for i,col in enumerate(columns):
#        s += "def %h.%s = row[%i]; " % (col, i)
        #s += "h.%s = row[%i]; " % (col, i)
    #print s
    print "Loading halos...",
    for row in cursor:
        t = row['snap_id']
        i = row['gid']

        #t,i = row[0:2]

        #print t,i

        if t != oldt:
            print t
            oldt = t
            env.halos.append([[]])

        h = Halo(row)
        h.a, h.b, h.c = 1,1,1

#        exec s

        try:
            env.halos[t].append(h)
        except:
            print len(env.halos), t
            sys.exit(1)
    print "Done."

    print "Selecting tracks..."
    cursor.execute('SELECT id,snap_id,gid FROM tracks ORDER BY id,snap_id ASC')
    print "Loading tracks...",
    env.mt = [None]
    oldt = 0
    for row in cursor:
        track = row['id']
        #t     = row['snap_id']
        #gid   = row['gid']

        if track != oldt:
            oldt = track
            env.mt.append([[]])
            #env.mt.append([[]] * 33)

        env.mt[track].append(row)
    print "Done."

def load_ahf(cursor):

    cursor.execute('SELECT min(Xc),max(Xc),min(Yc),max(Yc),min(Zc),max(Zc) FROM stat')
    row = cursor.fetchone()
    xmin,xmax, ymin,ymax, zmin,zmax = row[0],row[1], row[2],row[3], row[4],row[5]
    env.max_x = max(max(xmax-xmin, ymax-ymin), zmax-zmin)

    cursor.execute('SELECT min(VXc),max(VXc),min(VYc),max(VYc),min(VZc),max(VZc) FROM stat')
    row = cursor.fetchone()
    vxmin,vxmax, vymin,vymax, vzmin,vzmax = row[0],row[1], row[2],row[3], row[4],row[5]
    env.max_v = max(max(vxmax-vxmin, vymax-vymin), vzmax-vzmin)

    if env.max_x == 0: env.max_x = 1;
    if env.max_v == 0: env.max_v = 1;

    env.max_x /= 2
    env.max_v /= 2

    cx = (xmax+xmin) / 2
    cy = (ymax+ymin) / 2
    cz = (zmax+zmin) / 2

    print cx, cy, cz, env.max_x

    o = \
    cursor.execute(('SELECT snap_id,gid,npart,nvpart,' +
                   '(Xc - %(cx)f)/%(maxx)f AS X,'      +
                   '(Yc - %(cy)f)/%(maxx)f AS Y,'      +
                   '(Zc - %(cz)f)/%(maxx)f AS Z,'      +
                   'VXc*1e-3             AS VX,'       +
                   'VYc*1e-3             AS VY,'       +
                   'VZc*1e-3             AS VZ,'       +
                   'Mvir                 AS Mass,'     +
                   'Rvir/%(maxx)f*0.001    AS Radius,'   +
                   'Vmax,'                             +
                   'Rmax/%(maxx)f*0.001  AS Rmax,'     +
                   'sigV,'                             +
                   'lambda               AS Lambda,'   +
                   'Lx,'                               +
                   'Ly,'                               +
                   'Lz,'                               +
                   'a,'                                +
                   'Eax,'                              +
                   'Eay,'                              +
                   'Eaz,'                              +
                   'b,'                                +
                   'Ebx,'                              +
                   'Eby,'                              +
                   'Ebz,'                              +
                   'c,'                                +
                   'Ecx,'                              +
                   'Ecy,'                              +
                   'Ecz,'                              +
                   'ovdens,'                           +
                   'Redge,'                            +
                   'nbins,'                            +
                   'Ekin,'                             +
                   'Epot,'                             +
                   'mbp_offset,'                       +
                   'com_offset,'                       +
                   'r2,'                               +
                   'lambdaE '                          +
                   'FROM stat ORDER BY snap_id ASC') % {'cx':cx, 'cy':cy, 'cz':cz, 'maxx':env.max_x})

    columns = map(lambda x: x[0], o.description)
    oldt = 0 
    env.halos.append([None])

    print "Loading halos...",
    for row in cursor:
        t = row['snap_id']
        i = row['gid']

        #t,i = row[0:2]

        #print t,i

        if t != oldt:
            print t
            oldt = t
            env.halos.append([[]])

        h = Halo(row)
        #h.a, h.b, h.c = 1,1,1

#        exec s

        try:
            env.halos[t].append(h)
        except:
            print len(env.halos), t
            sys.exit(1)
    print "Done."

    print "Selecting tracks..."
    cursor.execute('SELECT id,snap_id,gid FROM tracks ORDER BY id,snap_id ASC')
    print "Loading tracks...",
    env.mt = [None]
    oldt = 0
    for row in cursor:
        track = row['id']
        #t     = row['snap_id']
        #gid   = row['gid']

        if track != oldt:
            oldt = track
            env.mt.append([[]])
            #env.mt.append([[]] * 33)

        env.mt[track].append(row)
    print "Done."

#   oldt = 0 
#   env.halos.append([None])
#   for row in cursor:
#       t = row['snap_id']
#       i = row['gid']

#       if t != oldt:
#           oldt = t
#           env.halos.append([[]])

#       h = Halo()

#       for col in columns:
#           exec "h.%s = row[col]" % col

#       env.halos[t].append(h)

#   cursor.execute('SELECT id,snap_id,gid FROM tracks ORDER BY id,snap_id ASC')
#   env.mt = [None]
#   oldt = 0
#   for row in cursor:
#       track = row['id']
#       t     = row['snap_id']
#       gid   = row['gid']

#       if track != oldt:
#           oldt = track
#           env.mt.append([[]] * 2)
#           #env.mt.append([[]] * 33)

#       env.mt[track][t] = row

#   env.eye.x = env.eye.ox = 0
#   env.eye.y = env.eye.oy = 0
#   env.eye.z = env.eye.oz = 0

    return 0

#============================================================================
#                                    help
#============================================================================
def help():
    printf >>stderr, "hviz"
    sys.exit(1)

#============================================================================
#                                    main
#============================================================================
if __name__ == '__main__':

    if len(sys.argv) != 2: help()

    env.t = 1
    env.background = BLACK
    env.fullscreen = 0
    env.screenWidth = 1024
    env.screenHeight = 768
    env.eye.x = env.eye.ox = 0.840533 
    env.eye.y = env.eye.oy = 0.464536
    env.eye.z = env.eye.oz = 0.568467

    env.eye.tx = 0
    env.eye.ty = 0
    env.eye.tz = -1
    env.eye.ux = 0
    env.eye.uy = 1
    env.eye.uz = 0

    env.make_movie = 0
    env.movie_prefix = None
    env.frame_buffer = None
    env.current_movie_frame = 0
    env.mode = MODE_TRACK | MODE_HALOBODIES
    #env.mode = MODE_HALOBODIES
    env.track_id = 0
    env.pointto_id = [9961,350433]
    #env.track_id = 414
    env.mouse_down = 0
    env.dirty = 1
    env.max_level = 0
    env.level = 0

    if env.screenWidth <= 0 or env.screenHeight <= 0: help()

    load_halos(sys.argv[1])
    #load_halos('halo3.db')

    env.level = env.max_level

    display_start(sys.argv) # never returns

