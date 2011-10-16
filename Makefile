#!/usr/bin/make

CFLAGS+=-Wall -O2 -g

LDFLAGS+=-lpng

PNG23D-OBJ=png23d.o bitmap.o out_pgm.o out_scad.o out_stl.o

.PHONY : all clean

all:png23d

png23d:$(PNG23D-OBJ)

clean:
	${RM} png23d $(PNG23D-OBJ)  *~
