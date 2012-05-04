//==============================================================================
//
// pfind
// Copyright 2009 Jonathan Coles
//
// Pfind compares two group files from an N-body simulation and determines
// which particles from group P(rogenitor) end up in group D(escendant).
// Particles for group D may come from many Ps in the past. The output contains
// one line from each D and a list of Ps.
//
//
// Notes:
//
// o Group ids start at 1, but there is a group 0 that represents all unbound
//   particles. Group counts always refer to the number of real groups and
//   arrays are allocated so that the number of elements is 1 plus the number
//   of groups. Group 0 is always in element 0.
//
// o This was originally written with AHF group files in mind in which the
//   groups are consecutively ordered and larger groups have smaller ids.  Stat
//   files from 6DFOF output groups are in an undefined order and do not
//   necessarily form a contiguous list of ids if some groups are thrown out
//   due to a some given cutoff. 
//
//   The simplest hack to work around this is to allocate the group list up to
//   the largest group id that we see, set all ids of groups that don't really
//   exist to INVALID_GROUP_ID and then skip over them when processing the
//   group list.
//==============================================================================

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include "set.h"
#include "pfind.h"
#include "jpcmacros.h"
#include "timing.h"
#include "getline.h"

CPUDEFS

#define INVALID_GROUP_ID            ULONG_MAX

#define forall_groups_inc_invalid(_i, _grp, _G, _n) \
    for (_i=0; _i <= (_n); _i++)       \
        if ((_grp = &_G[_i]))

#define forall_groups(_i, _grp, _G, _n) \
    for (_i=0; _i <= (_n); _i++)       \
        if ((_grp = &_G[_i])->id != INVALID_GROUP_ID)

int verbosity = 0;
char *tag = "pfind";

#define FMT_6DFOF    1
#define FMT_AHF     2
#define FMT_SKID    3


//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, PROGRAM_ID "\n");
    fprintf(stderr, 
    "Usage: pfind [OPTIONS] DGRP DSTAT PGRP PSTAT\n"
    "Find the progenitors (P) of decendent halos (D).\n"
    "\n"
    "    (P|D)GRP           A list of group ids for each particle.\n"
    "    (P|D)STAT          A catalogue of groups.\n"
    "\n"
    "OPTIONS can be any of\n"
    "    -o FILENAME        Write output to FILENAME (default standard out)\n"
    "    -h, --help         Show this help.\n"
    "\n"
    "Pfind compares two group files from an N-body simulation and determines\n"
    "which particles from group P(rogenitor) end up in group D(escendant).\n"
    "Particles for group D may come from many Ps in the past. The output\n"
    "contains one line from each D and a list of Ps.\n"
    "\n"
    "Report bugs to <jonathan@physik.uzh.ch>\n"
    );
    exit(2);
}

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
    group_t *cur_grp;

    forall_groups(i, cur_grp, D, n_groups)
    {
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

        qsort(cur_grp->index.v, cur_grp->index.len, sizeof(uint64_t), _mass_cmp);
    }
    return 0;
}

//============================================================================
//                            sort_group_belonging
//============================================================================
int sort_group_belonging(group_t *D, uint64_t n_groups)
{
    uint64_t i;
    group_t *cur_grp;

    forall_groups(i, cur_grp, D, n_groups)
    {
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

        qsort(cur_grp->belong.v, cur_grp->belong.len, sizeof(uint64_t), _id_cmp);
    }
    return 0;
}

