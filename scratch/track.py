import sys

zfilelookup = [200,195,190,185,180,175,170,165,160,155,150,145,140,
              130,120,110,100,90,85,80,75,70,65,60,50,40,30,20,17,15,10,5]

zfile = [11.804572769,7.356103716,5.452070792,4.951141131,4.357850859,
         3.119311745,2.406284595,1.932164377,1.588842606,1.4492995,
         1.32571125,1.215227028,1.11566782,1.025308252,
         0.94278872,0.797064662,0.671857715,0.562587976,0.465985537,
         0.379645228,0.339744449,0.301768064,0.265555263,0.23096621,
         0.197873116,0.166163087,0.135736346,0.106503129,0.0783822536,
         0.0513014793,0.0251930952,0.0]
zfile.reverse()

for i in apply(range, sys.argv[1:]):
    for snap,z in zip(zfilelookup,zfile):
        jobname = "halo%i.%5f.job" % (i,snap)

