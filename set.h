#ifndef SET_H
#define SET_H

#include <stdint.h>

typedef struct
{
    uint64_t *v;
    uint64_t len;   /* Length of valid data */
    uint64_t size;  /* Length allocated */
} set_t;

#define EMPTY_SET { NULL,0,0 };

int set_init(set_t *set);
int set_free(set_t *set);
int set_add(set_t *set, uint64_t val);
int set_in(set_t *set, uint64_t val);

#endif

