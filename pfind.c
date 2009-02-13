#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include "set.h"
#include "pfind.h"
#include "jpcmacros.h"


int verbosity = 0;

/* These have to be global so the sort function can access them :( */
float *P_masses = NULL;
float *D_masses = NULL;


//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, PROGRAM_ID "\n");
    fprintf(stderr, "Usage: pfind <D> <statD> <groupP> <statP>\n");
    exit(2);
}

//============================================================================
//                               P_mass_cmp
// Sorts highest to lowest.
//============================================================================

//============================================================================
//                               D_mass_cmp
// Sorts highest to lowest.
//============================================================================
int D_mass_cmp(const void *a0, const void *b0)
{
    group_t *a = (group_t *)a0;
    group_t *b = (group_t *)b0;

    /* Always put group 0 (field) at the beginning */
    if (a->id == 0) return -1;
    if (b->id == 0) return 1;

    if (a->npart > b->npart) return -1;
    if (a->npart < b->npart) return +1;
    return 0;
}

//============================================================================
//                           sort_group_progenitors
//============================================================================
int sort_group_progenitors(group_t *D, uint64_t n_groups)
{
    uint64_t i;

    for (i=0; i <= n_groups; i++)
    {
#if 0
        WARNIF(D[i].ps.len == 0, "Group %ld (id=%ld) has no progenitors?", i, D[i].id);
        else
        {
            uint64_t j, sum=0;
            for (j=0; j < D[i].index.len; j++) sum += D[i].index.v[j];
            ERRORIF(sum != D[i].npart);
        }
#endif

        group_t *cur_grp = &D[i];

        int _mass_cmp(const void *a0, const void *b0)
        {
            uint64_t a = *((uint64_t *)a0);
            uint64_t b = *((uint64_t *)b0);

            //fprintf(stdout, "%ld %ld\n", cur_grp->ps.v[a], cur_grp->ps.v[b]);

            /* Always put group 0 (field) at the end */
            if (cur_grp->ps.v[a] == 0) return +1;
            if (cur_grp->ps.v[b] == 0) return -1;

            uint64_t npart = cur_grp->npart;

            float afrac = (float)cur_grp->pfrac.v[a] / npart;
            float bfrac = (float)cur_grp->pfrac.v[b] / npart;

            //fprintf(stdout, "%f %f\n", afrac, bfrac);
            if (afrac > bfrac) return -1;
            if (afrac < bfrac) return +1;
            return 0;
        }

        //fprintf(stdout, "***  %ld (%ld, %ld) ******************************\n", i, D[i].index.len, (D[i].index.len>0 ? D[i].pfrac.v[0] : -1));

        qsort(D[i].index.v, D[i].index.len, sizeof(uint64_t), _mass_cmp);
    }
    return 0;
}

//============================================================================
//                            sort_group_belonging
//============================================================================
int sort_group_belonging(group_t *D, uint64_t n_groups)
{
    uint64_t i;

    for (i=0; i <= n_groups; i++)
    {
        group_t *cur_grp = &D[i];

        //--------------------------------------------------------------------
        // Sort id's from largest to smallest
        //--------------------------------------------------------------------
        int _id_cmp(const void *a0, const void *b0)
        {
            uint64_t a = *((uint64_t *)a0);
            uint64_t b = *((uint64_t *)b0);
            if (a > b) return -1;
            if (a < b) return +1;
            return 0;
        }

        qsort(D[i].belong.v, D[i].belong.len, sizeof(uint64_t), _id_cmp);
    }
    return 0;
}

