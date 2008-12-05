#include <math.h>
#include "cosmo.h"

//============================================================================
//                                cosmo_init0
//============================================================================
int cosmo_init0(cosmo_t *c)
{
    return cosmo_init1(c, 75);
}

//============================================================================
//                                cosmo_init1
//============================================================================
int cosmo_init1(cosmo_t *c, float H0)
{
    const float WM = 0.3;
    return cosmo_init3(c, H0, 0.3, 1.0 - WM - 0.4165/(H0*H0));
}

//============================================================================
//                                cosmo_init2
//============================================================================
int cosmo_init2(cosmo_t *c, float H0, float WM)
{
    return cosmo_init3(c, H0, WM, 0);
}

//============================================================================
//                                cosmo_init3
//============================================================================
int cosmo_init3(cosmo_t *c, float H0, float WM, float WV)
{
    c->H0 = H0;
    c->WM = WM;
    c->WV = WV;
    return 0;
}

//============================================================================
//                            cosmo_physical_time
//============================================================================
float cosmo_physical_time(cosmo_t *cosmo, float z)
{
    int i;

    const float H0 = cosmo->H0;
    const float WM = cosmo->WM;
    const float WV = cosmo->WV;

    //float a = 1.0        // 1/(1+z), the scale factor of the Universe

    //const float c     = 299792.458; // velocity of light in km/sec
    const float Tyr   = 977.8;      // coefficent for converting 1/H into Gyr

    const float h  = H0/100;

    /* Omega(radiation). includes 3 massless neutrino species, T0 = 2.72528 */
    const float WR = 4.165E-5/(h*h)  ;
    /* Omega curvaturve = 1-Omega(total) */
    const float WK = 1-WM-WR-WV;     

    const float az = 1/(1+z);
    float age = 0;
    const int n=1000;        // number of points in integrals
    for (i=0; i < n; i++)
    {
        float a = az*(i+0.5)/n;
        float adot  = sqrt(WK+(WM/a)+(WR/(a*a))+(WV*a*a));
        age  += 1/adot;
    }

    float zage     = az*age/n;
    float zage_Gyr = zage * (Tyr/H0);

    return zage_Gyr;

#if 0

    float ratio, x;

    float DTT = 0.0;    // time from z to now in units of 1/H0
    float DCMR = 0.0;   // comoving radial distance in units of c/H0

    // do integral over a=1/(1+z) from az to 1 in n steps, midpoint rule
    for (i=0; i < n; i++)
    {
        a    = az+(1-az)*(i+0.5)/n;
        adot = sqrt(WK+(WM/a)+(WR/(a*a))+(WV*a*a));
        DTT  += 1/adot;
        DCMR += 1/(a*adot);
    }

    DTT  = (1-az)*DTT/n;
    DCMR = (1-az)*DCMR/n;
    age  = DTT+zage;
    const float age_Gyr  = age  * (Tyr/H0);
    const float DTT_Gyr  = DTT  * (Tyr/H0);
    const float DCMR_Gyr = DCMR * (Tyr/H0);
    const float DCMR_Mpc = DCMR * (c/H0);

    // tangential comoving distance

    ratio = 1.00;
    x = sqrt(abs(WK))*DCMR;
    if (x > 0.1)
    {
        if (WK > 0)
          ratio = 0.5*(exp(x)-exp(-x))/x;
        else
          ratio = sin(x)/x;
    }
    else
    {
        float y = x*x;
        if (WK < 0) y = -y;
        ratio = 1 + y/6 + y*y/120;
    }

    const float DCMT   = ratio*DCMR;
    const float DA     = az * DCMT;     // angular size distance
    const float DA_Mpc = DA * (c/H0);
    const float kpc_DA = DA_Mpc/206.264806;
    const float DA_Gyr = DA * (Tyr/H0);
    const float DL     = DA / (az*az);      // luminosity distance
    const float DL_Mpc = DL * (c/H0);
    const float DL_Gyr = DL * (Tyr/H0);

// comoving volume computation

    ratio = 1.00;
    x = sqrt(abs(WK))*DCMR;
    if (x > 0.1)
    {
        if (WK > 0)
          ratio = (0.125*(exp(2*x)-exp(-2*x))-x/2)/(x*x*x/3);
        else
          ratio = (x/2 - sin(2*x)/4)/(x*x*x/3);
    }
    else
    {
        float y = x*x;
        if WK < 0: y = -y;
        ratio = 1. + y/5. + (2./105.)*y*y;
    }

    const float VCM   = ratio*pow(DCMR,3)/3.;
    const float V_Gpc = VCM * 4.*M_PI * pow(0.001*c/H0, 3);

  if verbose == 1:
    print 'For H_o = ' + '%1.1f' % H0 + ', Omega_M = ' + '%1.2f' % WM + ', Omega_vac = ',
    print '%1.2f' % WV + ', z = ' + '%1.3f' % z
    print 'It is now ' + '%1.1f' % age_Gyr + ' Gyr since the Big Bang.'
    print 'The age at redshift z was ' + '%1.1f' % zage_Gyr + ' Gyr.'
    print 'The light travel time was ' + '%1.1f' % DTT_Gyr + ' Gyr.'
    print 'The comoving radial distance, which goes into Hubbles law, is',
    print '%1.1f' % DCMR_Mpc + ' Mpc or ' + '%1.1f' % DCMR_Gyr + ' Gly.'
    print 'The comoving volume within redshift z is ' + '%1.1f' % V_Gpc + ' Gpc^3.'
    print 'The angular size distance D_A is ' + '%1.1f' % DA_Mpc + ' Mpc or',
    print '%1.1f' % DA_Gyr + ' Gly.'
    print 'This gives a scale of ' + '%.2f' % kpc_DA + ' kpc/".'
    print 'The luminosity distance D_L is ' + '%1.1f' % DL_Mpc + ' Mpc or ' + '%1.1f' % DL_Gyr + ' Gly.'
    print 'The distance modulus, m-M, is '+'%1.2f' % (5*log10(DL_Mpc*1e6)-5)
  else:
    print '%1.2f' % zage_Gyr,
    print '%1.2f' % DCMR_Mpc,
    print '%1.2f' % kpc_DA,
    print '%1.2f' % (5*log10(DL_Mpc*1e6)-5)

#endif

}

