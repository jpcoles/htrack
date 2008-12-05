/*
 * htrack v0.1
 * Copyright (C) 2008 Jonathan Coles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA
 */

#ifndef PFIND_H
#define PFIND_H

#include <stdint.h>

#define PROGRAM "pfind"
#define VERSION "0.1"
#define PROGRAM_ID PROGRAM " v" VERSION " (compiled on " __DATE__ ")"

/*****************************************************************************
 * Constants for file formats
 ****************************************************************************/
#define GROUP_FOF       1
#define GROUP_SKID      2
#define GROUP_SKID15    3
#define GROUP_AHF       4

typedef struct
{
    uint64_t id;
    set_t ps;
    set_t belong;
} group_t;

/*****************************************************************************
 * Prototypes
 ****************************************************************************/
uint32_t build_prog_list(FILE *fpD, FILE *fpP, group_t *groups, uint64_t n_groupsD, uint64_t n_groupsP);
uint32_t add_to_set(set_t *set, uint64_t val);
uint32_t sort_group_progenitors(group_t *groups, uint64_t n_groups);
uint32_t write_output_ascii(FILE *out, group_t *groups, uint64_t n_groups);
uint32_t read_ahf_groups_masses(FILE *in, float **masses0, uint64_t *n_groups0);
int      prog_mass_cmp(const void *a0, const void *b0);
void     help();

#endif

