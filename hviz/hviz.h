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


#define SIM_STOPPED 0
#define SIM_RUNNING 1
#define SIM_PAUSED  2

#define SVR_CMD_START_SIM   1000
#define SVR_CMD_NEXT_FRAME  1001
#define SVR_CMD_STOP_SIM    1002

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
    double angle;

    float roll, pitch, heading;

} Eye;

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

    float max_x, max_v;

    int spinning;

    char *movie_prefix;
    int make_movie;
    int print_frame;
    JSAMPROW frame_buffer; 
    int current_movie_frame;

    int background;

} Environment;

#endif

