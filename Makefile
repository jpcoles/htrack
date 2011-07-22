CC=gcc
CFLAGS=-Wall -g -O3 -I$(HOME)/tools #-fnested-functions
LDFLAGS=-lm

all: htrack pfind ahf2grp 

htrack: htrack.c htrack.h set.o cosmo.o getline.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ htrack.c set.o cosmo.o getline.o

pfind: pfind.c pfind.h set.o getline.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ pfind.c set.o  getline.o

ahf2grp: ahf2grp.c set.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ ahf2grp.c set.o

set.o: set.c set.h
	$(CC) $(CFLAGS) -c set.c
	
cosmo.o: cosmo.c cosmo.h
	$(CC) $(CFLAGS) -c cosmo.c

getline.o: getline.c getline.h
	$(CC) $(CFLAGS) -c getline.c

clean:
	rm -f *.o htrack pfind ahf2grp
