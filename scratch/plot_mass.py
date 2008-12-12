from pylab import plot, show, subplot, figure, title, gca, ylim, axvline, xlabel, ylabel
from numpy import loadtxt, argsort, amax, array, vectorize, sqrt, ceil

select=[] #[19,63,142,159,165,384,406]


@vectorize
def cosmo_physical_time(z, H0=70, WM=0.3, WV=0.7):
    Tyr = 977.8      # coefficent for converting 1/H into Gyr

    h = H0/100.

    # Omega(radiation). includes 3 massless neutrino species, T0 = 2.72528 
    WR = 4.165E-5/(h*h)
    # Omega curvaturve = 1-Omega(total) 
    WK = 1-WM-WR-WV

    az = 1/(1.0+z)
    age = 0
    n=1000              # number of points in integrals
    for i in range(n):
        a     = az*(i+0.5)/n
        adot  = sqrt(WK+(WM/a)+(WR/(a*a))+(WV*a*a))
        age  += 1/adot

    zage     = az*age/n
    zage_Gyr = zage * (Tyr/H0)

    return zage_Gyr

z1 = cosmo_physical_time(0) - cosmo_physical_time(1)

def p(f, fig=None, c='k', xs=None):
    I = loadtxt("%s.i.ht" % f)
    R = loadtxt("%s.r.ht" % f)
    V = loadtxt("%s.v.ht" % f)
    M = loadtxt("%s.m.ht" % f)

    max_mass = amax(M,axis=1)
    print len(max_mass)

    mm_order = argsort(max_mass)
    print mm_order

    if fig is None:
        f1 = figure()
        #f1.suptitle(f)
    p = 0
    style=['-', '--', '-.', ':', '^--', 'o--']
    chosen = []
    for i in mm_order[-2::-1]:
        if R[i][0] == 0:

            chosen.append(i)

            s = "%s%s" % (c, style[p % len(style)])
            subplot(2,1,1)
            plot(xs, R[i], s, linewidth=1)
            xlabel("Gyr")
            ylabel("Mpc")
            ylim(0,0.3)
            axvline(z1, color='k', linewidth=1)

            subplot(2,1,2)
            plot(xs, V[i], s, linewidth=1)
            axvline(z1, color='k', linewidth=1)
            xlabel("Gyr")
            ylabel("km/s")

#           subplot(3,1,3)
#           plot(xs, M[i], s, linewidth=1)
#           axvline(z1, color='k', linewidth=1)

            p += 1
            if p == 60: break

    return chosen

def p_some(f, some, fig=None, xs=None):
    I = loadtxt("%s.i.ht" % f)
    R = loadtxt("%s.r.ht" % f)
    V = loadtxt("%s.v.ht" % f)
    M = loadtxt("%s.m.ht" % f)

    if not fig: figure()

    p = 1
    n = ceil(sqrt(len(some)))
    for i in some:
       subplot(n,n,p)
       plot(xs, V[i])
       ylim(ymax=200)

       p += 1
       if p == 100: break

zs = [ 0.000, 0.025, 0.051, 0.078, 0.107, 0.136, 0.166, 0.198, 0.231,
       0.266, 0.302, 0.340, 0.380, 0.466, 0.563, 0.672, 0.797, 0.943,
       1.025, 1.116, 1.215, 1.326, 1.449, 1.589, 1.932, 2.406, 3.119,
       4.358, 4.951, 5.452, 7.356, 11.805 ]

zs.reverse()

ts = cosmo_physical_time(array(zs))

f = figure()
h1 = p("halo1/halo1", f, 'k', xs=ts)
h2 = p("halo2/halo2", f, 'r', xs=ts)
h3 = p("halo3/halo3", f, 'g', xs=ts)
h4 = p("halo4/halo4", f, 'b', xs=ts)

h3.insert(0,1)
h3.insert(0,0)

print h1
print h2
print h3
print h4

p_some("halo1/halo1", h1, xs=ts)
p_some("halo2/halo2", h2, xs=ts)
p_some("halo3/halo3", h3, xs=ts)
p_some("halo4/halo4", h4, xs=ts)

#p_some("halo3/halo3", [0,1,529, 1545, 1412, 1737, 1415, 1919], xs=ts)
#p_some("halo3/halo3", [0, 1, 1730, 1545, 1412, 1921, 1546, 1922] , xs=ts)

show()