//============================================================================
//                             write_output_ascii
//============================================================================
int write_output_ascii(FILE *out, group_t *groups, uint64_t n_groups)
{
    uint64_t i,j;
    group_t *g;

    fprintf(out, 
    "# Format:\n"
    "#     GroupID(1) Nprog(2) Progenitors(3...)\n"
    "#\n"
    "# Progenitors(3...) is a list where each item has the form I(F,N) and where I is\n"
    "# the progenitor id, F is the fraction of the decedent composed of particles\n"
    "# from the progenitor and N is the number of particles from the progenitor.\n"
    "#\n"
    "# Groups are sorted in decending order by mass. Group 0 is always first.\n"
    "# The progenitor list is sorted by fraction F with group 0 always last.\n"
    "#\n"
    );
    forall_groups(i, g, groups, n_groups)
    {
        //fprintf(out, "  %12ld  ", groups[i].id);
        //fprintf(out, "  %12ld(%ld)  ", groups[i].id, groups[i].npart);

        assert(g->belong.len != 0);

        //--------------------------------------------------------------------
        // Group id (and belonging id's)
        //--------------------------------------------------------------------
        fprintf(out, "%-8ld", g->belong.v[0]);
        for (j=1; j < g->belong.len; j++)
            fprintf(out, ",%ld", g->belong.v[j]);
        fprintf(out, " ");

        //--------------------------------------------------------------------
        // Number of progenitors
        //--------------------------------------------------------------------
        fprintf(out, "%-4ld  ", g->ps.len);

        //--------------------------------------------------------------------
        // Progenitors
        //--------------------------------------------------------------------
        for (j=0; j < g->ps.len; j++)
        {
            uint64_t pi = g->index.v[j];
            fprintf(out, "%8ld(%f,%ld) ", 
                g->ps.v[pi], 
                (float)g->pfrac.v[pi] / g->npart, 
                g->pfrac.v[pi]);
        }
        fprintf(out, "\n");
    }
    return 0;
}

#if 0
int write_output_binary(FILE *out, group_t *groups, uint64_t n_groups)
{
    uint64_t i,j;
    group_t *g;

    forall_groups(i, g, groups, n_groups)
    {
        assert(g->belong.len != 0);

        //--------------------------------------------------------------------
        // Group id (and belonging id's)
        //--------------------------------------------------------------------
        fwrite(&(g->belong.len), sizeof(g->belong.len), 1, out); 
        fwrite(&(g->belong.v), sizeof(g->belong.v[0]), g->belong.len, out); 

        //--------------------------------------------------------------------
        // Number of progenitors
        //--------------------------------------------------------------------
        fwrite(&(g->ps.len), sizeof(g->ps.len), 1, out); 

        //--------------------------------------------------------------------
        // Progenitors
        //--------------------------------------------------------------------
        for (j=0; j < g->ps.len; j++)
        {
            uint64_t pi = g->index.v[j];
            float frac = (float)g->pfrac.v[pi] / g->npart;
            fwrite(&(f->ps.v[pi]), sizeof(f->ps.v[pi]), 1, out);
            fwrite(&frac, sizeof(frac), 1, out);
            fwrite(&(f->pfrac.v[pi]), sizeof(f->pfrac.v[pi]), 1, out);
        }
    }
    return 0;
}
#endif

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
               ASSERT(id != 0, "Parent is a field particle! (parent id=0)");
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
                    group_t *D0, uint64_t nD, 
                    group_t *P0, uint64_t nP,
                    char check)
{
    group_t *D, *P;
    uint64_t Dnpart, Pnpart;
    int read;

    char *line = NULL;
    size_t len;

    if ((read = getline(&line, &len, fpD)) < 0) return 1;
    sscanf(line, "%ld", &Dnpart);
    if ((read = getline(&line, &len, fpP)) < 0) return 1;
    sscanf(line, "%ld", &Pnpart);

    ERRORIF(Dnpart != Pnpart, "%s] Number of particles is not consistent in input. %ld != %ld", tag, Dnpart, Pnpart);

    while (Dnpart-- > 0 && Pnpart-- > 0)
    {
        uint64_t gidD, gidP;
        ERRORIF(feof(fpD) || feof(fpP), "%s] Unexpected end of file encountered.", tag);

        //--------------------------------------------------------------------
        // Read one group id from each file.
        //--------------------------------------------------------------------
        if ((read = getline(&line, &len, fpP)) <= 0) return 1;
        sscanf(line, "%ld", &gidP);
        ERRORIF(gidP > nP, "%s] gidP %ld > number of groups (%ld)", tag, gidP, nP);

        if ((read = getline(&line, &len, fpD)) <= 0) return 1;
        sscanf(line, "%ld", &gidD);
        ERRORIF(gidD > nD, "%s] gidD %ld > number of groups (%ld)", tag, gidD, nD);

        D = &D0[gidD];
        P = &P0[gidP];

        D->npart++;
        P->npart++;

        //--------------------------------------------------------------------
        if (check) continue;
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        add_belonging_groups(line, D0, nD);
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        // Add the new group.
        //--------------------------------------------------------------------
        assert(set_empty(&(D->ps)) || D->id == gidD);

        //D[gidD].id = gidD;
        int i = set_add(&(D->ps), gidP);

        ERRORIF(list_ensure_index(&(D->index), i), "%s] No memory.", tag);
        ERRORIF(list_ensure_index(&(D->pfrac), i), "%s] No memory.", tag);

        D->index.v[i] = i;     // Later we do an indirect sort using the indices
        D->pfrac.v[i]++;
    }

    //------------------------------------------------------------------------
    // Check that a group that has been mentioned in the stat file is in the
    // group file and vice versa. If there is a mismatch then the number of
    // particles found in the group file will be different than the number
    // found in the stat file. This is then also a check that the number of
    // particles is consistent. Only warnings are issued so that all 
    // inconsistencies will be displayed.
    //------------------------------------------------------------------------

    uint64_t i;
    group_t *g;
    forall_groups_inc_invalid(i, g, D0, nD)
    {
        if (g->id == 0) continue;

        if (!check && g->id != INVALID_GROUP_ID)
            WARNIF(g->ps.len == 0, "%s] GroupD %ld has no progenitors?", tag, g->id);

        uint64_t id = (g->id != INVALID_GROUP_ID) ? g->id : i;
        WARNIF(g->npart != g->npart_stat, "%s] GroupD %ld has %ld particles but stat file says %ld.",
            tag, id, g->npart, g->npart_stat);
    }

    forall_groups_inc_invalid(i, g, P0, nP)
    {
        uint64_t id = (g->id != INVALID_GROUP_ID) ? g->id : i;
        if (g->id == 0) continue;
        WARNIF(g->npart != g->npart_stat, "%s] GroupP %ld has %ld particles but stat file says %ld.",
            tag, id, g->npart, g->npart_stat);
    }

    return 0;
}

