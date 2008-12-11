#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "set.h"

//============================================================================
//                                  list_init
//============================================================================
int list_init(list_t *list)
{
    memset(list, 0, sizeof(list_t));
    return 0;
}

//============================================================================
//                                  list_free
//============================================================================
int list_free(list_t *list)
{
    if (list->v != NULL)
    {
        free(list->v);
        list_init(list);
    }
    return 0;
}

//============================================================================
//                                 list_empty
//============================================================================
int list_empty(list_t *list)
{
    return list->len == 0;
}

//============================================================================
//                                   list_in
//============================================================================
int list_in(list_t *list, uint64_t val)
{
    int i;
    for (i=0; i < list->len; i++)
        if (list->v[i] == val) return i;

    return -1;
}

//============================================================================
//                              list_ensure_index
//============================================================================
int list_ensure_index(list_t *list, uint64_t index)
{
    assert(index >= 0);

    //========================================================================
    // Not in the list. Grow the list if necessary.
    //========================================================================
    if (index >= list->size)
    {
        if (list->size == 0) list->size = 32;
        while (list->size <= index) list->size *= 2;

        list->v = (uint64_t *)realloc(list->v, list->size * sizeof(uint64_t));
        if (list->v == NULL) return -1;
        memset(list->v + list->len, 0, list->size - list->len);

        /* IGNORE: No need to initialize anything. We keep track of how many valid objects there are. */
    }

    if (index >= list->len) list->len = index+1;

    return 0;
}

//============================================================================
//                                list_append
//============================================================================
int list_append(list_t *list, uint64_t val)
{
    return list_set(list, val, list->len);
}

//============================================================================
//                                  list_set
//============================================================================
int list_set(list_t *list, uint64_t val, uint64_t index)
{
    list_ensure_index(list, index);
    list->v[index] = val;
    return index;
}

//============================================================================
//                                  set_init
//============================================================================
int set_init(set_t *set)
{
    return list_init(set);
}

//============================================================================
//                                  set_free
//============================================================================
int set_free(set_t *set)
{
    return list_free(set);
}

//============================================================================
//                                 set_empty
//============================================================================
int set_empty(set_t *set)
{
    return list_empty(set);
}

//============================================================================
//                                   set_in
//============================================================================
int set_in(set_t *set, uint64_t val)
{
    return list_in(set, val);
}

//============================================================================
//                                 add_to_set
//============================================================================
int set_add(set_t *set, uint64_t val)
{
    int index = list_in(set, val);
    if (index >= 0) return index;
    return list_append(set, val);
}

