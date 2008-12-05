CC=gcc
CFLAGS=-Wall -g -O3 -I$(HOME)/tools -lm
LDFLAGS=

all: htrack pfind ahf2grp 

htrack: htrack.c htrack.h set.o cosmo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ htrack.c set.o cosmo.o

pfind: pfind.c pfind.h set.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ pfind.c set.o 

ahf2grp: ahf2grp.c set.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ ahf2grp.c set.o

set.o: set.c set.h
	$(CC) $(CFLAGS) -c set.c
	
cosmo.o: cosmo.c cosmo.h
	$(CC) $(CFLAGS) -c cosmo.c

