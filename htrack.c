#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include "htrack.h"
#include "jpcmacros.h"
#include "getline.h"

#define DELIM                       " \n\r"

#define INVALID_GROUP_ID            ULONG_MAX

#define FMT_6DFOF    1
#define FMT_AHF     2
#define FMT_SKID     3

int verbosity = 0;

int read_ahf_groups(FILE *in, group_t **groups0, uint64_t *n_groups0);
int find_progenitors(FILE *in, group_t *D, uint64_t nD);
void help();

//============================================================================
//                          parse_gid_and_belonging
//============================================================================
uint64_t parse_gid_and_belonging(group_t *d, char *s)
{
    char *endptr;

    uint64_t gid = strtol(s, &endptr, 10);
    if (gid != 0)
    {
        while (*endptr != '\0')
        {
            s = endptr+1;
            uint64_t id = strtol(s, &endptr, 10);
            if (s == endptr) break;
            list_append(&d[gid].belong, id);
        }
    }

    return gid;
}

//============================================================================
//                              find_progenitors
//============================================================================
int find_progenitors(FILE *in, group_t *D, uint64_t nD)
{
    int i,j;
    char *line = NULL;
    size_t len;
    uint64_t p;
    uint64_t maxp=0;
    static uint64_t allocd = 0;

    list_t order = EMPTY_LIST;

    static char *used = NULL;

    //------------------------------------------------------------------------
    // Parse the input. Each row has three main sections: 
    //  (1) Group ID, (2) Number of progenitors, (3) List of progenitors.
    // Each progenitor in section 3 may consist of a comma-separated list
    // of values (no spaces). We are only interested in the first value,
    // which is the id of the progenitor.
    //------------------------------------------------------------------------
    int errors_found = 0;
    while (!feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        uint64_t gid   = parse_gid_and_belonging(D, strtok(line, DELIM));
        uint64_t nprog = atol(strtok(NULL, DELIM));

#if 1
        if (gid > nD || D[gid].id == INVALID_GROUP_ID)
        {
            eprintf("Group %ld doesn't exist in stat file\n", gid);
            errors_found = 1;
            continue;
        }
#endif

        ERRORIF(D[gid].id == INVALID_GROUP_ID, "Group %ld doesn't exist in stat file\n", gid);

        if (gid == 0) continue;

        list_append(&order, gid);

        // Read progenitor id's
        for (i=0; i < nprog; i++)
        {
            p = atol(strtok(NULL, DELIM));
            if (p > maxp) maxp = p;
            list_append(&D[gid].ps, p);
        }
    }

#if 1
    if (errors_found) return 0;
#endif

    ERRORIF(order.len == 0, "Empty progenitor file.");

    if (maxp >= allocd)
    {
        if (allocd == 0) allocd = 2048;
        while (maxp >= allocd) allocd *= 2;
        used = REALLOC(used, char, allocd+1);
    }

    MEMSET(used, 0, maxp+1, char);

    //------------------------------------------------------------------------
    // For each decendent we find the first progenitor that passes all
    // acceptance criteria.  If we accept the progenitor, then it is added to a
    // list used groups so that it will not be considered again. Decendents are
    // considered in the order they appear in the input file.
    //------------------------------------------------------------------------
    for (i=0; i < order.len; i++) 
    { 
        uint64_t gid = order.v[i];

        group_t *d = &D[gid];

        d->pid = 0;

        // Find acceptable progenitor
        for (j=0; j < d->ps.len; j++)
        {
            p = d->ps.v[j];

            if (!used[p])
            {
                used[p] = 1;
                d->pid = p;
                break;
            }
        }

        assert(d->id  != INVALID_GROUP_ID);
        assert(d->pid != INVALID_GROUP_ID);
    }

    if (line != NULL) free(line);

    return 0;
}

