#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "htrack.h"
#include "jpcmacros.h"
#include "cosmo.h"

#define SECONDS_PER_GYR         (3.1536e16)

#define IGNORE_PHASE_SPACE              (1)
#define IGNORE_MASS_JUMP                (1)

#define DELIM                       " \n\r"

int verbosity = 0;

int  read_ahf_groups(FILE *in, group_t **groups0, uint64_t *n_groups0);
int  accept_phase_space(group_t *d, group_t *p, float dt);
int  accept_mass(group_t *d, group_t *p);
int  track(FILE *in, group_t *D, group_t *P);
void help();

//============================================================================
//                             accept_phase_space
//============================================================================
int accept_phase_space(group_t *d, group_t *p, float dt)
{
    const float eps = 10.9;

#define KM_PER_MPC (3.08567759756e19)

    float r = sqrt(d->r[0]*d->r[0] + d->r[1]*d->r[1] + d->r[2]*d->r[2]);

    return fabs(d->r[0] - (p->r[0] + dt*p->v[0]/KM_PER_MPC)) / r  < eps
        && fabs(d->r[1] - (p->r[1] + dt*p->v[1]/KM_PER_MPC)) / r  < eps
        && fabs(d->r[2] - (p->r[2] + dt*p->v[2]/KM_PER_MPC)) / r  < eps;
}

#if 1
//============================================================================
//                                accept_mass
//============================================================================
int accept_mass(group_t *d, group_t *p)
{
    return ! (p->vMax > 3*d->vMax);
}
#endif

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
            set_add(&d[gid].belong, id);
        }
    }

    return gid;
}

//============================================================================
//                                   track
//============================================================================
int track(FILE *in, group_t *D, group_t *P)
{
    int i,j;
    char *line = NULL;
    size_t len;
    uint64_t p;

    set_t used  = EMPTY_SET;
    set_t order = EMPTY_SET;

    while (!feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        //uint64_t gid   = atol(strtok(line, DELIM));
        uint64_t gid   = parse_gid_and_belonging(D, strtok(line, DELIM));
        uint64_t nprog = atol(strtok(NULL, DELIM));

        if (gid == 0) continue;

        set_add(&order, gid);

        for (i=0; i < nprog; i++)
            set_add(&D[gid].ps, atol(strtok(NULL, DELIM)));
    }

    uint64_t n_not_first = 0;

    for (i=0; i < order.len; i++)
    {
        uint64_t gid = order.v[i];

        D[gid].pid = 0;

        //if (D[gid].Mvir < 5e7) continue;

        //====================================================================
        // Find the first progenitor that passes all acceptance criteria.
        // If we accept it, then add it to the list of used groups so that
        // it will not be considered again.
        //====================================================================
        int accept = 0;
        for (j=0; j < D[gid].ps.len && !accept; j++)
        {
            p = D[gid].ps.v[j];
            accept = set_in(&used, p) < 0;
#if 0
                  && (IGNORE_PHASE_SPACE || accept_phase_space(&D[gid], &P[p], dt))
                  && (IGNORE_MASS_JUMP   || accept_mass(&D[gid], &P[p]))
                  ;
#endif
            if (accept) break;
        }

        if (accept) 
        {
            set_add(&used, p);
            D[gid].pid = p;
            if (j != 0 || D[gid].ps.len == 1)
            {
                eprintf("%ld\n", gid);
                n_not_first++;
            }
        }

    }

    eprintf("n_not_first = %ld of %ld (%f)\n", n_not_first, order.len, (float)n_not_first/order.len);

    set_free(&used);

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

    int i,j,k;                                                                      
    uint64_t next;                                                                  
    int n_tracks=0;                                                           

    for (k=0; k < n_zs; k++) 
        for (j=1; j <= zs[k].n_groups; j++) 
            zs[k].used[j] = 0; 

    for (k=0; k < n_zs; k++)                                                        
    {                                                                               
        for (j=1; j <= zs[k].n_groups; j++)                                         
        {                                                                           
            if (zs[k].used[j]) continue;                                            

            if (n_tracks == allocd)
            {
                if (allocd == 0) allocd = 64;
                else allocd *= 2;

                tracks = REALLOC(tracks, track_t, allocd);
                memset(tracks+n_tracks, 0, allocd-n_tracks);
            }


            tracks[n_tracks].t = CALLOC(uint64_t, n_zs);

            for (i=k, next=j; i < n_zs; i++)                                        
            {                                                                       
                tracks[n_tracks].t[i] = next;
                zs[i].used[next] = 1;                                               
                uint64_t t = zs[i].g[next].pid;                                     
                //if (i+1 < n_zs) assert(t == zs[i+1].g[t].id);                       
                next = t;                                                           
            }                                                               	    

            n_tracks++;                                                       
        }									                                        
    }                                                                               
    VL(2) fprintf(stderr, "n_tracks=%i\n", n_tracks);                   

    *tracks0   = tracks;
    *n_tracks0 = n_tracks;

    return 0;
}

