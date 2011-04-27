import sys
from os.path import basename
from pytipsy import ifTipsy, TipsyHeader
try:
    from sqlite3 import dbapi2 as sql
    from sqlite3.dbapi2 import connect as sql_connect
except ImportError: 
    from sqlite import main as sql
    from sqlite import connect as sql_connect

#=============================================================================
# Types
#=============================================================================

COSMOLOGICAL = 1
NONCOSMOLOGICAL = 2


create_table_header = '''\
CREATE TABLE IF NOT EXISTS header (
    name        VARCHAR(50),
    format      VARCHAR(50),
    version     INTEGER,
    revision    INTEGER,
    type        INTEGER,
    Omega0      REAL,
    OmegaLambda REAL,
    Hubble      REAL,
    ndims       INTEGER,
    nsnapshots  INTEGER,
    nsteps      INTEGER
)'''

create_table_snapinfo = '''\
CREATE TABLE IF NOT EXISTS snapinfo (
    snap_id     INTEGER PRIMARY_KEY,
    snapshot    INTEGER,
    redshift    REAL,
    time        REAL,
    nfiles      INTEGER,
    npart       INTEGER,
    nspecies    INTEGER,
    filename    VARCHAR(256)
)'''

create_table_stat = '''\
CREATE TABLE IF NOT EXISTS stat (
    snap_id     INTEGER,
    gid         INTEGER,
    npart       INTEGER,
    nvpart      REAL,
    Xc          REAL,
    Yc          REAL,
    Zc          REAL,
    VXc         REAL,
    VYc         REAL,
    VZc         REAL,
    Mvir        REAL,
    Rvir        REAL,
    Vmax        REAL,
    Rmax        REAL,
    sigV        REAL,
    lambda      REAL,
    Lx          REAL,
    Ly          REAL,
    Lz          REAL,
    a           REAL,
    Eax         REAL,
    Eay         REAL,
    Eaz         REAL,
    b           REAL,
    Ebx         REAL,
    Eby         REAL,
    Ebz         REAL,
    c           REAL,
    Ecx         REAL,
    Ecy         REAL,
    Ecz         REAL,
    ovdens      REAL,
    Redge       REAL,
    nbins       INTEGER,
    Ekin        REAL,
    Epot        REAL,
    mbp_offset  REAL,
    com_offset  REAL,
    r2          REAL,
    lambdaE     REAL,

    CONSTRAINT  FK1 FOREIGN KEY (snap_id) REFERENCES snapinfo (snap_id)
)'''

create_table_tracks = '''\
CREATE TABLE IF NOT EXISTS tracks (
    id          INTEGER,
    snap_id     INTEGER,
    gid         INTEGER,

    CONSTRAINT  FK1 FOREIGN KEY (snap_id) REFERENCES snapinfo (snap_id)
)'''

create_index_tracks = '''\
CREATE INDEX track_index ON tracks (id ASC)'''

create_index_stat = '''\
CREATE UNIQUE INDEX stat_index ON stat (snap_id ASC, gid ASC)'''

if __name__ == '__main__':

    connection = sql_connect('Hires.db')

    cursor = connection.cursor()

    cursor.execute(create_table_header)
    cursor.execute(create_table_snapinfo)
    cursor.execute(create_table_stat)
    cursor.execute(create_table_tracks)
    cursor.execute(create_index_tracks)
    cursor.execute(create_index_stat)

    f0 = open(sys.argv[1])
    for i,line in enumerate(f0):
        snap, stat = line.split()

        snapno = snap.split('.')[1]
        print basename(snap)

        #=====================================================================
        # Insert snapshot info
        #=====================================================================
        inp = ifTipsy()
        inp.open(snap, 'standard')
        h = inp.read()

        cmd = 'INSERT INTO snapinfo VALUES (NULL, %s, %f, %f, %i, %i, %i, "%s")' % \
            (snapno, 1/h.h_time - 1, h.h_time, 1, h.h_nBodies, 3, basename(snap))
        #print cmd
        cursor.execute(cmd)

        inp.close()

        #=====================================================================
        # Insert stat file
        #=====================================================================
        f1 = open(stat, 'r')
        gid=1
        for j,line in enumerate(f1):
            if line.startswith('#'): continue
            line = line.split()
            line = ", ".join(line)
            cmd = 'INSERT INTO stat VALUES (%i, %i, %s)' % (i+1,gid,line)
            #print cmd
            cursor.execute(cmd)
            gid+=1
        f1.close()

    #=========================================================================
    # Insert header info
    #=========================================================================
    cursor.execute('INSERT INTO header VALUES ("%s", "%s", %i, %i, %i, %f, %f, %f, %i, %i, %i)' % \
        ('Hires', 'skid', 0, 0, NONCOSMOLOGICAL, 0,0,0, 3, i+1, 200))

    #=========================================================================
    # Save all our changes
    #=========================================================================
    connection.commit()

    f0.close()

    if len(sys.argv) == 3:
        f0  = open(sys.argv[2])
        rows,cols = map(int, f0.readline().split()) # get dimensions
        #cmd = create_table_track(cols)
        #print cmd
        #cursor.execute('DROP TABLE IF EXISTS track')
        #cursor.execute(cmd)

        for i,line in enumerate(f0):
            tracks = map(lambda x: x.split(',')[0], line.split())
            tracks.reverse()
            for j,t in enumerate(tracks):
                if t == '0': t = 'NULL'
                cmd = 'INSERT INTO tracks VALUES (%i, %i, %s)' % (i+1, j+1, t)
                #print cmd
                cursor.execute(cmd)

        connection.commit()
    else:
        for g in range(1,gid):
            cmd = 'INSERT INTO tracks VALUES (%i, %i, %s)' % (g, 1, g)
            cursor.execute(cmd)

        connection.commit()


    #sels = map(lambda x: 'SELECT Mvir FROM stat,track WHERE (stat.snap_id=%i and stat.gid=track.S%i)' % (x,x), range(1,33))
    #cmd = ' UNION '.join(sels)
    #print cmd

