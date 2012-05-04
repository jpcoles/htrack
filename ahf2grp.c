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
#include "set.h"
#include "jpcmacros.h"
#include "getline.h"

const int debug_level = 0;
#define DBG(_lvl_) if (debug_level >= (_lvl_))

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

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, "Usage: ahf2grp [--dm-grp] [-b] [-o grp-output] [-N <#particles>] [--tipsy-file <file>] <.AHF_particles>\n");
    exit(2);
}

uint32_t bswap32(uint32_t b)
{
    char *b0 = (char *)&b; 
    uint32_t data[4] = {(b >> 0) & 0xFF, (b >> 8) & 0xFF, (b>>16) & 0xFF, (b>>24)&0xFF};

    //fprintf(stderr, "%0x\n", data[0]);
    //fprintf(stderr, "%0x\n", data[1]);
    //fprintf(stderr, "%0x\n", data[2]);
    //fprintf(stderr, "%0x\n", data[3]);
    return (((uint32_t)data[3])<<0) | (((uint32_t)data[2])<<8) | (((uint32_t)data[1])<<16) | (((uint32_t)data[0])<<24);
    //fprintf(stderr, "* %i\n", x);
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

    fprintf(stderr, "%ld\n", sizeof(*h));

    if (fread(h, sizeof(struct tipsy_header), 1, fp) != 1)
    {
        fprintf(stderr, "Error reading tipsy file %s\n", fname);
        exit(1);
    }

    if (h->h_nDims != 3) // might not be native format
    {
        fprintf(stderr, "Trying to byte swap\n");

        h->h_nBodies = bswap32(h->h_nBodies);
        h->h_nDims   = bswap32(h->h_nDims);
        h->h_nGas    = bswap32(h->h_nGas);
        h->h_nDark   = bswap32(h->h_nDark);
        h->h_nStar   = bswap32(h->h_nStar);

        if (h->h_nDims != 3)
        {
            fprintf(stderr, "Tipsy file possibly corrupt.\n");
            exit(1);
        }
    }

    fclose(fp);
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    int belong  = 0;
    int dm_grp  = 0;
    size_t nParticles=0;
    size_t min_particle_id, max_particle_id;
    FILE *in  = stdin, 
         *out = stdout;
    char *inname = NULL;
    char *outname = NULL;
    char *tipsyname = NULL;
    struct tipsy_header h;

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
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "N:hbo:", long_options, &option_index);
        if (c == -1)
           break;


        switch (c) {
            case 0:
                if (!strcmp("dm-grp", long_options[option_index].name))
                    dm_grp = 1;
                if (!strcmp("tipsy-file", long_options[option_index].name))
                    tipsyname = optarg;
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
            case 'N':
                nParticles = atol(optarg);
                if (nParticles <= 0)
                {
                    fprintf(stderr, "Bad number of particles %ld\n", nParticles);
                    exit(2);
                }
                break;
        }
    }

    if (argc-optind < 1) help();

    // Assume we want to take all the particles.
    min_particle_id = 0;
    max_particle_id = nParticles;

    if (tipsyname != NULL)
    {
        read_tipsy_header(tipsyname, &h);

        fprintf(stderr, "Tipsy file %s\n", tipsyname);
        fprintf(stderr, "  Time:            %f\n",  h.h_time);
        fprintf(stderr, "  Gas particles:   %i\n", h.h_nGas);
        fprintf(stderr, "  DM particles:    %i\n", h.h_nDark);
        fprintf(stderr, "  Star particles:  %i\n", h.h_nStar);
        fprintf(stderr, "  Total particles: %i\n", h.h_nBodies);

        nParticles = h.h_nBodies;

        if (dm_grp)
        {
            min_particle_id = h.h_nGas;
            max_particle_id = h.h_nGas + h.h_nDark - 1;

            min_particle_id = 0;
            max_particle_id = h.h_nDark - 1;
            nParticles = max_particle_id - min_particle_id + 1;

            fprintf(stderr, "Taking only dark matter particles (id: %ld-%ld)\n", min_particle_id, max_particle_id);
        }
    }
    else
    {
        if (nParticles == 0)
        {
            fprintf(stderr, "Without a tipsy file, the number of particles (-N) must be given.\n");
            exit(2);
        }
    }


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
    int gid=0, nGrpParticles;
    long pid;
    int line=0;
    char *linestr = NULL;
    size_t line_len;

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
                    fprintf(stderr, "Unexpected EOF of %s.\n", inname);
                    exit(1);
                }
                if (sscanf(linestr, "%ld", &pid) >= 1)
                {
                    if (pid < min_id) min_id = pid;
                    if (pid > max_id) max_id = pid;

                    if (! (min_particle_id <= pid && pid <= max_particle_id) )
                    {
                        if (!dm_grp)
                        {
                            fprintf(stderr, "ERROR: %s\n"
                                            "ERROR: Particle has bad id %ld on line %i.\n", 
                                            inname, pid, line);
                            exit(1);
                        }

                        continue;
                    }

                    pid -= min_particle_id;

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