//============================================================================
//                                build_tracks
//============================================================================
int build_tracks(z_t *zs, uint64_t n_zs, track_t **tracks0, uint64_t *n_tracks0)
{
    track_t *tracks = NULL;
    size_t allocd = 0;
    size_t allocd_guess = 1<<20;

    int i,j,k;                                                                      
    uint64_t next, prev;
    int cur_track=0;                                                           

    z_t *Z = zs;

    for (k=0; k < n_zs; k++) 
        for (j=1; j <= zs[k].n_groups; j++) 
        {
            zs[k].used[j] = 0; 
            if (zs[k].g[j].id != INVALID_GROUP_ID)
                ERRORIF(zs[k].g[j].pid == INVALID_GROUP_ID, "%i %i %ld %i", k, j, zs[k].g[j].id, zs[k].n_groups);
        }

    for (k=0; k < n_zs; k++, Z++)
    {
        for (j=1; j <= Z->n_groups; j++)
        {
            if (Z->g[j].id == INVALID_GROUP_ID) continue;
            if (Z->used[j]) continue;

            if (cur_track == allocd)
            {
                if (allocd == 0) allocd = allocd_guess;
                else allocd *= 2;

                tracks = REALLOC(tracks, track_t, allocd);
                memset(tracks+cur_track, 0, allocd-cur_track);
            }


            tracks[cur_track].t = CALLOC(uint64_t, n_zs);

            z_t *Zt = Z;
            next    = Zt->g[j].id;
            prev    = next;

            for (i=k; next != 0; Zt++, i++)
            {
                //if (j == 16836) eprintf("-> %ld\n", next);
                //if (next == INVALID_GROUP_ID) break;
                //if (next == 0) break;

                //eprintf("%i %i/%i %i %i %ld\n", k, j, Z->n_groups, i, cur_track, next);
                //eprintf("%i %i %i %i %ld\n", k, j, i, cur_track, next);
                //assert(next != INVALID_GROUP_ID);
                ERRORIF(next == INVALID_GROUP_ID, "%i %i %ld %i", k, j, prev, i);

                tracks[cur_track].t[i] = next;

                Zt->used[next] = 1;
                prev = next;
                next = Zt->g[next].pid;
            }

            cur_track++;
        }
    }
    VL(2) fprintf(stderr, "n_tracks=%i\n", cur_track);

    *tracks0   = tracks;
    *n_tracks0 = cur_track;

    return 0;
}

//============================================================================
//                              progenitor_groups
//============================================================================
int progenitor_groups(z_t *D, group_t **groups0, uint64_t *n_groups0)
{
    int i;
    uint64_t n_groups=0;
    uint64_t allocd = 0;
    group_t *groups = NULL;

    fprintf(stderr, "BEGIN progenitor_groups\n");

    for (i=0; i < D->n_groups; i++)
    {
        fprintf(stderr, "copying group %ld (%i)\n", D->g[i].pid, i);

        if (D->g[i].pid > n_groups)
            n_groups = D->g[i].pid;

        fprintf(stderr, "n_groups is %ld\n", n_groups);

        if (n_groups >= allocd)
        {
            do
            {
                if (allocd == 0) allocd = 32;
                else allocd *= 2;
            } while (n_groups >= allocd);

            groups = REALLOC(groups, group_t, allocd+1);
            ERRORIF(groups == NULL, "No memory for groups.");
            memset(groups + n_groups+1, 0, (allocd-n_groups) * sizeof(group_t));
        }

        groups[D->g[i].pid].id  = D->g[i].pid;
        groups[D->g[i].pid].pid = 0;
    }

    if (n_groups > 0)
    {
        groups[0].id   = 0;
        groups[0].pid  = INVALID_GROUP_ID;
    }

    fprintf(stderr, "END progenitor_groups\n");

    *n_groups0 = n_groups;
    *groups0   = groups;

    return 0;
}


//============================================================================
//                                WRITE_MATRIX
//============================================================================
#define WRITE_MATRIX(fname, _prop, _fmt)                                    \
int fname(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)      \
{                                                                           \
    int i,t;                                                                \
    fprintf(out, "%i %i\n", n_tracks, n_zs);                                \
    for (t=0; t < n_tracks; t++)                                            \
    {                                                                       \
        for (i=0; i < n_zs; i++)                                            \
            fprintf(out, _fmt, zs[i].g[tracks[t].t[i]]._prop);              \
        fprintf(out, "\n");							                        \
    }                                                                       \
    return 0;                                                               \
}