//============================================================================
//                             write_output_ascii
//============================================================================
int write_output_ascii(FILE *out, group_t *groups, uint64_t n_groups)
{
    uint64_t i,j;
    fprintf(out, "# %12s  %8s  %s\n", "GroupID(1)", "Nprog(2)", "Progenitors");
    for (i=0; i <= n_groups; i++)
    {
        //fprintf(out, "  %12ld  ", groups[i].id);
        //fprintf(out, "  %12ld(%ld)  ", groups[i].id, groups[i].npart);

        assert(groups[i].belong.len != 0);

        fprintf(out, "%ld", groups[i].belong.v[0]);
        for (j=1; j < groups[i].belong.len; j++)
            fprintf(out, ",%ld", groups[i].belong.v[j]);
        fprintf(out, " ",);

        fprintf(out, "%8ld  ", groups[i].ps.len);
        for (j=0; j < groups[i].ps.len; j++)
        {
            uint64_t pi = groups[i].index.v[j];
            //fprintf(out, "%ld ", groups[i].ps.v[pi]);
            fprintf(out, "%ld(%f,%ld) ", 
                groups[i].ps.v[pi], 
                (float)groups[i].pfrac.v[pi] / groups[i].npart, 
                groups[i].pfrac.v[pi]);

            //fprintf(out, "%ld(%ld) ", groups[i].ps.v[j], P[groups[i].ps.v[j]].npart);
            //fprintf(out, "%ld(%f) ", groups[i].ps[j], P_masses[groups[i].ps[j]]);
        }
        fprintf(out, "\n");
    }
    return 0;
}

//============================================================================
//                            add_belonging_groups
//============================================================================
int add_belonging_groups(char *line, group_t *D, uint64_t nD)
{
    uint64_t gid=0, first = 1;
    char *ap = NULL;

    while ((ap = strsep(&line, " \t\n")) != NULL)
    {
        if (*ap != '\0')
        {
            uint64_t id = atol(ap);
            if (first)
            {
                first = 0;
                gid   = id;
            }
            else
            {
               ASSERT(id != 0, "Parent id is that of a field particle (0)");
               // Groups belonging to other will have larger id's than their parents.
               ASSERT(id < gid, "Parent id is larger than child's: child=%ld  parent=%ld\n", id, gid);
            }
            set_add(&D[gid].belong, id);
        }
    }

    return 0;
}

//============================================================================
//                              build_prog_list
//============================================================================
int build_prog_list(FILE *fpD, FILE *fpP, 
                    group_t *D, uint64_t nD, 
                    group_t *P, uint64_t nP)
{
    uint64_t Dnpart, Pnpart;
    int read;

    char *line = NULL;
    size_t len;

    if ((read = getline(&line, &len, fpD)) < 0) return 1;
    sscanf(line, "%ld", &Dnpart);
    if ((read = getline(&line, &len, fpP)) < 0) return 1;
    sscanf(line, "%ld", &Pnpart);

    ERRORIF(Dnpart != Pnpart, "Number of particles is not consistent in input. %ld != %ld", Dnpart, Pnpart);

    while (Dnpart-- > 0 && Pnpart-- > 0)
    {
        ERRORIF(feof(fpD) || feof(fpP), "Unexpected end of file encountered.");

        //====================================================================
        // Read one group id from each file.
        //====================================================================
        uint64_t gidD, gidP;
        if ((read = getline(&line, &len, fpD)) <= 0) return 1;
        sscanf(line, "%ld", &gidD);
        ERRORIF(gidD > nD, "gidD %ld > number of groups (%ld)", gidD, nD);

        add_belonging_groups(line, D, nD);

        if ((read = getline(&line, &len, fpP)) <= 0) return 1;
        sscanf(line, "%ld", &gidP);
        ERRORIF(gidP > nP, "gidP %ld > number of groups (%ld)", gidP, nP);

        //====================================================================
        // Add the new group.
        //====================================================================
        assert(set_empty(&D[gidD].ps) || D[gidD].id == gidD);

        D[gidD].id = gidD;
        int i = set_add(&D[gidD].ps, gidP);

        ERRORIF(list_ensure_index(&D[gidD].index, i), "No memory.");
        ERRORIF(list_ensure_index(&D[gidD].pfrac, i), "No memory.");

        D[gidD].index.v[i] = i;     // Later we do an indirect sort using the indices
        D[gidD].pfrac.v[i]++;

        D[gidD].npart++;
        P[gidP].npart++;

        //assert(P[gidP].npart > 1348624510);
    }

    return 0;
}

