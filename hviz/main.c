#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
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

            fprintf(stderr, "n_halos=%ld  ", env.mt.n_halos);
            fprintf(stderr, "n_zs=%ld\n", env.mt.n_zs);
        }
    }

    while (!ret && !feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;

        GROW_ARRAY(env.hl, halo_list_t, env.t_max, allocd, 2, 32);
        env.hl[env.t_max].n_halos = 0;
        env.hl[env.t_max].halo    = NULL;

        char *p = (char *)memchr(line, '\n', len);
        if (p) *p = '\0';
        hin = fopen(line, "r"); assert(hin != NULL);
        ret = read_ahf_halos(hin, &env.hl[env.t_max].halo, &env.hl[env.t_max].n_halos);
        fclose(hin);

        env.t_max++;
    }

    assert(env.mt.n_zs == env.t_max);

    if (line != NULL) free(line);

    env.max_x = 0;
    env.max_v = 0;
    env.max_m = 0;

    float xmin=1e10,xmax=-1e10;
    float ymin=1e10,ymax=-1e10;
    float zmin=1e10,zmax=-1e10;

    float vxmin=1e10,vxmax=-1e10;
    float vymin=1e10,vymax=-1e10;
    float vzmin=1e10,vzmax=-1e10;
    float mmax=-1e10;

    int t,i;
    for (t=0; t < env.t_max; t++)
    {
        for (i=1; i <= env.hl[t].n_halos; i++)
        {
            xmax = fmax(xmax, env.hl[t].halo[i].Xc);
            ymax = fmax(ymax, env.hl[t].halo[i].Yc);
            zmax = fmax(zmax, env.hl[t].halo[i].Zc);

            xmin = fmin(xmin, env.hl[t].halo[i].Xc);
            ymin = fmin(ymin, env.hl[t].halo[i].Yc);
            zmin = fmin(zmin, env.hl[t].halo[i].Zc);

            vxmax = fmax(vxmax, env.hl[t].halo[i].VXc);
            vymax = fmax(vymax, env.hl[t].halo[i].VYc);
            vzmax = fmax(vzmax, env.hl[t].halo[i].VZc);

            vxmin = fmin(vxmin, env.hl[t].halo[i].VXc);
            vymin = fmin(vymin, env.hl[t].halo[i].VYc);
            vzmin = fmin(vzmin, env.hl[t].halo[i].VZc);

            mmax = fmax(mmax, env.hl[t].halo[i].Mvir);
        }
    }

    xmin = ymin = zmin = 0;
    vxmin = vymin = vzmin = 0;

    env.max_x = fmax(fmax(xmax-xmin, ymax-ymin), zmax-zmin);
    env.max_v = fmax(fmax(vxmax-vxmin, vymax-vymin), vzmax-vzmin);
    env.max_m = mmax;

    float cx = (xmax-xmin) / 2;
    float cy = (ymax-ymin) / 2;
    float cz = (zmax-zmin) / 2;

    fprintf(stderr, "max_x = %f\n", env.max_x);
    fprintf(stderr, "max_v = %f\n", env.max_v);
    fprintf(stderr, "max_m = %e\n", env.max_m);

    for (t=0; t < env.t_max; t++)
    {
        for (i=1; i <= env.hl[t].n_halos; i++)
        {
            env.hl[t].halo[i].w.Xc         = (env.hl[t].halo[i].Xc - cx) / env.max_x;
            env.hl[t].halo[i].w.Yc         = (env.hl[t].halo[i].Yc - cy) / env.max_x;
            env.hl[t].halo[i].w.Zc         = (env.hl[t].halo[i].Zc - cz) / env.max_x;

            env.hl[t].halo[i].w.VXc        = env.hl[t].halo[i].VXc;
            env.hl[t].halo[i].w.VYc        = env.hl[t].halo[i].VYc;
            env.hl[t].halo[i].w.VZc        = env.hl[t].halo[i].VZc;

            env.hl[t].halo[i].w.Mvir       = env.hl[t].halo[i].Mvir;
            env.hl[t].halo[i].w.Rvir       = env.hl[t].halo[i].Rvir / env.max_x * 0.001;
            env.hl[t].halo[i].w.Vmax       = env.hl[t].halo[i].Vmax;
            env.hl[t].halo[i].w.Rmax       = env.hl[t].halo[i].Rmax / env.max_x * 0.001;
            env.hl[t].halo[i].w.sigV       = env.hl[t].halo[i].sigV;
            env.hl[t].halo[i].w.lambda     = env.hl[t].halo[i].lambda;
            env.hl[t].halo[i].w.Lx         = env.hl[t].halo[i].Lx;
            env.hl[t].halo[i].w.Ly         = env.hl[t].halo[i].Ly;
            env.hl[t].halo[i].w.Lz         = env.hl[t].halo[i].Lz;
            env.hl[t].halo[i].w.a          = env.hl[t].halo[i].a;
            env.hl[t].halo[i].w.Eax        = env.hl[t].halo[i].Eax;
            env.hl[t].halo[i].w.Eay        = env.hl[t].halo[i].Eay;
            env.hl[t].halo[i].w.Eaz        = env.hl[t].halo[i].Eaz;
            env.hl[t].halo[i].w.b          = env.hl[t].halo[i].b;
            env.hl[t].halo[i].w.Ebx        = env.hl[t].halo[i].Ebx;
            env.hl[t].halo[i].w.Eby        = env.hl[t].halo[i].Eby;
            env.hl[t].halo[i].w.Ebz        = env.hl[t].halo[i].Ebz;
            env.hl[t].halo[i].w.c          = env.hl[t].halo[i].c;
            env.hl[t].halo[i].w.Ecx        = env.hl[t].halo[i].Ecx;
            env.hl[t].halo[i].w.Ecy        = env.hl[t].halo[i].Ecy;
            env.hl[t].halo[i].w.Ecz        = env.hl[t].halo[i].Ecz;
            env.hl[t].halo[i].w.ovdens     = env.hl[t].halo[i].ovdens;
            env.hl[t].halo[i].w.Redge      = env.hl[t].halo[i].Redge;
            env.hl[t].halo[i].w.nbins      = env.hl[t].halo[i].nbins;
            env.hl[t].halo[i].w.Ekin       = env.hl[t].halo[i].Ekin;
            env.hl[t].halo[i].w.Epot       = env.hl[t].halo[i].Epot;
            env.hl[t].halo[i].w.mbp_offset = env.hl[t].halo[i].mbp_offset;
            env.hl[t].halo[i].w.com_offset = env.hl[t].halo[i].com_offset;
            env.hl[t].halo[i].w.r2         = env.hl[t].halo[i].r2;
            env.hl[t].halo[i].w.lambdaE    = env.hl[t].halo[i].lambdaE;

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

    memset(&env, 0, sizeof(Environment));

    env.t = 0;
    env.background = BLACK;
    env.fullscreen = 0;
    env.screenWidth = 1024;
    env.screenHeight = 768;
    env.eye.x = env.eye.ox = 0.840533; 
    env.eye.y = env.eye.oy = 0.464536;
    env.eye.z = env.eye.oz = 0.568467;

    //env.eye.x = env.eye.ox = 0; 
    //env.eye.y = env.eye.oy = 0;
    //env.eye.z = env.eye.oz = .01;
     
    env.eye.tx = 0;
    env.eye.ty = 0;
    env.eye.tz = -1;
    env.eye.ux = 0;
    env.eye.uy = 1;
    env.eye.uz = 0;
    env.make_movie = 0;
    env.movie_prefix = NULL;
    env.frame_buffer = NULL;
    env.current_movie_frame = 0;
    env.mode = MODE_TRACK | MODE_HALOBODIES;
    //env.mode = MODE_HALOBODIES;
    env.track_id = 413;
    env.mouse_down = 0;
    env.dirty = 1;
    env.max_level = 0;
    env.level = 0;

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

    env.level = env.max_level;

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