int write_matrix(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks, 
                 void (*f)(FILE*, group_t*))
{
    int i,t;

    fprintf(out, "%i %i\n", n_tracks, n_zs);
    for (t=0; t < n_tracks; t++)
    {
        int seen = 0;
        for (i=0; i < n_zs; i++)
        {
            int q = tracks[t].t[i];
            group_t *g = &zs[i].g[q];
            if (g->id == INVALID_GROUP_ID)
            {
                fprintf(stderr, "# %i %i %i\n", t, i, q);
                assert(g->id != INVALID_GROUP_ID);
            }

            if (g->id == 0 && seen) break;
            if (g->id != 0)
            {
                if (!seen)
                {
                    fprintf(out, "%i ", i);
                    seen = 1;
                }

                f(out, g);
            }
        }
        fprintf(out, "\n");
    }
    return 0;
}

//============================================================================
//                             write_pid_matrix
//============================================================================
void write_pid(FILE *out, group_t *g)
{
    int j;

    fprintf(out, "%ld", g->id);
    if (g->id != 0)
    {
        for (j=0; j < g->belong.len; j++)
            fprintf(out, ",%ld", g->belong.v[j]);
    }
    fprintf(out, " ");
}

int write_pid_matrix(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)
{
    fprintf(out, 
    "# Format:\n"
    "#     The first line gives the number of tracks that follow and the\n"
    "#     maximum number of track columns. The total number of columns\n"
    "#     may be one more that the number of tracks as described below.\n"
    "#\n"
    "#     Each subsequent line has the format\n"
    "#\n"
    "#         START GID [GID ...]\n"
    "#\n"
    "#     where each GID corresponds to an output.\n"
    "#\n"
    "#     START is the first track column where a group appears. Some\n"
    "#     groups may not appear until earlier in time, thus START will\n"
    "#     be greater than zero.\n"
    "#\n"
    "#     GID is the group id in a particular output.\n"
    "#\n"
    );

    return write_matrix(out, zs, n_zs, tracks, n_tracks, write_pid);
}

//============================================================================
//                             write_mass_matrix
//============================================================================
void write_mass(FILE *out, group_t *g)
{
    fprintf(out, "%.8g ", g->mass);
}

int write_mass_matrix(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)
{
    fprintf(out, 
    "# Format:\n"
    "#     The first line gives the number of tracks that follow and the\n"
    "#     maximum number of track columns. The total number of columns\n"
    "#     may be one more that the number of tracks as described below.\n"
    "#\n"
    "#     Each subsequent line has the format\n"
    "#\n"
    "#         START MASS [GID ...]\n"
    "#\n"
    "#     where each MASS corresponds to an output.\n"
    "#\n"
    "#     START is the first track column where a group appears. Some\n"
    "#     groups may not appear until earlier in time, thus START will\n"
    "#     be greater than zero.\n"
    "#\n"
    );

    return write_matrix(out, zs, n_zs, tracks, n_tracks, write_mass);
}

//============================================================================
//                             write_vmax_matrix
//============================================================================
void write_v(FILE *out, group_t *g)
{
    fprintf(out, "%.8g,%.8g,%.8g ", g->v[0], g->v[1], g->v[2]);
}

int write_v_matrix(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)
{
    fprintf(out, 
    "# Format:\n"
    "#     The first line gives the number of tracks that follow and the\n"
    "#     maximum number of track columns. The total number of columns\n"
    "#     may be one more that the number of tracks as described below.\n"
    "#\n"
    "#     Each subsequent line has the format\n"
    "#\n"
    "#         START vx,vy,vz [vx,vy,vz ...]\n"
    "#\n"
    "#     where each vx,vy,vz corresponds to an output.\n"
    "#\n"
    "#     START is the first track column where a group appears. Some\n"
    "#     groups may not appear until earlier in time, thus START will\n"
    "#     be greater than zero.\n"
    "#\n"
    );

    return write_matrix(out, zs, n_zs, tracks, n_tracks, write_v);
}

//============================================================================
//                               write_R_matrix
//============================================================================
void write_r(FILE *out, group_t *g)
{
    fprintf(out, "%.8g,%.8g,%.8g ", g->r[0], g->r[1], g->r[2]);
}

