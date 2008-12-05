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

#define SECONDS_PER_GYR (3.1536e16)

int verbosity = 0;

int  read_ahf_groups(FILE *in, group_t **groups0, uint64_t *n_groups0);
int  accept_phase_space(group_t *d, group_t *p, float dt);
int  accept_mass(group_t *d, group_t *p);
int  track(FILE *in, group_t *D, group_t *P, float dt);
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

//============================================================================
//                                accept_mass
//============================================================================
int accept_mass(group_t *d, group_t *p)
{
    return 1;
    //return ! (p->Mvir > 10*d->Mvir);
}

//============================================================================
//                                   track
//============================================================================
int track(FILE *in, group_t *D, group_t *P, float dt)
{
    int i;
    char *line = NULL;
    size_t len;

    set_t used = EMPTY_SET;

    while (!feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        uint64_t gid   = atol(strtok(line, " "));
        uint64_t nprog = atol(strtok(NULL, " "));

        //====================================================================
        // Find the first progenitor that passes all acceptance criteria.
        // If we accept it, then add it to the list of used groups so that
        // it will not be considered again.
        //====================================================================
        int accept = 0;
        uint64_t p = 0;
        for (i=0; i < nprog && !accept; i++)
        {
            p = atol(strtok(NULL, " "));
            accept = !set_in(&used, p)
                  && accept_phase_space(&D[gid], &P[p], dt)
                  && accept_mass(&D[gid], &P[p]);
        }

        if (accept) 
        {
            set_add(&used, p);
            D[gid].pid = p;
        }
    }

    if (line != NULL) free(NULL);

    return 0;
}

//============================================================================
//                                WRITE_MATRIX
//============================================================================
#define WRITE_MATRIX(fname, _prop, _fmt)                                \
int fname(FILE *out, z_t *zs, int n_zs, int max_groups)                 \
{                                                                       \
    int i,j;                                                            \
    for (j=1; j <= max_groups; j++)                                     \
    {                                                                   \
        uint64_t next=j;                                                \
        for (i=0; i < n_zs; i++)                                        \
        {                                                               \
            if (j > zs[i].n_groups)                                     \
                fprintf(out, _fmt, (typeof(zs[0].g[0]._prop))0);        \
            else                                                        \
            {                                                           \
                fprintf(out, _fmt, zs[i].g[next]._prop);                \
                uint64_t t = zs[i].g[next].pid;                         \
                if (i+1 < n_zs) assert(t == zs[i+1].g[t].id);           \
                next = t;                                               \
            }                                                           \
        }                                                               \
        fprintf(out, "\n");                                             \
    }                                                                   \
    return 0;                                                           \
}

//============================================================================
//                             write_pid_matrix
//============================================================================
WRITE_MATRIX(write_pid_matrix, pid, "% 5ld ")

//============================================================================
//                             write_mass_matrix
//============================================================================
WRITE_MATRIX(write_mass_matrix, Mvir, "%.3e ")

//============================================================================
//                             write_vmax_matrix
//============================================================================
WRITE_MATRIX(write_vmax_matrix, vMax, "%.3e ")

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
            ERRORIF(groups == NULL, "No memory for mass list.");
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

        ERRORIF(read != 11, "Missing columns.");

        groups[n_groups].id = n_groups;
        ERRORIF(groups[n_groups].Mvir <= 0, "Zero/Negative mass in group file.");
    }

    if (n_groups > 0)
    {
        groups[0].Mvir = 0;
        groups[0].id   = 0;
        groups[0].pid  = 0;
    }

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups;
    *groups0   = groups;

    return 0;
}

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, "Usage: htrack --h # --zD #  --zP #  .pf_file  .grpD  .grpP\n");
    exit(2);
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    group_t *D = NULL;
    group_t *P = NULL;
    uint64_t nD = 0, nP = 0;

    float h = 0.71;
    float zD = -1, zP = -1;

    char *infile = NULL;
    FILE *in = stdin;

    //if (argc < 2) help();

    while (1)
    {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
           {"help", no_argument,       0, 'h'},
           {"h",    required_argument, 0, 0},
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "hf:", long_options, &option_index);
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
        }
    }

    //========================================================================
    //========================================================================

    if (infile != NULL)
    {
        in = fopen(infile, "r");
        ERRORIF(in == NULL, "Can't open input file.");
    }

    char *line = NULL;
    size_t len;
    char pf[256], stats[256];

    z_t zs[256];
    int n_zs=0; 
    int max_groups=0;

    FILE *pf_fp=NULL, *stats_fp;
    while (!feof(in))
    {
        float z;

        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        int nfields = sscanf(line, "%f %s %s", &z, stats, pf);

        fprintf(stderr, line);

        stats_fp = fopen(stats, "r");
        ERRORIF(stats_fp == NULL, "Can't open stats file.");

        if (D == NULL)
        {
            if (nfields == 3) 
                ERRORIF((pf_fp = fopen(pf, "r")) == NULL, "Can't open pf file.");
            zD = z;
            read_ahf_groups(stats_fp, &D, &nD);
            if (nD > max_groups) max_groups = nD;
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

        fprintf(stderr, "nD=%ld  nP=%ld\n", nD, nP);

        if (nD > max_groups) max_groups = nD;

        //====================================================================
        //====================================================================

        cosmo_t c;
        cosmo_init1(&c, h); 

        float dt = (cosmo_physical_time(&c, zD) - cosmo_physical_time(&c, zP)) * SECONDS_PER_GYR;

        assert(pf_fp != NULL);

        track(pf_fp, D, P, dt);
        zs[n_zs].n_groups = nD;
        zs[n_zs].g = D;
        n_zs++;

        //====================================================================
        //====================================================================

        if (nfields == 3) 
        {
            fclose(pf_fp);
            ERRORIF((pf_fp = fopen(pf, "r")) == NULL, "Can't open pf file.");
        }
    }

    if (in != stdin) fclose(in);


    assert(zs[0].n_groups != 0);

    //write_pid_matrix(stdout, zs, n_zs, max_groups);
    //write_mass_matrix(stdout, zs, n_zs, max_groups);
    write_vmax_matrix(stdout, zs, n_zs, max_groups);

    return 0;
}

