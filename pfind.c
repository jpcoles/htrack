#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pfind.h"
#include "misc.h"

uint32_t build_prog_list(FILE *fpD, FILE *fpP, group_t *groups, uint64_t n_groupsD, uint64_t n_groupsP);
uint32_t add_to_list(group_t *group, uint64_t gidP);
uint32_t write_output_ascii(group_t *groups, uint64_t n_groups);
uint32_t sort_group_progenitors(group_t *groups, uint64_t n_groups);
uint32_t write_output_ascii(group_t *groups, uint64_t n_groups);
uint32_t read_ahf_groups_masses(FILE *in, float **masses0, uint64_t *n_groups0);
int prog_mass_cmp(const void *a0, const void *b0);
void help();

int verbosity = 0;

float *grpP_masses = NULL;
float *grpD_masses = NULL;

void help()
{
    fprintf(stderr, "Usage: pfind <particle-groupD> <particle-groupP> <groupP>\n");
    exit(2);
}

/* Sorts highest to lowest */
int grpP_mass_cmp(const void *a0, const void *b0)
{
    uint64_t a = *((uint64_t *)a0);
    uint64_t b = *((uint64_t *)b0);

    if (grpP_masses[a] > grpP_masses[b]) return -1;
    if (grpP_masses[a] < grpP_masses[b]) return 1;
    return 0;
}

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

int main(int argc, char **argv)
{
    if (argc < 4) help();

    group_t *groupsD;
    uint64_t n_groupsD, n_groupsP;

    FILE *fpD = fopen(argv[1], "r");
    ERRORIF(fpD == NULL, "Can't open particle-groupD file.");

    FILE *fpGrpD = fopen(argv[2], "r");
    ERRORIF(fpGrpD == NULL, "Can't open groupD file.");

    FILE *fpP = fopen(argv[3], "r");
    ERRORIF(fpP == NULL, "Can't open particle-groupP file.");

    FILE *fpGrpP = fopen(argv[4], "r");
    ERRORIF(fpGrpP == NULL, "Can't open groupP file.");


    read_ahf_groups_masses(fpGrpP, &grpP_masses, &n_groupsP);
    read_ahf_groups_masses(fpGrpD, &grpD_masses, &n_groupsD);

    groupsD = CALLOC(group_t, n_groupsD+1);
    ERRORIF(groupsD == NULL, "No memory for groups.");

    build_prog_list(fpD, fpP, groupsD, n_groupsD, n_groupsP);
    fclose(fpD);
    fclose(fpP);

#if 1

    //ERRORIF(n_groupsPhalos < n_groupsP, "Fewer groups in group file than in particle-group file.");

    qsort(groupsD, n_groupsD, sizeof(group_t), grpD_mass_cmp);
    sort_group_progenitors(groupsD, n_groupsD);
#endif

    write_output_ascii(groupsD, n_groupsD);

    return 0;
}

//============================================================================
//                           sort_group_progenitors
//============================================================================
uint32_t sort_group_progenitors(group_t *groups, uint64_t n_groups)
{
    uint64_t i;
    for (i=0; i <= n_groups; i++)
    {
        assert(groups[i].nprog != 0);
        qsort(groups[i].prog, groups[i].nprog, sizeof(uint64_t), grpP_mass_cmp);
    }
    return 0;
}

//============================================================================
//                             write_output_ascii
//============================================================================
uint32_t write_output_ascii(group_t *groups, uint64_t n_groups)
{
    uint64_t i,j;
    printf("# %12s  %8s  %s\n", "GroupID(1)", "Nprog(2)", "Progenitors");
    for (i=1; i <= n_groups; i++)
    {
        printf("  %12ld  ", groups[i].id);
        printf("%8ld  ", groups[i].nprog);
        for (j=0; j < groups[i].nprog; j++)
            printf("%ld ", groups[i].prog[j]);
            //printf("%ld(%f) ", groups[i].prog[j], grpP_masses[groups[i].prog[j]]);
        printf("\n");
    }
    return 0;
}

