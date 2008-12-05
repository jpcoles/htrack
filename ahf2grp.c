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

const int debug_level = 0;
#define DBG(_lvl_) if (debug_level >= (_lvl_))

/* Halos must have at least n-1 parent halos to count. */
#define MIN_LEVEL -1

void help()
{
    fprintf(stderr, "Usage: ahf2tipgrp <#Particles> <AHF_particles> <grp-output>\n");
    exit(2);
}

int main(int argc, char **argv)
{
    int belong  = 0;
    size_t nParticles=0;
    FILE *in, *out;

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

        c = getopt_long(argc, argv, "hb", long_options, &option_index);
        if (c == -1)
           break;

        switch (c) {
            case 'h':
                help();
                break;
            case 'b':
                belong = 1;
                break;
        }
    }

    if (argc-optind < 3) help();

    if ((nParticles = atol(argv[optind])) < 0)
    {
        fprintf(stderr, "Bad number of particles %s\n", argv[optind]);
        exit(2);
    }
    optind++;

    if (!strcmp("-", argv[optind]))
    {
        in = stdin;
    }
    else if ((in = fopen(argv[optind], "r")) == NULL)
    {
        fprintf(stderr, "Can't open input file %s\n", argv[optind]);
        exit(2);
    }
    optind++;

    if (!strcmp("-", argv[optind]))
    {
        out = stdout;
    }
    else if ((out = fopen(argv[optind], "w")) == NULL)
    {
        fprintf(stderr, "Can't open output file %s\n", argv[optind]);
        exit(2);
    }
    optind++;

    //========================================================================
    //========================================================================

    set_t *list = (set_t *)calloc(nParticles, sizeof(set_t));

    fprintf(stderr, "Reading input...\n");

    int i,j;
    int gid=0, pid, nGrpParticles;
    while (!feof(in))
    {
        if (fscanf(in, "%i", &nGrpParticles) == 1)
        {
            gid++;

            if (nGrpParticles == 0)
            {
                fprintf(stderr, "Corrupt file? Group %i has 0 particles.\n", gid);
                exit(1);
            }

            for (i=0; i < nGrpParticles; i++)
            {
                if (fscanf(in, "%i", &pid) == 1)
                {
                    if (! (0 <= pid && pid < nParticles) )
                    {
                        fprintf(stderr, "ERROR: Particle has id %i.\n", pid);
                        exit(1);
                    }

                    set_add(list+pid, gid);
                }
            }

        }
    }

    //========================================================================
    //========================================================================

    if (gid == 0)
    {
        fprintf(stderr, "ERROR: No groups found!\n");
        exit(1);
    }

    fprintf(stderr, "Writing output...\n");

    fprintf(out, "%ld\n", nParticles);

    for (i=0; i < nParticles; i++)
    {
        if (list[i].len == 0)
            fprintf(out, "0");
        else
        {
            int end = ((belong==0) * (list[i].len-1));
            for (j=list[i].len-1; j >= end; j--)
                fprintf(out, "%ld ", list[i].v[j]);
        }
        fprintf(out, "\n");
    }

    if (in  != stdin)  fclose(in);
    if (out != stdout) fclose(out);

    return 0;
}

