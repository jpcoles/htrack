#ifndef SET_H
#define SET_H

#include <stdint.h>

typedef struct
{
    uint64_t *v;
    uint64_t len;   /* Length of valid data */
    uint64_t size;  /* Length allocated */
} list_t;

typedef list_t set_t;

#define EMPTY_LIST { NULL,0,0 }
#define EMPTY_SET  EMPTY_LIST

int set_init(set_t *set);
int set_free(set_t *set);
int set_empty(set_t *set);
int set_add(set_t *set, uint64_t val);
int set_in(set_t *set, uint64_t val);

int list_init(list_t *list);
int list_free(list_t *list);
int list_empty(list_t *list);
int list_ensure_index(list_t *list, uint64_t index);
int list_append(list_t *list, uint64_t val);
int list_set(list_t *list, uint64_t val, uint64_t index);
int list_in(list_t *list, uint64_t val);

#endif