//============================================================================
//                                add_to_list
//============================================================================
uint32_t add_to_list(group_t *group, uint64_t gidP)
{
    uint64_t i;

    //========================================================================
    // First see if the group is already in the list.
    //========================================================================
    for (i=0; i < group->nprog; i++)
        if (group->prog[i] == gidP) return 0;

    //========================================================================
    // Not in the list. Grow the list if necessary.
    //========================================================================
    if (group->nprog == group->prog_len)
    {
        if (group->prog_len == 0) group->prog_len = 32;
        else group->prog_len *= 2;

        group->prog = (uint64_t *)realloc(group->prog, group->prog_len * sizeof(uint64_t));
        ERRORIF(group->prog == NULL, "No memory for progenitor list.");
        /* No need to initialize anything since we keep track of how many valid objects there are. */
    }

    //========================================================================
    // Add the group to the list.
    //========================================================================
    group->prog[group->nprog] = gidP;
    group->nprog++;

    return 0;
}

//============================================================================
//                              build_prog_list
//============================================================================
uint32_t build_prog_list(FILE *fpD, FILE *fpP, group_t *groups, uint64_t n_groupsD, uint64_t n_groupsP)
{
    uint64_t ND, NP;

    fscanf(fpD, "%ld", &ND);
    fscanf(fpP, "%ld", &NP);
    ERRORIF(ND != NP, "Number of particles is not consistent in input.");

    while (ND-- > 0 && NP-- > 0)
    {
        ERRORIF(feof(fpD) || feof(fpP), "Unexpected end of file encountered.");

        //====================================================================
        // Read one group id from each file.
        //====================================================================
        uint64_t gidD, gidP;
        fscanf(fpD, "%ld", &gidD);
        fscanf(fpP, "%ld", &gidP);

        ERRORIF(gidD > n_groupsD, "gidD > n_groupsD");
        ERRORIF(gidP > n_groupsP, "gidP > n_groupsP");

#if 0
        //====================================================================
        // Keep track of the largest group referenced so far.
        //====================================================================
        if (gidD > nGroupsD) nGroupsD = gidD;
        if (gidP > nGroupsP) nGroupsP = gidP;

        //====================================================================
        // Grow the number of groups and initialize.
        //====================================================================
        if (nGroupsD >= allocdGroups)
        {
            uint64_t old_allocdGroups = allocdGroups;

            if  (allocdGroups == 0) allocdGroups = 512; 
            while (allocdGroups < nGroupsD) allocdGroups *= 2;

            groups = (group_t *)realloc(groups, (allocdGroups+1) * sizeof(group_t));
            ERRORIF(groups == NULL, "No memory for group data structure");

            for (j=old_allocdGroups; j <= allocdGroups; j++)
            {
                groups[j].prog = NULL;
                groups[j].nprog = 0;
                groups[j].prog_len = 0;
            }
        }
#endif

        //====================================================================
        // Add the new group.
        //====================================================================
        groups[gidD].id = gidD;
        add_to_list(&(groups[gidD]), gidP);
    }

#if 0
    *groups0   = groups;
    *nGroupsD0 = nGroupsD;
    *nGroupsP0 = nGroupsP;
#endif
    return 0;
}

#if 1

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
        if (!read || line[0] == '#') continue;

        if (n_groups == allocd)
        {
            if (allocd == 0) allocd = 32;
            else allocd *= 2;

            masses = (float *)realloc(masses, (allocd+1) * sizeof(float));
            ERRORIF(masses == NULL, "No memory for mass list.");
            memset(masses + n_groups+1, 0, (allocd-n_groups+1) * sizeof(float));
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

        if (read == 0) continue; /* check for EOF at top of loop */

        if (read != 10) { ret = 3; break; }

        masses[++n_groups] = mass;
    }

    masses[0] = 0;

    if (line != NULL) free(NULL);

    *n_groups0 = n_groups-1;
    *masses0   = masses;

    return ret;
}

#endif

