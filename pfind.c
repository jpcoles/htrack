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
float *grpP_masses = NULL;
float *grpD_masses = NULL;

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, PROGRAM_ID "\n");
    fprintf(stderr, "Usage: pfind <groupD> <statD> <groupP> <statP>\n");
    exit(2);
}

//============================================================================
//                               grpP_mass_cmp
// Sorts highest to lowest.
//============================================================================
int grpP_mass_cmp(const void *a0, const void *b0)
{
    uint64_t a = *((uint64_t *)a0);
    uint64_t b = *((uint64_t *)b0);

    if (grpP_masses[a] > grpP_masses[b]) return -1;
    if (grpP_masses[a] < grpP_masses[b]) return 1;
    return 0;
}

//============================================================================
//                               grpD_mass_cmp
// Sorts highest to lowest.
//============================================================================
int grpD_mass_cmp(const void *a0, const void *b0)
{
    group_t *a = (group_t *)a0;
    group_t *b = (group_t *)b0;

    if (a->id == 0) return -1;
    if (b->id == 0) return 1;

    if (grpD_masses[a->id] > grpD_masses[b->id]) return -1;
    if (grpD_masses[a->id] < grpD_masses[b->id]) return 1;
    return 0;
}

//============================================================================
//                           sort_group_progenitors
//============================================================================
uint32_t sort_group_progenitors(group_t *groups, uint64_t n_groups)
{
    uint64_t i;
    for (i=1; i <= n_groups; i++)
    {
        if (groups[i].ps.len == 0)
        {
            fprintf(stderr, "Group %ld (%ld) has no progenitors?\n", i, groups[i].id);
            assert(groups[i].ps.len != 0);
        }
        qsort(groups[i].ps.v, groups[i].ps.len, sizeof(uint64_t), grpP_mass_cmp);
    }
    return 0;
}

//============================================================================
//                             write_output_ascii
//============================================================================
uint32_t write_output_ascii(FILE *out, group_t *groups, uint64_t n_groups)
{
    uint64_t i,j;
    fprintf(out, "# %12s  %8s  %s\n", "GroupID(1)", "Nprog(2)", "Progenitors");
    for (i=1; i <= n_groups; i++)
    {
        fprintf(out, "  %12ld  ", groups[i].id);
        fprintf(out, "%8ld  ", groups[i].ps.len);
        for (j=0; j < groups[i].ps.len; j++)
            fprintf(out, "%ld ", groups[i].ps.v[j]);
            //fprintf(out, "%ld(%f) ", groups[i].ps[j], grpP_masses[groups[i].ps[j]]);
        fprintf(out, "\n");
    }
    return 0;
}

//============================================================================
//                              build_prog_list
//============================================================================
uint32_t build_prog_list(FILE *fpD, FILE *fpP, 
                         group_t *groups, uint64_t n_groupsD, uint64_t n_groupsP)
{
    uint64_t ND, NP;
    int read;

    char *line = NULL;
    size_t len;

    if ((read = getline(&line, &len, fpD)) < 0) return 1;
    sscanf(line, "%ld", &ND);
    if ((read = getline(&line, &len, fpP)) < 0) return 1;
    sscanf(line, "%ld", &NP);

    ERRORIF(ND != NP, "Number of particles is not consistent in input.");

    while (ND-- > 0 && NP-- > 0)
    {
        ERRORIF(feof(fpD) || feof(fpP), "Unexpected end of file encountered.");

        //====================================================================
        // Read one group id from each file.
        //====================================================================
        uint64_t gidD, gidP;
        if ((read = getline(&line, &len, fpD)) < 0) return 1;
        sscanf(line, "%ld", &gidD);
        //fprintf(stderr, "line=%s ND=%ld gidD=%ld  ngroupsD=%ld\n", line, ND, gidD, n_groupsD);
        ERRORIF(gidD > n_groupsD, "gidD > n_groupsD");

        if ((read = getline(&line, &len, fpP)) < 0) return 1;
        sscanf(line, "%ld", &gidP);
        ERRORIF(gidP > n_groupsP, "gidP > n_groupsP");

        //====================================================================
        // Add the new group.
        //====================================================================
        groups[gidD].id = gidD;
        set_add(&(groups[gidD].ps), gidP);
    }

    return 0;
}