int write_r_matrix(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)
{
    fprintf(out, 
    "# Format:\n"
    "#     The first line gives the number of tracks that follow and the\n"
    "#     maximum number of track columns. The total number of columns\n"
    "#     may be one more that the number of tracks as described below.\n"
    "#\n"
    "#     Each subsequent line has the format\n"
    "#\n"
    "#         START x,y,z [x,y,z ...]\n"
    "#\n"
    "#     where each x,y,z corresponds to an output.\n"
    "#\n"
    "#     START is the first track column where a group appears. Some\n"
    "#     groups may not appear until earlier in time, thus START will\n"
    "#     be greater than zero.\n"
    "#\n"
    );

    return write_matrix(out, zs, n_zs, tracks, n_tracks, write_r);
}

//============================================================================
//                               calc_R
//
// Computes the distance of each halo from the halo considered to be halo 1
// at each redshift.
//============================================================================
#if 0
int calc_R(z_t *zs, int n_zs, track_t *tracks, int n_tracks)
{                                                                                   
    int i, t;
    for (t=0; t < n_tracks; t++ )
    {
        for (i=0; i < n_zs; i++)
        {
            float x = zs[i].g[tracks[0].t[i]].r[0];
            float y = zs[i].g[tracks[0].t[i]].r[1];
            float z = zs[i].g[tracks[0].t[i]].r[2];

            /* Check that the next halo isn't just the field */
            uint64_t n = tracks[t].t[i];
            if (n)
                zs[i].g[n].R = 
                    sqrt( pow(x - zs[i].g[n].r[0], 2)
                        + pow(y - zs[i].g[n].r[1], 2)
                        + pow(z - zs[i].g[n].r[2], 2));

        }
    }
    return 0;
}
#endif

//============================================================================
//                              read_ahf_groups
//============================================================================
int read_ahf_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int read;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading AHF format group file.\n");

    uint64_t n_groups=0;
    uint64_t allocd = 0;
    group_t *groups = NULL;

    while (!feof(in))
    {
        /* Read the whole line */
        read = getline(&line, &len, in);
        if (read <= 0 || line[0] == '#') continue;

        if (n_groups == allocd)
        {
            if (allocd == 0) allocd = 32;
            else allocd *= 2;

            groups = REALLOC(groups, group_t, allocd+1);
            ERRORIF(groups == NULL, "No memory for groups.");
            memset(groups + n_groups+1, 0, (allocd-n_groups) * sizeof(group_t));
        }

        n_groups++;

        /* Now extract just the first 11 values */
#if 0
        read = 
            sscanf(line, "%d %d %g %g %g %g %g %g %g %g %g",
                /* No. Particles    (1)   */
                &di,            
                &di,
                /* Position         (3-5) */
                &groups[n_groups].r[0],&groups[n_groups].r[1],&groups[n_groups].r[2],
                /* Velocity         (6-8) */
                &groups[n_groups].v[0],&groups[n_groups].v[1],&groups[n_groups].v[2],
                /* Mass             (9)   */
                &groups[n_groups].Mvir,
                &df,
                /* Vmax             (11)  */
                &groups[n_groups].vMax
                );
        ERRORIF(read != 11, "Missing columns. Expected at least 11.");
        ERRORIF(groups[n_groups].Mvir <= 0, "Group %ld has bad mass %f in group file.", n_groups, groups[n_groups].Mvir);
        groups[n_groups].R   = 0;
#endif

        groups[n_groups].id  = n_groups;
        groups[n_groups].pid = INVALID_GROUP_ID;
    }

    if (n_groups > 0)
    {
        groups[0].id   = 0;
        groups[0].pid  = INVALID_GROUP_ID;
#if 0
        groups[0].R    = 0;
        groups[0].Mvir = 0;
#endif
    }

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups;
    *groups0   = groups;

    return 0;
}

