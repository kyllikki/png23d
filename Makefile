#!/usr/bin/make

CFLAGS+=-Wall -O2

LDFLAGS+=-lpng

.PHONY : all clean

all:png23d

png23d:png23d.o bitmap.o

clean:
	${RM} png23d png23d.o bitmap.o *~