//============================================================================
//                                WRITE_MATRIX
//============================================================================
#define WRITE_MATRIX(fname, _prop, _fmt)                                            \
int fname(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)                                             \
{                                                                                   \
    int i,t;                                                                      \
    fprintf(out, "%i %i\n", n_tracks, n_zs); \
    for (t=0; t < n_tracks; t++)                                         \
    {                                                                           \
        for (i=0; i < n_zs; i++)                                                        \
            fprintf(out, _fmt, zs[i].g[tracks[t].t[i]]._prop);                            \
        fprintf(out, "\n");							                            \
    }                                                                               \
    return 0;                                                                       \
}

//============================================================================
//                             write_pid_matrix
//============================================================================
int write_pid_matrix(FILE *out, z_t *zs, int n_zs, track_t *tracks, int n_tracks)                                             \
{
    int i,t,j;
    fprintf(out, "%i %i\n", n_tracks, n_zs);
    for (t=0; t < n_tracks; t++)
    {
        for (i=0; i < n_zs; i++)
        {
            group_t *g = &zs[i].g[tracks[t].t[i]];
            fprintf(out, "%ld", g->id);
            if (g->id != 0)
            {
                for (j=0; j < g->belong.len; j++)
                    fprintf(out, ",%ld", g->belong.v[j]);
            }
            fprintf(out, " ");
        }
        fprintf(out, "\n");
    }
    return 0;
}

//============================================================================
//                             write_mass_matrix
//============================================================================
WRITE_MATRIX(write_mass_matrix, Mvir, "%.3e ")

//============================================================================
//                             write_vmax_matrix
//============================================================================
WRITE_MATRIX(write_vmax_matrix, vMax, "%.3e ")

//============================================================================
//                               write_R_matrix
//============================================================================
WRITE_MATRIX(write_R_matrix, R, "%.3e ")