//==============================================================================
//                              read_6dfof_groups
//==============================================================================
int read_6dfof_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int read;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading 6DFOF format group file.\n");

    uint64_t n_groups=0;
    group_t *groups = NULL;
    uint64_t id, i;
    uint64_t allocd = 0;
    uint64_t t0;
    float t;
    float mass;
    float x,y,z;
    float vx,vy,vz;

    while (!feof(in))
    {
        /* Read the whole line */
        read = getline(&line, &len, in);
        if (read <= 0 || line[0] == '#') continue;

        //read = sscanf(line, "%ld %ld %f", &id, &dummy, &mass);
        read = sscanf(line, 
            "%ld %ld %g %g %g %g %g %g %g %g %g %g %g",
            &id, &t0, &mass, 
            &t, &t, &t, &t, 
            &x,&y,&z,
            &vx,&vy,&vz
        );
        ERRORIF(read != 13, "Missing columns. Expected at least 13.");

        //--------------------------------------------------------------------
        // If the group file lists an "unbounded" group, skip it.
        //--------------------------------------------------------------------
        if (id == 0) continue;

        if (id > allocd) 
        {
            uint64_t n = allocd;
            if (n == 0) n = 2048;
            while (n < id) n *= 2;

            groups = REALLOC(groups, group_t, n+1);
            ERRORIF(groups == NULL, "No memory for mass list.");
            MEMSET(groups + allocd+1, 0, n-allocd, group_t);

            for (i=allocd+1; i <= n; i++)
            {
                groups[i].id  = INVALID_GROUP_ID;
                groups[i].pid = INVALID_GROUP_ID;
            }

            allocd = n;
        }

        if (id > n_groups) n_groups = id;
        groups[id].id = id;
        groups[id].mass = mass;
        groups[id].r[0] = x;
        groups[id].r[1] = y;
        groups[id].r[2] = z;
        groups[id].v[0] = vx;
        groups[id].v[1] = vy;
        groups[id].v[2] = vz;
        //--------------------------------------------------------------------
        // Group progenitors default to the field.
        //--------------------------------------------------------------------
        groups[id].pid = 0;

#if 0
        groups[id].R   = 0;
        groups[id].id  = n_groups;
        groups[id].pid = 0;
#endif
    }

    assert(ferror(in) == 0);

    if (groups == NULL)
    {
        WARNIF(1, "Stat file has no groups.");
        groups = REALLOC(groups, group_t, 1);
        ERRORIF(groups == NULL, "No memory for mass list.");
    }

    MEMSET(groups, 0, 1, group_t);
    groups[0].id   = 0;
    groups[0].pid  = INVALID_GROUP_ID;

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups;
    *groups0   = groups;

    return 0;
}
#if 0
int read_6dfof_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int read;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading 6DFOF format group file.\n");

    uint64_t n_groups=0;
    group_t *groups = NULL;
    uint64_t id, i;
    uint64_t allocd = 0;

    while (!feof(in))
    {
        /* Read the whole line */
        read = getline(&line, &len, in);
        if (read <= 0 || line[0] == '#') continue;

        /* Now extract just the id */
        read = sscanf(line, "%ld", &id);
        ERRORIF(read != 1, "Missing columns. Expected at least 1.");

        //--------------------------------------------------------------------
        // If the group file lists an "unbounded" group, skip it.
        //--------------------------------------------------------------------
        if (id == 0) continue;

        if (id > allocd) 
        {
            uint64_t n = allocd;
            if (n == 0) n = 2048;
            while (n < id) n *= 2;

            groups = REALLOC(groups, group_t, n+1);
            ERRORIF(groups == NULL, "No memory for mass list.");
            MEMSET(groups + allocd+1, 0, n-allocd, group_t);

            for (i=allocd+1; i <= n; i++)
            {
                groups[i].id  = INVALID_GROUP_ID;
                groups[i].pid = INVALID_GROUP_ID;
            }

            allocd = n;
        }

        if (id > n_groups) n_groups = id;
        groups[id].id = id;
        //--------------------------------------------------------------------
        // Group progenitors default to the field.
        //--------------------------------------------------------------------
        groups[id].pid = 0;

#if 0
        groups[id].R   = 0;
        groups[id].id  = n_groups;
        groups[id].pid = 0;
#endif
    }

    assert(ferror(in) == 0);

    if (groups == NULL)
    {
        WARNIF(1, "Stat file has no groups.");
        groups = REALLOC(groups, group_t, 1);
        ERRORIF(groups == NULL, "No memory for mass list.");
    }

    MEMSET(groups, 0, 1, group_t);
    groups[0].id   = 0;
    groups[0].pid  = INVALID_GROUP_ID;

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups;
    *groups0   = groups;

    return 0;
}
#endif

