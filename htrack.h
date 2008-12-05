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
    float Mvir;         /* Mass                       (M_sun/h) */
    float r[3];         /* Position                   (Mpc/h)   */
    float v[3];         /* Peculiar velocity          (km/s)    */
    float vMax;         /* Maximum of rotation curve. (km/s)    */
    set_t ps;
} group_t;

//============================================================================
//                                    z_t
//============================================================================
typedef struct
{
    int n_groups;
    group_t *g;
} z_t;

#endif

