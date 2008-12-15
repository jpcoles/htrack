#ifndef IO_H
#define IO_H

#include "hviz.h"

int read_ahf_halos(FILE *in, halo_t **halos0, uint64_t *n_halos0);
int read_merger_tree(FILE *in, merger_tree_t *mt);

void save_frame_buffer();
void save_frame_buffer_fp(FILE *outfile);

#endif
