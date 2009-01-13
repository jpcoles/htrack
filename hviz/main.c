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
            fprintf(stderr, "ASDFAS %s\n", line);
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
    env.max_m = 0;

    float xmin=1e10,xmax=-1e10;
    float ymin=1e10,ymax=-1e10;
    float zmin=1e10,zmax=-1e10;

    float vxmin=1e10,vxmax=-1e10;
    float vymin=1e10,vymax=-1e10;
    float vzmin=1e10,vzmax=-1e10;
    float mmax=-1e10;

    int i,j;
    for (i=0; i < env.t_max; i++)
    {
        for (j=1; j <= env.hl[i].n_halos; j++)
        {
            xmax = fmax(xmax, env.hl[i].halo[j].Xc);
            ymax = fmax(ymax, env.hl[i].halo[j].Yc);
            zmax = fmax(zmax, env.hl[i].halo[j].Zc);

            xmin = fmin(xmin, env.hl[i].halo[j].Xc);
            ymin = fmin(ymin, env.hl[i].halo[j].Yc);
            zmin = fmin(zmin, env.hl[i].halo[j].Zc);

            vxmax = fmax(vxmax, env.hl[i].halo[j].VXc);
            vymax = fmax(vymax, env.hl[i].halo[j].VYc);
            vzmax = fmax(vzmax, env.hl[i].halo[j].VZc);

            vxmin = fmin(vxmin, env.hl[i].halo[j].VXc);
            vymin = fmin(vymin, env.hl[i].halo[j].VYc);
            vzmin = fmin(vzmin, env.hl[i].halo[j].VZc);

            mmax = fmax(mmax, env.hl[i].halo[j].Mvir);
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

    for (i=0; i < env.t_max; i++)
    {
        for (j=1; j <= env.hl[i].n_halos; j++)
        {
            env.hl[i].halo[j].w.Xc = (env.hl[i].halo[j].Xc - cx) / env.max_x;
            env.hl[i].halo[j].w.Yc = (env.hl[i].halo[j].Yc - cy) / env.max_x;
            env.hl[i].halo[j].w.Zc = (env.hl[i].halo[j].Zc - cz) / env.max_x;

            env.hl[i].halo[j].w.Rvir = env.hl[i].halo[j].Rvir / env.max_x;
            env.hl[i].halo[j].w.Mvir = env.hl[i].halo[j].Mvir / env.max_m;

            env.hl[i].halo[j].w.Xc = (env.hl[i].halo[j].Xc - cx) / env.max_x;
            env.hl[i].halo[j].w.Yc = (env.hl[i].halo[j].Yc - cy) / env.max_x;
            env.hl[i].halo[j].w.Zc = (env.hl[i].halo[j].Zc - cz) / env.max_x;

            env.hl[i].halo[j].w.Mvir       = env.hl[i].halo[j].Mvir;
            env.hl[i].halo[j].w.Rvir       = env.hl[i].halo[j].Rvir / env.max_x * 0.001;
            env.hl[i].halo[j].w.Vmax       = env.hl[i].halo[j].Vmax;
            env.hl[i].halo[j].w.Rmax       = env.hl[i].halo[j].Rmax / env.max_x * 0.001;
            env.hl[i].halo[j].w.sigV       = env.hl[i].halo[j].sigV;
            env.hl[i].halo[j].w.lambda     = env.hl[i].halo[j].lambda;
            env.hl[i].halo[j].w.Lx         = env.hl[i].halo[j].Lx;
            env.hl[i].halo[j].w.Ly         = env.hl[i].halo[j].Ly;
            env.hl[i].halo[j].w.Lz         = env.hl[i].halo[j].Lz;
            env.hl[i].halo[j].w.a          = env.hl[i].halo[j].a;
            env.hl[i].halo[j].w.Eax        = env.hl[i].halo[j].Eax;
            env.hl[i].halo[j].w.Eay        = env.hl[i].halo[j].Eay;
            env.hl[i].halo[j].w.Eaz        = env.hl[i].halo[j].Eaz;
            env.hl[i].halo[j].w.b          = env.hl[i].halo[j].b;
            env.hl[i].halo[j].w.Ebx        = env.hl[i].halo[j].Ebx;
            env.hl[i].halo[j].w.Eby        = env.hl[i].halo[j].Eby;
            env.hl[i].halo[j].w.Ebz        = env.hl[i].halo[j].Ebz;
            env.hl[i].halo[j].w.c          = env.hl[i].halo[j].c;
            env.hl[i].halo[j].w.Ecx        = env.hl[i].halo[j].Ecx;
            env.hl[i].halo[j].w.Ecy        = env.hl[i].halo[j].Ecy;
            env.hl[i].halo[j].w.Ecz        = env.hl[i].halo[j].Ecz;
            env.hl[i].halo[j].w.ovdens     = env.hl[i].halo[j].ovdens;
            env.hl[i].halo[j].w.Redge      = env.hl[i].halo[j].Redge;
            env.hl[i].halo[j].w.nbins      = env.hl[i].halo[j].nbins;
            env.hl[i].halo[j].w.Ekin       = env.hl[i].halo[j].Ekin;
            env.hl[i].halo[j].w.Epot       = env.hl[i].halo[j].Epot;
            env.hl[i].halo[j].w.mbp_offset = env.hl[i].halo[j].mbp_offset;
            env.hl[i].halo[j].w.com_offset = env.hl[i].halo[j].com_offset;
            env.hl[i].halo[j].w.r2         = env.hl[i].halo[j].r2;
            env.hl[i].halo[j].w.lambdaE    = env.hl[i].halo[j].lambdaE;

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