//============================================================================
//                              read_ahf_groups
//============================================================================
int read_ahf_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
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
        uint64_t i, npart;

        /* Read the whole line */
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;

        /* Get the number of particles */
        read = sscanf(line, "%ld", &npart);

        if (read <= 0) continue; /* check for EOF at top of loop */
        if (read != 1) { ret = 3; break; }

        if (n_groups == allocd)
        {
            if (allocd == 0) allocd = 32; else allocd *= 2;
            groups = REALLOC(groups, group_t, allocd+1);
            ERRORIF(groups == NULL, "%s] No memory for mass list.", tag);
            MEMSET(groups + n_groups+1, 0, allocd-n_groups, group_t);

            for (i=n_groups+1; i <= allocd; i++)
                groups[i].id = INVALID_GROUP_ID;
        }

        n_groups++;

        groups[n_groups].id         = n_groups;
        groups[n_groups].npart_stat = npart;
    }

    if (groups == NULL)
    {
        WARNIF(1, "%s] Stat file has no groups.", tag);
        groups = REALLOC(groups, group_t, 1);
        ERRORIF(groups == NULL, "%s] No memory for mass list.", tag);
    }

    MEMSET(groups, 0, 1, group_t);
    groups[0].id = 0;

    if (line != NULL) free(NULL);

    *groups0   = groups;
    *n_groups0 = n_groups;

    return ret;
}

