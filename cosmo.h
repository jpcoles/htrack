#ifndef COSMO_H
#define COSMO_H

typedef struct
{
    float H0, WM, WV;
} cosmo_t;

int cosmo_init0(cosmo_t *c);
int cosmo_init1(cosmo_t *c, float H0);
int cosmo_init2(cosmo_t *c, float H0, float WM);
int cosmo_init3(cosmo_t *c, float H0, float WM, float WV);
float cosmo_physical_time(cosmo_t *c, float z);


#endif

