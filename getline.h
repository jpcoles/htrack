#ifndef GETLINE_H
#define GETLINE_H

#include <stdio.h>
#include <sys/types.h>

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#endif