//============================================================================
//                             read_6dfof_groups
//============================================================================
int read_6dfof_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int ret=0;
    int read;
    int i;
    uint64_t allocd = 0;

    uint64_t id, npart;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading 6DFOF format group file.\n");

    uint64_t n_groups=0;
    group_t *groups = NULL;

    while (!ret && !feof(in))
    {
        //------------------------------------------------------------------------
        // Read the whole line but only extract the group id and number of
        // particles.
        //------------------------------------------------------------------------
        if (getline(&line, &len, in) <= 0) continue;
        if (line[0] == '#') continue;
        //ERRORIF(line[0] == '#', "%s] Format does not support comments.", tag);

        read = sscanf(line, "%ld %ld", &id, &npart);

        if (read <= 0) continue; /* check for EOF at top of loop */
        if (read != 2) { ret = 3; break; }

        //------------------------------------------------------------------------
        // If the unbound particles are listed as their own group, skip it.
        //------------------------------------------------------------------------
        if (id == 0) continue;

        //------------------------------------------------------------------------
        // Group ids are not contiguous but we used a contiguous array to store
        // the group info. Extend this array if we find a group id larger than
        // the size of the array.
        //------------------------------------------------------------------------
        if (id > allocd) 
        {
            uint64_t n = allocd;
            if (n == 0) n = 2048;
            while (n < id) n *= 2;

            groups = REALLOC(groups, group_t, n+1);
            ERRORIF(groups == NULL, "%s] No memory for mass list.", tag);
            MEMSET(groups + allocd+1, 0, n-allocd, group_t);

            for (i=allocd+1; i <= n; i++)
                groups[i].id = INVALID_GROUP_ID;

            allocd = n;
        }

        if (id > n_groups) n_groups = id;
        groups[id].id = id;
        groups[id].npart_stat = npart;
    }

    //----------------------------------------------------------------------------
    // Early universe snapshots may not have any groups other that the "unbound"
    // group 0 so just make sure that we have at least one group.
    //----------------------------------------------------------------------------
    if (groups == NULL)
    {
        WARNIF(1, "%s] Stat file has no groups.", tag);
        groups = REALLOC(groups, group_t, 1);
        ERRORIF(groups == NULL, "%s] No memory for mass list.", tag);
    }

    MEMSET(groups, 0, 1, group_t);
    groups[0].id = 0;

    if (line != NULL) free(NULL);

    *groups0   = groups;
    *n_groups0 = n_groups;

    return ret;
}