//============================================================================
//                               calc_R
//
// Computes the distance of each halo from the halo considered to be halo 1
// at each redshift.
//============================================================================
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
        uint32_t di;
        float df;

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

        groups[n_groups].R   = 0;
        groups[n_groups].id  = n_groups;
        groups[n_groups].pid = 0;
        ERRORIF(groups[n_groups].Mvir <= 0, "Group %ld has bad mass %f in group file.", n_groups, groups[n_groups].Mvir);
    }

    if (n_groups > 0)
    {
        groups[0].Mvir = 0;
        groups[0].id   = 0;
        groups[0].pid  = 0;
        groups[0].R    = 0;
    }

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
    work_t *work = NULL;
    int work_len = 0; 
    int allocd=0;

    char *line = NULL;
    size_t len;

    char *t;

    while (!feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        fprintf(stderr, line);

        if (work_len == allocd)
        {
            if (allocd == 0) allocd = 32; else allocd *= 2;
            work = REALLOC(work, work_t, allocd);
            assert(work != NULL);
        }

        t = strtok(line, " \n\r"); 
        if (t == NULL) continue; 
        float z  = atof(t);
        char *f1 = strtok(NULL, DELIM);
        char *f2 = strtok(NULL, DELIM);

        work[work_len].z = z;
        if (f1 != NULL) { work[work_len].stats = MALLOC(char, strlen(f1)+1); strcpy(work[work_len].stats, f1); }
        if (f2 != NULL) { work[work_len].pf    = MALLOC(char, strlen(f2)+1); strcpy(work[work_len].pf,    f2); }

        work_len++;
    }

    if (line != NULL) free(line);

    *work0 = work;
    *work_len0 = work_len;
    return 0;
}

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, "Usage: htrack [--help] [--h H0/100] [-w aivm] [-o prefix] [-f file]\n");
    exit(2);
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    int i;
    group_t *D = NULL;
    group_t *P = NULL;
    uint64_t nD = 0, nP = 0;

    float h = 0.71;
    float zD = -1, zP = -1;

    char *infile = NULL;
    char *prefix = NULL;
    char *what = "ivmr";
    FILE *in = stdin;

    //if (argc < 2) help();

    while (1)
    {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
           {"help",          no_argument,       0, 'h'},
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

    //========================================================================
    //========================================================================

    if (infile != NULL)
    {
        in = fopen(infile, "r");
        ERRORIF(in == NULL, "Can't open %s.", infile);
    }

    z_t zs[256];
    int n_zs=0; 

    work_t *work = NULL;
    int work_len;
    read_work(in, &work, &work_len);
    if (in != stdin) fclose(in);

    FILE *pf_fp=NULL;

    for (i=0; i < work_len; i++)
    {
        float z     = work[i].z;
        char *stats = work[i].stats;
        char *pf    = work[i].pf;

        FILE *stats_fp = fopen(stats, "r");
        ERRORIF(stats_fp == NULL, "Can't open %s.", stats);

        if (D == NULL)
        {
            ERRORIF((pf_fp = fopen(pf, "r")) == NULL, "Can't open %s.", pf);
            zD = z;
            read_ahf_groups(stats_fp, &D, &nD);
            fclose(stats_fp);
            continue;
        }
        else if (P != NULL)
        {
            D  = P;
            nD = nP;
            zD = zP;
        }

        zP = z;
        read_ahf_groups(stats_fp, &P, &nP);
        fclose(stats_fp);

        //fprintf(stderr, "nD=%ld  nP=%ld\n", nD, nP);

        //====================================================================
        //====================================================================

        cosmo_t c;
        cosmo_init1(&c, h); 

        //float dt = (cosmo_physical_time(&c, zD) - cosmo_physical_time(&c, zP)) * SECONDS_PER_GYR;

        assert(pf_fp != NULL);

        eprintf("__________________________________________________________________________\n");
        eprintf("  %f\n", zD);

        track(pf_fp, D, P);
        zs[n_zs].n_groups = nD;
        zs[n_zs].g = D;
        zs[n_zs].used = CALLOC(int, nD+1);
        n_zs++;

        if (i == work_len-1)
        {
            zs[n_zs].n_groups = nP;
            zs[n_zs].g = P;
            zs[n_zs].used = CALLOC(int, nP+1);
            n_zs++;
        }

        //====================================================================
        //====================================================================

        fclose(pf_fp);
        if (pf != NULL)
            ERRORIF((pf_fp = fopen(pf, "r")) == NULL, "Can't open %s.", pf);
    }

    track_t *tracks;
    uint64_t n_tracks;
    build_tracks(zs, n_zs, &tracks, &n_tracks);

    assert(zs[0].n_groups != 0);

    FILE *fp = stdout;
    char *fname = MALLOC(char, strlen(prefix)+1+1+1+2+1);
    for (; *what; what++)
    {
        if (prefix) 
        {
            sprintf(fname, "%s.%c.ht", prefix, *what);
            ERRORIF((fp = fopen(fname, "w")) == NULL, "Can't open %s for writing. Skipping.", fname);
        }

        switch (*what)
        {
            case 'i': write_pid_matrix(fp, zs, n_zs, tracks, n_tracks);  break;
            case 'm': write_mass_matrix(fp, zs, n_zs, tracks, n_tracks); break;
            case 'v': write_vmax_matrix(fp, zs, n_zs, tracks, n_tracks); break;
            case 'r': 
                calc_R(zs, n_zs, tracks, n_tracks); 
                write_R_matrix(fp, zs, n_zs, tracks, n_tracks); 
                break;
        }

        if (fp != stdout) fclose(fp);
    }

    return 0;
}

