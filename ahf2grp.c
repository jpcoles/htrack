/*
 * 
 * Group id's begin with 1 (0 is for field particles)
 * Particle id's begin with 0
 *
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <byteswap.h>
#include "set.h"
#include "jpcmacros.h"
#include "getline.h"

const int debug_level = 0;
#define DBG(_lvl_) if (debug_level >= (_lvl_))

#define G 0
#define D 1
#define S 2

struct tipsy_header
{
    double h_time;
    uint32_t h_nBodies;
    uint32_t h_nDims;
    uint32_t h_nGas;
    uint32_t h_nDark;
    uint32_t h_nStar;
    char dummy[4];
};

struct order
{
    int len;

    struct
    {
        char name[5];
        int type;
        long min_id;
        long max_id;
    } *t;
};

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, "Usage: ahf2grp [--dm-grp] [-b] [-o grp-output] --tipsy-file=<file> <.AHF_particles>\n");
    exit(2);
}

void read_tipsy_header(char *fname, struct tipsy_header *h)
{
    FILE *fp;

    fp = fopen(fname, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Can't open tipsy file %s.\n", fname); 
        exit(1);
    }

    if (fread(h, sizeof(struct tipsy_header), 1, fp) != 1)
    {
        fprintf(stderr, "Error reading tipsy file %s\n", fname);
        exit(1);
    }

    if (h->h_nDims != 3) // might not be native format
    {
        h->h_time    = bswap_64(h->h_time);
        h->h_nBodies = bswap_32(h->h_nBodies);
        h->h_nDims   = bswap_32(h->h_nDims);
        h->h_nGas    = bswap_32(h->h_nGas);
        h->h_nDark   = bswap_32(h->h_nDark);
        h->h_nStar   = bswap_32(h->h_nStar);

        if (h->h_nDims != 3)
        {
            fprintf(stderr, "Tipsy file possibly corrupt. Byte swapping didn't help.\n");
            exit(1);
        }
    }

    fclose(fp);
}

struct order *particle_order(char *type, struct tipsy_header *h)
{
    int len = (h->h_nGas  != 0)
            + (h->h_nDark != 0)
            + (h->h_nStar != 0);

    struct types
    {
        char *name;
        int type;
        long N;
    };
    
    struct types tipsy_types[3] = { {"gas", G, h->h_nGas},
                                    {"dark", D, h->h_nDark},
                                    {"star", S, h->h_nStar} };

    struct types ahf_types[3] = { {"dark", D, h->h_nDark},
                                  {"gas", G, h->h_nGas},
                                  {"star", S, h->h_nStar} };
    struct types *t;

    if (!strcmp("tipsy", type))
    {
        t = tipsy_types;
    }
    else if (!strcmp("ahf", type))
    {
        t = ahf_types;
    }
    else
    {
        fprintf(stderr, "Unknown particle ordering type %s.\n", type);
        exit(1);
    }

    struct order *ord = malloc(sizeof(struct order));
    ord->t = malloc(len * sizeof(*ord->t));
    ord->len = len;

    int i,j;
    long prev_max_id = -1;

    for (j=0,i=0; i < 3; i++)
    {
        if (t[i].N)
        {
            strcpy(ord->t[j].name, t[i].name);
            ord->t[j].type = t[i].type;
            ord->t[j].min_id = prev_max_id + 1;
            ord->t[j].max_id = t[i].N-1;
            prev_max_id = ord->t[j].max_id;
            j++;
        }
    }

    return ord;
}


//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    int i,j,k;
    int belong  = 0;
    int dm_grp  = 0;
    size_t nParticles=0;
    FILE *in  = stdin, 
         *out = stdout;
    char *inname = NULL;
    char *outname = NULL;
    char *tipsyname = NULL;
    char *iordstr = NULL;
    char *oordstr = NULL;
    struct tipsy_header h;
    struct order *iord = NULL;
    struct order *oord = NULL;
    int do_nothing = 0;

    //========================================================================
    // Process command line arguments.
    //========================================================================

    if (argc < 3) help();

    while (1)
    {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
           {"help", 1, 0, 'h'},
           {"belong", 1, 0, 'b'},
           {"dm-grp", 0, 0, 0},
           {"tipsy-file", 1, 0, 0},
           {"iord", 1, 0, 0},
           {"oord", 1, 0, 0},
           {"do-nothing", 0, 0, 0},
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "hbo:", long_options, &option_index);
        if (c == -1)
           break;


        switch (c) {
            case 0:
                if (!strcmp("dm-grp", long_options[option_index].name))
                    dm_grp = 1;
                if (!strcmp("tipsy-file", long_options[option_index].name))
                    tipsyname = optarg;
                if (!strcmp("iord", long_options[option_index].name))
                    iordstr = optarg;
                if (!strcmp("oord", long_options[option_index].name))
                    oordstr = optarg;
                if (!strcmp("do-nothing", long_options[option_index].name))
                    do_nothing = 1;
                break;
            case 'h':
                help();
                break;
            case 'b':
                belong = 1;
                break;
            case 'o':
                outname = optarg;
                break;
        }
    }

    if (tipsyname == NULL)
        help();

    if (argc-optind < 1) help();



    if (iordstr == NULL && oordstr == NULL)
    {
        iordstr = "ahf";
        oordstr = "ahf";
    }
    else if (iordstr != NULL && oordstr == NULL)
    {
        oordstr = iordstr;
    }
    else if (iordstr == NULL && oordstr != NULL)
    {
        iordstr = oordstr;
    }

    read_tipsy_header(tipsyname, &h);

    fprintf(stderr, "Tipsy file %s\n", tipsyname);
    fprintf(stderr, "  Time  : %f\n",  h.h_time);
    fprintf(stderr, "  Gas   : %i\n", h.h_nGas);
    fprintf(stderr, "  Dark  : %i\n", h.h_nDark);
    fprintf(stderr, "  Star  : %i\n", h.h_nStar);
    fprintf(stderr, "  Total : %i\n", h.h_nBodies);

    iord = particle_order(iordstr, &h);
    oord = particle_order(oordstr, &h);

    fprintf(stderr, "Particle ordering\n");
    fprintf(stderr, "%24s --> %24s\n", iordstr, oordstr);
    for (i=0; i < iord->len; i++)
    {
        for (j=0; j < oord->len; j++)
        {
            if (iord->t[i].type == oord->t[j].type)
            {
                fprintf(stderr, "%4s % 9ld % 9ld --> %4s % 9ld % 9ld\n", 
                    iord->t[i].name, iord->t[i].min_id, iord->t[i].max_id,
                    oord->t[j].name, oord->t[j].min_id, oord->t[j].max_id);
                break;
            }
        }
    }

    nParticles = h.h_nBodies;

    if (dm_grp)
    {
        fprintf(stderr, "Taking only dark matter particles\n.");
        nParticles = h.h_nDark;
    }

    if (do_nothing)
        exit(0);

    inname = argv[optind++];
    if ((in = fopen(inname, "r")) == NULL)
    {
        fprintf(stderr, "Can't open input file %s\n", inname);
        exit(2);
    }

    if (outname != NULL)
    {
        if ((out = fopen(outname, "w")) == NULL)
        {
            fprintf(stderr, "Can't open output file %s\n", outname);
            exit(2);
        }
    }
    else
    {
        outname = "stdout";
    }

    //========================================================================
    //========================================================================

    fprintf(stderr, "Reading input from %s...\n", inname);

    int gid=0, nGrpParticles;
    long pid;
    int line=0;
    char *linestr = NULL;
    size_t line_len;

    set_t *list = CALLOC(set_t, nParticles);

    list_t gcount = EMPTY_LIST;
    list_append(&gcount, 0);  // Because gid's begin at 1 we need something at 0

    if (getline(&linestr, &line_len, in) < 0)
    {
        fprintf(stderr, "Couldn't read first line of %s.\n", inname);
        exit(1);
    }
    line++;

    long min_id=0x0fffffff, max_id=0;

    //========================================================================
    // Process the file.
    //========================================================================
    while (!feof(in))
    {
        //====================================================================
        // A new group begins with the number of particles in it.
        //====================================================================
        if (getline(&linestr, &line_len, in) < 0) break;
        if (sscanf(linestr, "%i", &nGrpParticles) == 1)
        {
            line++;
            gid++;

            //================================================================
            // We keep track of how many particles are actually in a given
            // group and have not sunk to lower groups. First, assume all
            // particles of this new group will not sink.
            //================================================================
            list_append(&gcount, nGrpParticles);

            if (nGrpParticles == 0)
            {
                fprintf(stderr, "ERROR: %s\n"
                                "ERROR: Corrupt file? Group %i has 0 particles on line %i.\n", 
                                inname, gid, line);
                exit(1);
            }

            if (gid == 18)
            {
                fprintf(stderr, "Group %i. Should have %i particles\n", gid, nGrpParticles);
            }

            //================================================================
            // Read in each of those particle ids.
            //================================================================
            for (i=0; i < nGrpParticles; i++)
            {
                line++;
                if (getline(&linestr, &line_len, in) < 0)
                {
                    fprintf(stderr, "Unexpected EOF in %s.\n", inname);
                    exit(1);
                }
                if (sscanf(linestr, "%ld", &pid) >= 1)
                {
                    if (pid < min_id) min_id = pid;
                    if (pid > max_id) max_id = pid;

                    // Find which type and range the pid belongs to in the input.
                    for (j=0; j < iord->len; j++)
                    {
                        if (iord->t[j].min_id <= pid && pid <= iord->t[j].max_id)
                        {
                            // Adjust the pid for the output
                            for (k=0; k < oord->len; k++)
                            {
                                if (oord->t[k].type == iord->t[j].type)
                                {
                                    pid = pid - iord->t[j].min_id + oord->t[k].min_id;
                                    break;
                                }
                            }
                            break;
                        }
                    }

                    if (j == iord->len)
                    {
                        fprintf(stderr, "ERROR: %s\n"
                                        "ERROR: Particle has bad id %ld on line %i.\n", 
                                        inname, pid, line);
                        exit(1);
                    }

                    if (dm_grp && iord->t[j].type != D)
                    {
                        continue;
                    }

                    //========================================================
                    // Decrement the particle count from the last group that
                    // this particle belonged to.
                    //========================================================
                    if ((j=list[pid].len)) gcount.v[list[pid].v[j-1]]--;

                    set_add(&list[pid], gid);
                }
            }

        }
    }

    fprintf(stderr, "Group %i. Had %ld particles\n", 18, gcount.v[18]);
    fprintf(stderr, "AHF id range %ld, %ld\n", min_id, max_id);

    //========================================================================
    //========================================================================

    if (gid == 0)
    {
        fprintf(stderr, "ERROR: %s\n"
                        "ERROR: No groups found!\n", inname);
        exit(1);
    }

    //========================================================================
    // We let this slide as a warning, but really it is a serious error
    // if a group has no particles in it. It means that *all* the particles
    // from a larger group have sunk into at least one "smaller" group. It
    // has been observed that one "smaller" group has had precisely the same
    // particles as its host. This is clearly a problem with the group
    // finder.
    //========================================================================
    for (j=1; j < gcount.len; j++)
    {
        if (gcount.v[j] == 0)
        {
            fprintf(stderr, "WARNING: %s\n"
                            "WARNING: All particles from group %i also belonged to a subgroup!\n", 
                            inname, j);
        }
    }

    fprintf(stderr, "%ld groups found.\n", gcount.len);

    fprintf(stderr, "Writing output to %s...\n", outname);

    //========================================================================
    // Now print out the new group file.
    //========================================================================

    fprintf(out, "%ld\n", nParticles);
    for (i=0; i < nParticles; i++)
    {
        if (list[i].len == 0)  // There may be particles that are not part
            fprintf(out, "0"); // of a group.
        else
        {
            //================================================================
            // Print the group list from smallest (deepest) to largest.
            // Include all larger halos if the -b flag was given.
            //================================================================
            int start = list[i].len-1;
            int end   = ((belong==0) * (list[i].len-1));

            fprintf(out, "%ld", list[i].v[start]);
            for (j=start-1; j >= end; j--)
                fprintf(out, " %ld", list[i].v[j]);

        }
        fprintf(out, "\n");
    }

    if (in  != stdin)  fclose(in);
    if (out != stdout) fclose(out);

    return 0;
}