//============================================================================
//                          read_skid_groups_masses
//============================================================================
int read_skid_groups(FILE *in, group_t **groups0, uint64_t *n_groups0)
{
    int ret=0;
    int read;
    int i;
    uint64_t allocd = 0;

    uint64_t id, npart;

    char *line = NULL;
    size_t len;

    VL(1) printf("Reading SKID format group file.\n");

    uint64_t n_groups=0;
    group_t *groups = NULL;

    while (!ret && !feof(in))
    {
        //------------------------------------------------------------------------
        // Read the whole line but only extract the group id and number of
        // particles.
        //------------------------------------------------------------------------
        if (getline(&line, &len, in) <= 0) continue;
        if (line[0] == '#') continue;
        //ERRORIF(line[0] == '#', "%s] Format does not support comments.", tag);

        read = sscanf(line, "%ld %ld", &id, &npart);

        if (read <= 0) continue; /* check for EOF at top of loop */
        if (read != 2) { ret = 3; break; }

        //------------------------------------------------------------------------
        // If the unbound particles are listed as their own group, skip it.
        //------------------------------------------------------------------------
        if (id == 0) continue;

        //------------------------------------------------------------------------
        // Group ids are not contiguous but we used a contiguous array to store
        // the group info. Extend this array if we find a group id larger than
        // the size of the array.
        //------------------------------------------------------------------------
        if (id > allocd) 
        {
            uint64_t n = allocd;
            if (n == 0) n = 2048;
            while (n < id) n *= 2;

            groups = REALLOC(groups, group_t, n+1);
            ERRORIF(groups == NULL, "%s] No memory for mass list.", tag);
            MEMSET(groups + allocd+1, 0, n-allocd, group_t);

            for (i=allocd+1; i <= n; i++)
                groups[i].id = INVALID_GROUP_ID;

            allocd = n;
        }

        if (id > n_groups) n_groups = id;
        groups[id].id = id;
        groups[id].npart_stat = npart;
    }

    //----------------------------------------------------------------------------
    // Early universe snapshots may not have any groups other that the "unbound"
    // group 0 so just make sure that we have at least one group.
    //----------------------------------------------------------------------------
    if (groups == NULL)
    {
        WARNIF(1, "%s] Stat file has no groups.", tag);
        groups = REALLOC(groups, group_t, 1);
        ERRORIF(groups == NULL, "%s] No memory for mass list.", tag);
    }

    MEMSET(groups, 0, 1, group_t);
    groups[0].id = 0;

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
    group_t *D = NULL, *P = NULL;
    uint64_t nD, nP;

    double T_start, T_end;

    char check = 0;

    FILE *out = stdout;
    char *outname = NULL;

    int format = FMT_AHF;

    //----------------------------------------------------------------------------
    // Process command line arguments.
    //----------------------------------------------------------------------------

    if (argc < 4) help();

    while (1)
    {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
           {"help",  no_argument,       0, 'h'},
           {"6dfof", no_argument,       0, 0},      /* 6dfof group file                 */
           {"ahf",   no_argument,       0, 0},      /* ahf group file                   */
           {"skid",  no_argument,       0, 0},      /* skid group file                  */
           {"check", no_argument,       0, 0},      /* only check file consistency      */
           {"tag",   required_argument, 0, 0},      /* preface warnings/errors with tag */
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "ho:", long_options, &option_index);
        if (c == -1)
           break;

        switch (c) 
        {
            case 0:
                if (!strcmp("6dfof", long_options[option_index].name))
                    format = FMT_6DFOF;
                else if (!strcmp("ahf", long_options[option_index].name))
                    format = FMT_AHF;
                else if (!strcmp("skid", long_options[option_index].name))
                    format = FMT_SKID;
                else if (!strcmp("check", long_options[option_index].name))
                    check = 1;
                else if (!strcmp("tag", long_options[option_index].name))
                {
                    tag = MALLOC(char, strlen(optarg)+1);
                    strcpy(tag, optarg);
                }

                break;
            case 'h':
                help();
                break;
            case 'o':
                outname = optarg;
                break;
        }
    }

    if (argc-optind < 4) help();

    //----------------------------------------------------------------------------
    // Open all the files now just in case there is a problem. If we detect it
    // early then we don't have to wait to read the large group files only to 
    // find an error opening the output file.
    //----------------------------------------------------------------------------

    if (outname != NULL && !check)
    {
        out = fopen(outname, "w");
        ERRORIF(out == NULL, "%s] Can't open %s.", tag, outname);
    }

    FILE *fpD = fopen(argv[optind], "r");
    ERRORIF(fpD == NULL, "%s] Can't open %s.", tag, argv[optind]);
    optind++;

    FILE *fpGrpD = fopen(argv[optind], "r");
    ERRORIF(fpGrpD == NULL, "%s] Can't open %s.", tag, argv[optind]);
    optind++;

    FILE *fpP = fopen(argv[optind], "r");
    ERRORIF(fpP == NULL, "%s] Can't open %s.", tag, argv[optind]);
    optind++;

    FILE *fpGrpP = fopen(argv[optind], "r");
    ERRORIF(fpGrpP == NULL, "%s] Can't open %s.", tag, argv[optind]);
    optind++;

    T_start = CPUTIME;

    //----------------------------------------------------------------------------
    // Read the stat files so we know something about what to expect in the 
    // group files. We can check consistency this way.
    //----------------------------------------------------------------------------

    switch (format)
    {
        case FMT_6DFOF:
            read_6dfof_groups(fpGrpD, &D, &nD);
            read_6dfof_groups(fpGrpP, &P, &nP);
            break;
        case FMT_AHF:
            read_ahf_groups(fpGrpD, &D, &nD);
            read_ahf_groups(fpGrpP, &P, &nP);
            break;
        case FMT_SKID:
            read_skid_groups(fpGrpD, &D, &nD);
            read_skid_groups(fpGrpP, &P, &nP);
            break;
    }

    fclose(fpGrpD);
    fclose(fpGrpP);

    //----------------------------------------------------------------------------
    // Now read the files and build the progenitor lists. Abort on error.
    //----------------------------------------------------------------------------
    ERRORIF(build_prog_list(fpD, fpP, D, nD, P, nP, check),
        "%s] Unable to read group files.", tag);

    fclose(fpD);
    fclose(fpP);

    if (!check)
    {
        eprintf("Number of D groups: %ld  Number of P groups: %ld\n", nD, nP);

        //------------------------------------------------------------------------
        // First sort the groups by mass and then each list of progenitors.
        // Sort is always highest to lowest.
        //------------------------------------------------------------------------
        qsort(D, nD+1, sizeof(group_t), D_mass_cmp);
        sort_group_progenitors(D, nD);
        sort_group_belonging(D, nD);

        write_output_ascii(out, D, nD);
    }

    //----------------------------------------------------------------------------
    // Just some timing information.
    //----------------------------------------------------------------------------
    T_end = CPUTIME;
    double T_total = T_end - T_start;
    int hours = T_total / 3600;
    int mins  = (T_total - hours * 3600) / 60;
    eprintf("%s] Time: %ih:%im:%.2fs\n", tag, hours, mins, (T_total - hours * 3600 - mins * 60));

    if (out != stdout) fclose(stdout);

    return 0;
}
