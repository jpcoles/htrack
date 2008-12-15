#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "hviz.h"
#include "io.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "jpeglib.h"
#ifdef __cplusplus
}
#endif

extern Environment env;

//============================================================================
//                               loadNextFrame
//============================================================================
int loadNextFrame()
{
    return 0;
}

//============================================================================
//                             save_frame_buffer
//============================================================================
void save_frame_buffer()
{
    assert(env.make_movie);

    FILE * outfile;
    static char *filename = NULL;
    if (filename == NULL)
        filename = MALLOC(char, strlen(env.movie_prefix)+20+1);

    sprintf(filename, "%s.%05i.jpg", env.movie_prefix, ++env.current_movie_frame);
    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "Can't open %s\n", filename);
        exit(1);
    }

    save_frame_buffer_fp(outfile);

    fclose(outfile);
}

//============================================================================
//                            save_frame_buffer_fp
//============================================================================
void save_frame_buffer_fp(FILE *outfile)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width      = env.screenWidth;
    cinfo.image_height     = env.screenHeight;
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];              /* pointer to a single row */
    int row_stride;                       /* physical row width in buffer */

    row_stride = cinfo.image_width * 3;   /* JSAMPLEs per row in image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = & (env.frame_buffer[(cinfo.image_height-cinfo.next_scanline-1) * row_stride]);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

}

//============================================================================
//                               read_ahf_halos
//============================================================================
int read_ahf_halos(FILE *in, halo_t **halos0, uint64_t *n_halos0)
{
    int ret=0;
    int read;

    char *line = NULL;
    size_t len;

    //VL(1) printf("Reading AHF format group file.\n");

    uint64_t n_halos=0;
    uint64_t allocd = 0;
    halo_t *halos = NULL;

    while (!ret && !feof(in))
    {
        /* Read the whole line */
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;

        GROW_ARRAY_BASEONE(halos, halo_t, n_halos, allocd, 2, 32);

#if 0
        if (n_halos == allocd)
        {
            if (allocd == 0) allocd = 32; else allocd *= 2;
            halos = REALLOC(halos, halo_t, allocd+1);
            assert(halos != NULL);
            MEMSET(halos + n_halos+1, 0, allocd-n_halos, halo_t);
        }
#endif

        n_halos++;

        /* Now extract just the 38 values */
        read = 
            sscanf(line, "%d %d "
                         "%g %g %g "
                         "%g %g %g "
                         "%g %g %g %g %g %g "
                         "%g %g %g "
                         "%g %g %g %g "
                         "%g %g %g %g "
                         "%g %g %g %g "
                         "%g %g %g "
                         "%g %g %g "
                         "%g %g %g ",
                &halos[n_halos].npart, 
                &halos[n_halos].nvpart,
                &halos[n_halos].Xc,
                &halos[n_halos].Yc,
                &halos[n_halos].Zc,
                &halos[n_halos].VXc, 
                &halos[n_halos].VYc, 
                &halos[n_halos].VZc,
                &halos[n_halos].Mvir, 
                &halos[n_halos].Rvir,
                &halos[n_halos].Vmax, 
                &halos[n_halos].Rmax,
                &halos[n_halos].sigV,
                &halos[n_halos].lambda,
                &halos[n_halos].Lx, 
                &halos[n_halos].Ly, 
                &halos[n_halos].Lz,
                &halos[n_halos].a,
                &halos[n_halos].Eax,
                &halos[n_halos].Eay,
                &halos[n_halos].Eaz,
                &halos[n_halos].b,
                &halos[n_halos].Ebx,
                &halos[n_halos].Eby,
                &halos[n_halos].Ebz,
                &halos[n_halos].c,
                &halos[n_halos].Ecx,
                &halos[n_halos].Ecy,
                &halos[n_halos].Ecz,
                &halos[n_halos].ovdens,
                &halos[n_halos].Redge,
                &halos[n_halos].nbins,
                &halos[n_halos].Ekin,
                &halos[n_halos].Epot,
                &halos[n_halos].mbp_offset,
                &halos[n_halos].com_offset,
                &halos[n_halos].r2,
                &halos[n_halos].lambdaE);

        if (read <= 0) continue; /* check for EOF at top of loop */

        if (read != 38) { ret = 3; break; }
    }

    MEMSET(halos, 0, 1, halo_t);

    if (line != NULL) free(NULL);

    *halos0   = halos;
    *n_halos0 = n_halos;

    return ret;
}

//============================================================================
//                              read_merger_tree
//============================================================================
int read_merger_tree(FILE *in, merger_tree_t *mt)
{
    int i, ret = 0;
    char *line = NULL;
    size_t len;
    size_t allocd=0;

    mt->n_halos = 0;
    mt->n_zs    = 0;
    mt->h       = NULL;
    while (!ret && !feof(in))
    {
        if (getline(&line, &len, in) <= 0 || line[0] == '#') continue;
        GROW_ARRAY(mt->h, uint64_t *, mt->n_halos, allocd, 2, 32);

        mt->h[mt->n_halos] = NULL;

        char *ap;
        char *lineptr = line;
        size_t allocd2 = 0;
        for (i=0; (ap = strsep(&lineptr, " \t\n")) != NULL; )
        {
            if (*ap != '\0')
            {
                GROW_ARRAY(mt->h[mt->n_halos], uint64_t, i, allocd2, 2, 32);
                mt->h[mt->n_halos][i] = atol(ap);
                i++;
            }
        }

        if (mt->n_halos > 0) assert(mt->n_zs == i);
        mt->n_zs = i;

        mt->n_halos++;
    }

    return ret;
}

