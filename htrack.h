#ifndef HTRACK_H
#define HTRACK_H

#include <stdint.h>
#include "set.h"

//============================================================================
//                                  group_t
//============================================================================
typedef struct
{
    uint64_t id,        /* This group's id                      */
             pid;       /* It's progenitor's id                 */
    //float Mvir;         /* Mass                       (M_sun/h) */
    //float R;            
    //float r[3];         /* Position                   (Mpc/h)   */
    //float v[3];         /* Peculiar velocity          (km/s)    */
    //float vMax;         /* Maximum of rotation curve. (km/s)    */
    list_t ps;
    list_t belong;
} group_t;

//============================================================================
//                                    z_t
//============================================================================
typedef struct
{
    int n_groups;
    group_t *g;
    int *used;
} z_t;

//============================================================================
//                                   work_t
//============================================================================
typedef struct
{
    float z;
    char *stats, *pf;
} work_t;

typedef struct
{
    uint64_t id;
    uint64_t *t;
} track_t;

#endif