//============================================================================
//                             read_ahf_group_masses
//============================================================================
int read_ahf_groups_masses(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int ret=0;
    int read;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading AHF format group file.\n");

    uint64_t n_groups=0;
    uint64_t allocd = 0;
    group_t *groups = NULL;

    while (!ret && !feof(in))
    {
        uint32_t di;
        float df;

        /* Read the whole line */
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;

        if (n_groups == allocd)
        {
            if (allocd == 0) allocd = 32; else allocd *= 2;
            groups = REALLOC(groups, group_t, allocd+1);
            ERRORIF(groups == NULL, "No memory for mass list.");
            MEMSET(groups + n_groups+1, 0, allocd-n_groups, group_t);
        }

        n_groups++;

        /* Now extract just the first 10 values */
        read = 
            sscanf(line, "%d %d %g %g %g %g %g %g %g %g %g",
                &di,            /* No. Particles */
                &di,
                &df,&df,&df,    /* Position      */
                &df,&df,&df,    /* Velocity      */
                /* Mass          */
                &groups[n_groups].M,
                /* Radius        */
                &df,
                /* Vmax             (11)  */
                &groups[n_groups].vMax
                );

        assert(groups[n_groups].npart == 0);
        ERRORIF(groups[n_groups].M <= 0, "Zero/Negative mass in group file.");

        if (read <= 0) continue; /* check for EOF at top of loop */

        if (read != 11) { ret = 3; break; }

    }

    MEMSET(groups, 0, 1, group_t);

    if (line != NULL) free(NULL);

    *groups0   = groups;
    *n_groups0 = n_groups;

    return ret;
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    uint64_t i;
    group_t *D = NULL, *P = NULL;
    uint64_t nD, nP;

    FILE *out = stdout;
    char *outname = NULL;

    if (argc < 4) help();

    while (1)
    {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
           {"help", no_argument,       0, 'h'},
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "ho:", long_options, &option_index);
        if (c == -1)
           break;

        switch (c) 
        {
            case 'h':
                help();
                break;
            case 'o':
                outname = optarg;
                break;
        }
    }

    if (argc-optind < 4) help();

    if (outname != NULL)
    {
        out = fopen(outname, "w");
        ERRORIF(out == NULL, "Can't open %s.", outname);
    }

    FILE *fpD = fopen(argv[optind], "r");
    ERRORIF(fpD == NULL, "Can't open %s.", argv[optind]);
    optind++;

    FILE *fpGrpD = fopen(argv[optind], "r");
    ERRORIF(fpGrpD == NULL, "Can't open %s.", argv[optind]);
    optind++;

    FILE *fpP = fopen(argv[optind], "r");
    ERRORIF(fpP == NULL, "Can't open %s.", argv[optind]);
    optind++;

    FILE *fpGrpP = fopen(argv[optind], "r");
    ERRORIF(fpGrpP == NULL, "Can't open %s.", argv[optind]);
    optind++;


    read_ahf_groups_masses(fpGrpD, &D, &nD);
    read_ahf_groups_masses(fpGrpP, &P, &nP);

    ERRORIF(build_prog_list(fpD, fpP, D, nD, P, nP), "Unable to read group files.");

    fclose(fpD);
    fclose(fpP);
    fclose(fpGrpD);
    fclose(fpGrpP);

    for (i=0; i <= nD; i++)
    {
        WARNIF(D[i].ps.len == 0, "Group %ld (id=%ld) has no progenitors?", i, D[i].id);
        //eprintf("%ld  %ld\n", D[i].id, D[i].npart);
    }

    fprintf(stderr, "nD=%ld  nP=%ld\n", nD, nP);

    //========================================================================
    // First sort the groups by mass and then each list of progenitors.
    // Sort is always highest to lowest.
    //========================================================================
    qsort(D, nD+1, sizeof(group_t), D_mass_cmp);
    sort_group_progenitors(D, nD);
    sort_group_belonging(D, nD);

    write_output_ascii(out, D, nD);

    if (out != stdout) fclose(stdout);

    return 0;
}
