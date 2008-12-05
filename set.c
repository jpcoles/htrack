#include <stdlib.h>
#include <string.h>
#include "set.h"

//============================================================================
//                                  set_init
//============================================================================
int set_init(set_t *set)
{
    memset(set, 0, sizeof(set_t));
    return 0;
}

//============================================================================
//                                  set_free
//============================================================================
int set_free(set_t *set)
{
    if (set->v != NULL)
    {
        free(set->v);
        set_init(set);
    }
    return 0;
}

//============================================================================
//                                 add_to_set
//============================================================================
int set_add(set_t *set, uint64_t val)
{
    if (set_in(set, val)) return 0;

    //========================================================================
    // Not in the list. Grow the list if necessary.
    //========================================================================
    if (set->len == set->size)
    {
        if (set->size == 0) set->size = 32;
        else set->size *= 2;

        set->v = (uint64_t *)realloc(set->v, set->size * sizeof(uint64_t));
        if (set->v == NULL) return -1;
        /* No need to initialize anything. We keep track of how many valid objects there are. */
    }

    //========================================================================
    // Add the group to the list.
    //========================================================================
    set->v[set->len] = val;
    set->len++;

    return 0;
}

//============================================================================
//                                   set_in
//============================================================================
int set_in(set_t *set, uint64_t val)
{
    int i;
    for (i=0; i < set->len; i++)
        if (set->v[i] == val) return 1;

    return 0;
}

