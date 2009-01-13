#ifndef STARSPRAY
#define STARSPRAY

#include <sys/socket.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "jpeglib.h"
#ifdef __cplusplus
}
#endif


#define MODE_NONE               0x00
#define MODE_TRACK              0x01
#define MODE_HALOTRACKS         0x02
#define MODE_HALOBODIES         0x04

#define MALLOC(type, n) (type *)malloc((n) * sizeof(type))
#define REALLOC(ptr, type, n) (type *)realloc((ptr), (n) * sizeof(type))
#define MEMSET(ptr, val, n, type) memset((ptr), (val), (n) * sizeof(type))

#define GROW_ARRAY(_ptr, _type, _len, _allocd, _fac, _initial) \
do { \
    if ((_len) >= (_allocd)) { \
        if ((_allocd) == 0) { (_allocd) = (_initial); } \
        while ((_allocd) <= (_len)) { (_allocd) *= (_fac); } \
        (_ptr) = REALLOC((_ptr), _type, (_allocd)); \
    } \
} while (0)

#define GROW_ARRAY_BASEONE(_ptr, _type, _len, _allocd, _fac, _initial) \
do { \
    if ((_len) >= (_allocd)) { \
        if ((_allocd) == 0) (_allocd) = (_initial); \
        while ((_allocd) <= (_len)) (_allocd) *= (_fac); \
        (_ptr) = REALLOC((_ptr), _type, (_allocd)+1); \
    } \
} while (0)

#define BLACK 0
#define WHITE 255

#define NO_MESSAGE -1
#define MSG_PRINTING 0
#define MSG_UPLOADING 1
#define MSG_LENSING 2
#define MSG_STILL_PRINTING 3

typedef struct
{
    float x, y, z;
} Coord;

typedef struct
{
    float ox, oy, oz; /* Original position */
    float x, y, z;

    float tx, ty, tz;
    float ux, uy, uz;

} Eye;

typedef struct
{
    float Xc, Yc, Zc;
    float VXc, VYc, VZc;
    float Mvir, Rvir;
    float Vmax, Rmax;
    float sigV;
    float lambda;
    float Lx, Ly, Lz;
    float a, Eax, Eay, Eaz;
    float b, Ebx, Eby, Ebz;
    float c, Ecx, Ecy, Ecz;
    float ovdens;
    float Redge;
    float nbins;
    float Ekin;
    float Epot;
    float mbp_offset;
    float com_offset;
    float r2;
    float lambdaE;
} world_t;

typedef struct
{
    int npart, nvpart;
    float Xc, Yc, Zc;
    float VXc, VYc, VZc;
    float Mvir, Rvir;
    float Vmax, Rmax;
    float sigV;
    float lambda;
    float Lx, Ly, Lz;
    float a, Eax, Eay, Eaz;
    float b, Ebx, Eby, Ebz;
    float c, Ecx, Ecy, Ecz;
    float ovdens;
    float Redge;
    float nbins;
    float Ekin;
    float Epot;
    float mbp_offset;
    float com_offset;
    float r2;
    float lambdaE;
    world_t w;
} halo_t;


typedef struct 
{
    uint64_t n_halos;
    halo_t *halo;
} halo_list_t;

typedef struct
{
    uint64_t n_halos, n_zs;
    uint64_t **h;
} merger_tree_t;

typedef struct
{
    int maxParticles;
    int fullscreen;

    int screenWidth;
    int screenHeight;

    Coord pointer;
    Eye eye;

    int t_max, t;
    halo_list_t *hl;
    merger_tree_t mt;

    float max_x, max_v, max_m;

    int spinning;

    char *movie_prefix;
    int make_movie;
    int print_frame;
    JSAMPROW frame_buffer; 
    int current_movie_frame;

    int background;

    int mode;

    int track_id;

    int mouse_down;

    int dirty;

    int selected_halo;

} Environment;

#endif

