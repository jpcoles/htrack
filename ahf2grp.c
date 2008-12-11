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
#include <getopt.h>
#include "set.h"
#include "jpcmacros.h"

const int debug_level = 0;
#define DBG(_lvl_) if (debug_level >= (_lvl_))

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, "Usage: ahf2grp [-b] [-o grp-output] <#Particles> <AHF_particles>\n");
    exit(2);
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    int belong  = 0;
    size_t nParticles=0;
    FILE *in  = stdin, 
         *out = stdout;
    char *inname = NULL;
    char *outname = NULL;

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
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "hbo:", long_options, &option_index);
        if (c == -1)
           break;

        switch (c) {
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

    if (argc-optind < 2) help();

    if ((nParticles = atol(argv[optind])) < 0)
    {
        fprintf(stderr, "Bad number of particles %s\n", argv[optind]);
        exit(2);
    }
    optind++;

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

    //========================================================================
    //========================================================================

    set_t *list = CALLOC(set_t, nParticles);

    list_t gcount = EMPTY_LIST;
    list_append(&gcount, 0);  // Because gid's begin at 1 we need something at 0

    fprintf(stderr, "Reading input...\n");

    int i,j;
    int gid=0, pid, nGrpParticles;
    int line=0;

    //========================================================================
    // Process the file.
    //========================================================================
    while (!feof(in))
    {
        //====================================================================
        // A new group begins with the number of particles in it.
        //====================================================================
        if (fscanf(in, "%i", &nGrpParticles) == 1)
        {
            line++;
            gid++;

            //================================================================
            // We keep track of how many particles are actually in a given
            // group and have not sunk to lower groups. First, assume all
            // particles of this new group will not sink.
            //================================================================
            list_append(&gcount, nGrpParticles);

#if 0
            if (gid == 459) fprintf(stderr, "Group %i starts on line %i\n", gid, line);
            if (gid == 464) fprintf(stderr, "Group %i starts on line %i\n", gid, line);
#endif

            if (nGrpParticles == 0)
            {
                fprintf(stderr, "ERROR: %s\n"
                                "ERROR: Corrupt file? Group %i has 0 particles on line %i.\n", 
                                inname, gid, line);
                exit(1);
            }

            //================================================================
            // Read in each of those particle ids.
            //================================================================
            for (i=0; i < nGrpParticles; i++)
            {
                line++;
                if (fscanf(in, "%i", &pid) == 1)
                {
                    if (! (0 <= pid && pid < nParticles) )
                    {
                        fprintf(stderr, "ERROR: %s\n"
                                        "ERROR: Particle has bad id %i on line %i.\n", 
                                        inname, pid, line);
                        exit(1);
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
                            "WARNING: Group %i has no more particles!\n", 
                            inname, j);
        }
    }

    fprintf(stderr, "Writing output...\n");

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