//============================================================================
//                             read_ahf_group_masses
//============================================================================
uint32_t read_ahf_groups_masses(FILE *in, float **masses0, uint64_t *n_groups0)
{
    uint32_t ret=0;
    int read;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading AHF format group file.\n");

    uint64_t n_groups=0;
    uint64_t allocd = 0;
    float *masses = NULL;

    while (!ret && !feof(in))
    {
        uint32_t di;
        float df, mass;

        /* Read the whole line */
        read = getline(&line, &len, in);
        if (read <= 0 || line[0] == '#') continue;

        if (n_groups == allocd)
        {
            if (allocd == 0) allocd = 32;
            else allocd *= 2;

            masses = REALLOC(masses, float, allocd+1);
            ERRORIF(masses == NULL, "No memory for mass list.");
            memset(masses + n_groups+1, 0, (allocd-n_groups) * sizeof(float));
        }

        /* Now extract just the first 10 values */
        read = 
            sscanf(line, "%d %d %g %g %g %g %g %g %g %g",
                &di,            /* No. Particles */
                &di,
                &df,&df,&df,    /* Position      */
                &df,&df,&df,    /* Velocity      */
                &mass,          /* Mass          */
                &df             /* Radius        */
                );

        ERRORIF(mass <= 0, "Zero/Negative mass in group file.");

        if (read <= 0) continue; /* check for EOF at top of loop */

        if (read != 10) { ret = 3; break; }

        masses[++n_groups] = mass;
    }

    masses[0] = 0;

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups;
    *masses0   = masses;

    return ret;
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    group_t *groupsD;
    uint64_t n_groupsD, n_groupsP;
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
                outname = MALLOC(char, strlen(optarg)+1);
                strcpy(outname, optarg);
                break;
        }
    }

    if (argc-optind < 4) help();

    if (outname != NULL)
    {
        out = fopen(outname, "w");
        ERRORIF(out == NULL, "Can't open output file.");
    }

    FILE *fpD = fopen(argv[optind++], "r");
    ERRORIF(fpD == NULL, "Can't open groupD file.");

    FILE *fpGrpD = fopen(argv[optind++], "r");
    ERRORIF(fpGrpD == NULL, "Can't open statD file.");

    FILE *fpP = fopen(argv[optind++], "r");
    ERRORIF(fpP == NULL, "Can't open groupP file.");

    FILE *fpGrpP = fopen(argv[optind++], "r");
    ERRORIF(fpGrpP == NULL, "Can't open statP file.");


    read_ahf_groups_masses(fpGrpP, &grpP_masses, &n_groupsP);
    read_ahf_groups_masses(fpGrpD, &grpD_masses, &n_groupsD);

    groupsD = CALLOC(group_t, n_groupsD+1);
    ERRORIF(groupsD == NULL, "No memory for groups.");

    ERRORIF(build_prog_list(fpD, fpP, groupsD, n_groupsD, n_groupsP),
            "Unable to read group files.");

    fclose(fpD);
    fclose(fpP);
    fclose(fpGrpD);
    fclose(fpGrpP);

    fprintf(stderr, "n_groupsD = %ld\n", n_groupsD);

    //========================================================================
    // First sort the groups by mass and then each list of progenitors.
    // Sort is always highest to lowest.
    //========================================================================
    qsort(groupsD+1, n_groupsD, sizeof(group_t), grpD_mass_cmp);
    sort_group_progenitors(groupsD, n_groupsD);

    write_output_ascii(out, groupsD, n_groupsD);

    if (out != stdout) fclose(stdout);

    return 0;
}