//
//============================================================================
//                              read_skid_groups
//============================================================================
int read_skid_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int read;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading SKID format group file.\n");

    uint64_t n_groups=0;
    group_t *groups = NULL;
    uint64_t id, i;
    uint64_t allocd = 0;
    uint64_t t0;
    float t;
    float mass;
    float x,y,z;
    float vx,vy,vz;

    while (!feof(in))
    {
        /* Read the whole line */
        read = getline(&line, &len, in);
        if (read <= 0 || line[0] == '#') continue;

        //read = sscanf(line, "%ld %ld %f", &id, &dummy, &mass);
        read = sscanf(line, 
            "%ld %ld %g %g %g %g %g %g %g %g %g %g %g %g %g %ld %g %g %g %g %g %g %g %g %g\n",
            &id, &t0, &mass,
            &t, &t, &t, &t, &t, &t, &t, &t, &t, &t, &t, &t, &t0,
            &t, &t, &t,
            &x,&y,&z,
            &vx,&vy,&vz
        );
        ERRORIF(read != 25, "Missing columns. Expected at least 25.");

        //--------------------------------------------------------------------
        // If the group file lists an "unbounded" group, skip it.
        //--------------------------------------------------------------------
        if (id == 0) continue;

        if (id > allocd) 
        {
            uint64_t n = allocd;
            if (n == 0) n = 2048;
            while (n < id) n *= 2;

            groups = REALLOC(groups, group_t, n+1);
            ERRORIF(groups == NULL, "No memory for mass list.");
            MEMSET(groups + allocd+1, 0, n-allocd, group_t);

            for (i=allocd+1; i <= n; i++)
            {
                groups[i].id  = INVALID_GROUP_ID;
                groups[i].pid = INVALID_GROUP_ID;
            }

            allocd = n;
        }

        if (id > n_groups) n_groups = id;
        groups[id].id = id;
        groups[id].mass = mass;
        groups[id].r[0] = x;
        groups[id].r[1] = y;
        groups[id].r[2] = z;
        groups[id].v[0] = vx;
        groups[id].v[1] = vy;
        groups[id].v[2] = vz;
        //--------------------------------------------------------------------
        // Group progenitors default to the field.
        //--------------------------------------------------------------------
        groups[id].pid = 0;

#if 0
        groups[id].R   = 0;
        groups[id].id  = n_groups;
        groups[id].pid = 0;
#endif
    }

    assert(ferror(in) == 0);

    if (groups == NULL)
    {
        WARNIF(1, "Stat file has no groups.");
        groups = REALLOC(groups, group_t, 1);
        ERRORIF(groups == NULL, "No memory for mass list.");
    }

    MEMSET(groups, 0, 1, group_t);
    groups[0].id   = 0;
    groups[0].pid  = INVALID_GROUP_ID;

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups;
    *groups0   = groups;

    return 0;
}

