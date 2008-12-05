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

#ifndef HTRACK_H

#define PROGRAM "htrack"
#define VERSION "0.1"
#define PROGRAM_ID PROGRAM " v" VERSION " (compiled on " __DATE__ ")"

/*****************************************************************************
 * Constants for file formats
 ****************************************************************************/
#define TIPSY_STANDARD  0
#define TIPSY_NATIVE    1
#define GADGET          2

#define ONLY_MASSES     1


/*****************************************************************************
 * Constants and Macros used to convert to GADGET units from Tipsy files.
 ****************************************************************************/
//float kel2kms2   = BOLTZMANN/(0.6*PROTONMASS)*1.e-10; 
//float rho_crit   = 1.9e-29 * pow(header1.HubbleParam,2);      /*! Units of g/cm^3 for Omega_m=1 */ 
//float rho_bar    = 0.019 * 1.9e-29 *pow(header1.HubbleParam,2); /*! Baryon density (indep of H) */ 
//float rho_z      = 1.e10*rho_msol/pow(header1.HubbleParam,2)/1.e9; 
//#define  BOLTZMANN         1.3806e-16
//#define  PROTONMASS        1.6726e-24

/* Define a few variables that are needed in a function that uses the scaling macros */
#define DEFINE_SCALE_CONSTANTS(header1) \
float H0         = 100. * header1.HubbleParam; \
float rho_msol   = 2.7755*10.*pow(header1.HubbleParam,2);       /*! Units of Msol/Mpc^3  */ \
float volume     = pow((header1.BoxSize/1.0e3/header1.HubbleParam),3); \
float fact_mass  = rho_msol * volume; \
float fact_scale = header1.BoxSize/1.0e3/header1.HubbleParam/(1.+header1.redshift); \
float fact_vel   = (header1.BoxSize/1.0e3/header1.HubbleParam * H0 / 2.894405) / (1+header1.redshift); 

/* Scale a position in p0 and store it in p1. Both must be 3-element arrays. */
#define SCALE_POS(p0, p1)  \
    do { \
        const float _s = (1.0e3F*iohdr.HubbleParam)*fact_scale*(1.0F+iohdr.redshift); \
        p1[0] = (p0[0] + 0.5F) * _s; \
        p1[1] = (p0[1] + 0.5F) * _s; \
        p1[2] = (p0[2] + 0.5F) * _s; \
    } while (0)

/* Scale a velocity in v0 and store it in v1. Both must be 3-element arrays. */
#define SCALE_VEL(v0, v1)  \
    do { \
        v1[0] = (v0[0]) / sqrt(iohdr.time) * fact_vel; \
        v1[1] = (v0[1]) / sqrt(iohdr.time) * fact_vel; \
        v1[2] = (v0[2]) / sqrt(iohdr.time) * fact_vel; \
    } while (0)
//#define SCALE_MASS(m) ((m) * cfg.RhoCritical * iohdr.HubbleParam * fact_mass)
#define SCALE_MASS(m) ((m) * 1.0 * iohdr.HubbleParam * fact_mass)
//#define SCALE_TEMP(t) ((t) * 1.5F * kel2kms2)
//#define SCALE_RHO(r)  ((r) * rho_z / 1.0e10F)


#define Pmass(i) (iohdr.mass[P[i].Type])
//#define Pmass(i) (P[i].Mass)

using namespace std;

/*****************************************************************************
 * Data structures
 ****************************************************************************/

struct io_header_1
{
    unsigned int      npart[6];
    double   mass[6];
    double   time;
    double   redshift;
    int      flag_sfr;
    int      flag_feedback;
    int      npartTotal[6];
    int      flag_cooling;
    int      num_files;
    double   BoxSize;
    double   Omega0;
    double   OmegaLambda;
    double   HubbleParam; 
    char     fill[256- 6*4- 6*8- 2*8- 2*4- 6*4- 2*4 - 4*8];  /* fills to 256 Bytes */
} iohdr;

struct particle_data 
{
    int   Type;
#if !ONLY_MASSES
    float  Pos[3];
    float  Vel[3];
#endif
    //float  Mass;
    //float  Rho, U, Temp, Ne;
} *P;


/*****************************************************************************
 * Prototypes
 ****************************************************************************/
void make_z0groups(uint32_t *pmap, z0group_t **groups0, uint32_t *nGroups0);
void make_zXgroups(uint32_t *pmap, zXgroup_t **groups0, uint32_t *nGroups0);
//int EvalGroups1(uint32_t *idx);
void EvalProgenitors(FILE *fp, uint32_t *pmap, uint32_t *ps, uint32_t npart);
uint32_t read_pmap(char *fname, 
              float fEpsf, 
              float fPeriodf1, float fPeriodf2, float fPeriodf3, 
              uint32_t nf, uint32_t *pmap);
uint32_t read_pmap_ascii(char *fname, uint32_t *NumPart0, uint32_t **pmap, uint32_t *NumGroup);
uint32_t write_pmap_ascii(char *fname, uint32_t NumPart0, uint32_t *pmap);

uint32_t write_pmap(char *fname, 
               float fEpsf, 
               float fPeriodf1, float fPeriodf2, float fPeriodf3, 
               uint32_t nf, uint32_t *pmap);
uint32_t convert_pmap(char *fname_in, char *fname_out);

gmap_t *make_gmap(uint32_t *idx, uint32_t Ngroups);
uint32_t read_gmap(char *fname, gmap_t **gmap0, uint32_t *nGroups);
uint32_t write_gmap(char *fname, gmap_t *gmap0, uint32_t nGroups0, uint32_t *pmap);
uint32_t read_z0groups(char *fname, z0group_t **groups0, uint32_t *nGroups0);
uint32_t read_fof_z0groups(FILE *in, z0group_t **groups0, uint32_t *nGroups0);
uint32_t read_skid_z0groups(FILE *in, z0group_t **groups0, uint32_t *nGroups0);
uint32_t read_skid15_z0groups(FILE *in, z0group_t **groups0, uint32_t *nGroups0);
uint32_t read_ahf_z0groups(FILE *in, z0group_t **groups0, uint32_t *nGroups0);
uint32_t find_progenitors(uint32_t*,uint32_t*,uint32_t);
uint32_t load_gadget(char *fname, uint32_t files);
uint32_t load_tipsy(char *filename, uint32_t format);
uint32_t load_snapshot(char *filename, uint32_t format);

int allocate_memory(void);
bool is_valid_mass(float m);
int compar_prog_files(const void *a0, const void *b0);
int compar_progs(const void *a0, const void *b0);
void print_stamp(FILE *fp);


#endif 

