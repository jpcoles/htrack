#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "getline.h"

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    int c;
    size_t offs;

    if (n == NULL) return -1;
    if (lineptr == NULL) return -1;

    if (*lineptr == NULL)
        *n = 0;

    offs = 0;

    while (!feof(stream) && !ferror(stream))
    {
        if (offs == *n)
        {
            if (*n == 0) *n = 32; else *n *= 2;
            *lineptr = realloc(*lineptr, *n + 1); // +1 for the NULL terminator
            if (*lineptr == NULL) return -1;
        }

        c = fgetc(stream);
        if (c == EOF) return -1;
        (*lineptr)[offs] = (char)c;
        offs++;

        if (c == '\n')
        {
            (*lineptr)[offs] = '\0';
            return offs;
        }
    }

    return -1;
}
