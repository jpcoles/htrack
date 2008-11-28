CC=gcc
CFLAGS=-Wall -g -O2
LDFLAGS=

all: pfind

pfind: pfind.c pfind.h misc.h
	$(CC) $(CFLAGS) $(LDFLAGS) pfind.c -o pfind