//============================================================================
//                                 read_work
//============================================================================
int read_work(FILE *in, work_t **work0, int *work_len0)
{
    int i;
    int allocd=0;
    work_t *work = NULL;

    char *line = NULL;
    size_t len;

    for (i=0; !feof(in); i++)
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        fprintf(stderr, line);

        if (i == allocd)
        {
            if (allocd == 0) allocd = 32; else allocd *= 2;
            work = REALLOC(work, work_t, allocd);
            assert(work != NULL);
        }

        const char *t = strtok(line, DELIM); 
        if (t == NULL) continue; 
        const float z  = atof(t);
        const char *f1 = strtok(NULL, DELIM);
        const char *f2 = strtok(NULL, DELIM);

        work[i].z = z;
        if (f1 != NULL) { work[i].stats = MALLOC(char, strlen(f1)+1); assert(work[i].stats != NULL); strcpy(work[i].stats, f1); }
        if (f2 != NULL) { work[i].pf    = MALLOC(char, strlen(f2)+1); assert(work[i].pf    != NULL); strcpy(work[i].pf,    f2); }
    }

    if (line != NULL) free(line);

    *work0     = work;
    *work_len0 = i-1;
    return 0;
}

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, 
    
    "Usage: htrack [OPTIONS] WORKFILE\n"
    "Generate halo tracks from files listed in WORKFILE.\n"
    "If WORKFILE is -, read from standard input.\n"
    "\n"
    "OPTIONS are\n"
    "\n"
    "      --ahf                    Group files are in AHF format.\n"
    "      --skid                   Group files are in SKID format.\n"
    "      --6dfof                  Group files are in 6DFOF format from PKDGRAV.\n"
    "\n"
    "\n"
    "  -w, --what=WHAT              What to output. WHAT may be any of 'aivmr'\n"
    "                               where 'i' means halo id,\n"
    "                                     'v' means vmax,\n"
    "                                     'm' means mass,\n"
    "                                     'r' means position relative to halo 1,\n"
    "                               and   'a' means all of the above.\n"
    "  -o, --output-prefix=PREFIX   Output files will be labeled with PREFIX.\n"
    "                               The default is 'htrack'.\n"
    "      --help                   Show this help text.\n"
    "\n"
    "\n"
    "WORKFILE must contain a list of files to work on. Each line has the format\n"
    "\n"
    "   <redshift> <stats file> <progenitor file>\n"
    "\n"
    "where <stats file> contains group information from a halo finder and\n"
    "<progenitor file> contains the lists of progenitors for each halo as\n"
    "determined by pfind. These lines should be sorted by increasing redshift.\n"
    "  \n"
    "Report bugs to <jonathan@physik.uzh.ch>\n"
    );
    exit(2);
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    int i;
    group_t *D = NULL;
    uint64_t nD = 0;

    float h = 0.71;

    char *infile = NULL;
    char *prefix = NULL;
    char *what = "ivmr";
    FILE *in = stdin;

    int format = FMT_AHF;

    //if (argc < 2) help();

    while (1)
    {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
           {"help",          no_argument,       0, 'h'},
           {"6dfof",         no_argument,       0, 0},
           {"ahf",           no_argument,       0, 0},
           {"skid",          no_argument,       0, 0},
           {"h",             required_argument, 0,   0},
           {"what",          required_argument, 0, 'w'},
           {"output-prefix", required_argument, 0, 'o'},
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "h:f:o:w:", long_options, &option_index);
        if (c == -1)
           break;

        switch (c) 
        {
            case 0:
                if (!strcmp("h", long_options[option_index].name))
                    h = atof(optarg);
                else if (!strcmp("6dfof", long_options[option_index].name))
                    format = FMT_6DFOF;
                else if (!strcmp("ahf", long_options[option_index].name))
                    format = FMT_AHF;
                else if (!strcmp("skid", long_options[option_index].name))
                    format = FMT_SKID;
                break;

            case 'h':
                help();
                break;
            case 'f':
                infile = optarg;
                break;
            case 'o':
                prefix = optarg;
                break;
            case 'w':
                what = optarg;
                for (i=0; i < strlen(optarg); i++)
                {
                    if (optarg[i] == 'a')
                    {
                        what = "ivmr";
                        break;
                    }

                    if (! (  optarg[i] == 'i' 
                          || optarg[i] == 'v'
                          || optarg[i] == 'r'
                          || optarg[i] == 'm'))
                    {
                        help();
                    }

                }

                break;
        }
    }

    if (optind < argc)
        infile = argv[optind++];
    else
        help();

    //------------------------------------------------------------------------
    //------------------------------------------------------------------------

    if (infile != NULL && strcmp("-", infile) != 0)
    {
        in = fopen(infile, "r");
        ERRORIF(in == NULL, "Can't open %s.", infile);
    }

    z_t *zs = NULL;
    int n_zs=0; 


    //------------------------------------------------------------------------
    // Read in the list of files to process.
    //------------------------------------------------------------------------
    work_t *work = NULL;
    int work_len;
    read_work(in, &work, &work_len);
    ERRORIF(work_len < 1, "No work to do.");
    if (in != stdin) fclose(in);


    //------------------------------------------------------------------------
    //------------------------------------------------------------------------


    int (*read_groups)(FILE *in, group_t **groups0, uint64_t *n_groups0) = NULL;

    switch (format)
    {
        case FMT_6DFOF:
            read_groups = read_6dfof_groups;
            break;
        case FMT_AHF:
            read_groups = read_ahf_groups;
            break;
        case FMT_SKID:
            read_groups = read_skid_groups;
            break;
    }


    //------------------------------------------------------------------------
    //------------------------------------------------------------------------


    //work_len = 10;
    n_zs = work_len+1;
    zs = MALLOC(z_t, n_zs);

    z_t *Z = zs;


    //------------------------------------------------------------------------
    //------------------------------------------------------------------------


    FILE *pf_fp=NULL;

    for (i=0; i < work_len; i++, Z++)
    {
        float z     = work[i].z;
        char *stats = work[i].stats;
        char *pf    = work[i].pf;

        //--------------------------------------------------------------------
        //--------------------------------------------------------------------

        eprintf("__________________________________________________________________________\n");
        eprintf("  [%i]  %f  %s %s\n", i+1, z, stats, pf);

        FILE *stats_fp = fopen(stats, "r");
        ERRORIF(stats_fp == NULL, "Can't open %s.", stats);

        //--------------------------------------------------------------------
        read_groups(stats_fp, &D, &nD);
        fclose(stats_fp);
        Z->n_groups = nD;
        Z->g        = D;
        Z->used     = CALLOC(int, nD+1); assert(Z->used != NULL);
        //--------------------------------------------------------------------

        if (Z->n_groups > 0)
        {
            ERRORIF((pf_fp = fopen(pf, "r")) == NULL, "Can't open %s.", pf);
            //----------------------------------------------------------------
            find_progenitors(pf_fp, D, nD);
            //----------------------------------------------------------------
            fclose(pf_fp);
        }
    }

    progenitor_groups(&zs[n_zs-2], &D, &nD);
    zs[n_zs-1].n_groups = nD;
    zs[n_zs-1].g = D;
    zs[n_zs-1].used = CALLOC(int, nD+1); assert(zs[n_zs-1].used != NULL);

    track_t *tracks;
    uint64_t n_tracks;

    //------------------------------------------------------------------------
    eprintf("Building tracks...\n");
    build_tracks(zs, n_zs, &tracks, &n_tracks);
    //------------------------------------------------------------------------

    //assert(zs[0].n_groups != 0);

    //------------------------------------------------------------------------
    // Output.
    //------------------------------------------------------------------------

    eprintf("Writing output...\n");

    FILE *fp = stdout;
    int fname_len = 1+1+6+1;
    if (prefix != NULL)
        fname_len += strlen(prefix) + 1;
    char *fname = MALLOC(char, fname_len);
    for (; *what; what++)
    {
        if (prefix) 
        {
            switch (*what)
            {
                case 'm':
                case 'r':
                case 'v':
                case 'i':
                    if (prefix != NULL)
                        sprintf(fname, "%s.%c.htrack", prefix, *what);
                    else
                        sprintf(fname, "%c.htrack", *what);
                    ERRORIF((fp = fopen(fname, "w")) == NULL, "Can't open %s for writing. Skipping.", fname);
                    break;
            }
        }

        switch (*what)
        {
            case 'i': write_pid_matrix(fp, zs, n_zs, tracks, n_tracks);  break;
            case 'm': write_mass_matrix(fp, zs, n_zs, tracks, n_tracks); break;
            case 'r': write_r_matrix(fp, zs, n_zs, tracks, n_tracks); break;
            case 'v': write_v_matrix(fp, zs, n_zs, tracks, n_tracks); break;
#if 0
            case 'r': 
                calc_R(zs, n_zs, tracks, n_tracks); 
                write_R_matrix(fp, zs, n_zs, tracks, n_tracks); 
                break;
#endif
        }

        if (fp != stdout) fclose(fp);
    }

    return 0;
}

