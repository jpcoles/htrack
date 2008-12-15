#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "io.h"
#include "display.h"
#include "hviz.h"
#if defined(HAVE_GPU) && defined(HAVE_NVIDIA_GPU)
#include "nvidia.h"
#endif

Environment env;

//============================================================================
//                              load_halo_files
//============================================================================
int load_halo_files(FILE *in)
{
    FILE *hin;
    char *line = NULL;
    ssize_t read;
    size_t len;
    size_t allocd = 0;
    int ret = 0;

    if (!ret && !feof(in))
    {
        if ( !(getline(&line, &len, in) <= 0 || line[0] == '#') )
        {
            char *p = (char *)memchr(line, '\n', len);
            if (p) *p = '\0';
            hin = fopen(line, "r"); assert(hin != NULL);
            ret = read_merger_tree(hin, &env.mt);
            fclose(hin);

            fprintf(stderr, "n_halos=%ld  n_zs=%ld\n", env.mt.n_halos, env.mt.n_zs);
        }
    }

    while (!ret && !feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;

        char *p = (char *)memchr(line, '\n', len);
        if (p) *p = '\0';
        hin = fopen(line, "r"); assert(hin != NULL);
        GROW_ARRAY(env.hl, halo_list_t, env.t_max, allocd, 2, 32);
        env.hl[env.t_max].n_halos = 0;
        env.hl[env.t_max].halo    = NULL;
        ret = read_ahf_halos(hin, &env.hl[env.t_max].halo, &env.hl[env.t_max].n_halos);
        fclose(hin);
        env.t_max++;
    }

    assert(env.mt.n_zs == env.t_max);

    if (line != NULL) free(line);

    env.max_x = 0;
    env.max_v = 0;
    int i,j;
    for (i=0; i < env.t_max; i++)
    {
        for (j=1; j <= env.hl[i].n_halos; j++)
        {
            if      (env.hl[i].halo[j].Xc > env.max_x) env.max_x = env.hl[i].halo[j].Xc;
            else if (env.hl[i].halo[j].Yc > env.max_x) env.max_x = env.hl[i].halo[j].Yc;
            else if (env.hl[i].halo[j].Zc > env.max_x) env.max_x = env.hl[i].halo[j].Zc;

            if      (env.hl[i].halo[j].VXc > env.max_v) env.max_v = env.hl[i].halo[j].VXc;
            else if (env.hl[i].halo[j].VYc > env.max_v) env.max_v = env.hl[i].halo[j].VYc;
            else if (env.hl[i].halo[j].VZc > env.max_v) env.max_v = env.hl[i].halo[j].VZc;
        }
    }

    return 0;
}

//============================================================================
//                                    help
//============================================================================
void help()
{
    fprintf(stderr, "hviz\n");
    exit(1);
}

//============================================================================
//                                    main
//============================================================================
int main(int argc, char **argv)
{
    FILE *in = stdin;

    static struct option long_options[] = {
        {"movie",  required_argument, 0, 0},
        {"frames",  required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {"bcolor", required_argument, 0, 0},
        {"fullscreen", no_argument, 0, 0},
        {0, 0, 0, 0}
    };

    env.t = 0;
    env.background = BLACK;
    env.fullscreen = 0;
    env.screenWidth = 1024;
    env.screenHeight = 768;
    env.eye.x = env.eye.ox = 0; 
    env.eye.y = env.eye.oy = 3;
    env.eye.z = env.eye.oz = 3;
    env.eye.angle = 0;
    env.eye.roll = 
    env.eye.pitch =
    env.eye.heading = 0;
    env.make_movie = 0;
    env.movie_prefix = NULL;
    env.frame_buffer = NULL;
    env.current_movie_frame = 0;

    //if (argc < 2) help();

    /*========================================================================
     * Process the command line flags
     *======================================================================*/
    while (1)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "ho:",
                            long_options, &option_index);

        if (c == -1) break;

        switch (c)
        {
            case 0:
                if (!strcmp("movie", long_options[option_index].name))
                {
                    env.make_movie = 1;
                    if (optarg)
                    {
                        if (sscanf(optarg, "%ix%i", &(env.screenWidth), &(env.screenHeight)) != 2)
                            help();
                    }
                } 
                else if (!strcmp("fullscreen", long_options[option_index].name))
                {
                    env.fullscreen = 1;
                }
                else if (!strcmp("bcolor", long_options[option_index].name))
                {
                    if (!strcasecmp(optarg, "black"))
                        env.background = BLACK;
                    else if (!strcasecmp(optarg, "white"))
                        env.background = WHITE;
                    else
                    {
                        fprintf(stderr, "ERROR: Background color can be only black or white\n");
                        exit(2);
                    }
                }
                break;

            case 'h': help(); break;
            case 'o': 
                env.movie_prefix = MALLOC(char, strlen(optarg)+1); assert(env.movie_prefix != NULL);
                strcpy(env.movie_prefix, optarg);
                break;
            case '?': break;
        }
    }

    if (env.screenWidth <= 0 || env.screenHeight <= 0) help();
    if (optind < argc)
    {
        in = fopen(argv[optind], "r");
        assert(in != NULL);
    }

    load_halo_files(in);
    if (in != stdin) fclose(in);


#if defined(HAVE_GPU) && defined(HAVE_NVIDIA_GPU)
    nvidia_init();
#endif

    //========================================================================
    // Allocate frame buffer if we are making movie frames.
    //========================================================================
    env.frame_buffer = (JSAMPROW)MALLOC(char, env.screenWidth * env.screenHeight * 4); 
    assert(env.frame_buffer != NULL);

    if (env.make_movie)
    {

        if (env.movie_prefix == NULL)
        {
            env.movie_prefix = MALLOC(char, strlen("spray")+1); assert(env.movie_prefix != NULL);
            strcpy(env.movie_prefix, "spray");
        }
        printf("Making a %ix%i movie with prefix %s.\n", 
               env.screenWidth, env.screenHeight, env.movie_prefix);
    }


    display_start(argc, argv); // never returns

    return 0;
}
