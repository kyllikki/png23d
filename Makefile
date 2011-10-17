#!/usr/bin/make

WARNFLAGS = -W -Wall -Wundef -Wpointer-arith \
        -Wcast-align -Wwrite-strings -Wstrict-prototypes \
        -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
        -Wnested-externs
ifneq ($(GCCVER),2)
  WARNFLAGS += -Wno-unused-parameter 
endif

CFLAGS+=$(WARNFLAGS) -O2 -g

LDFLAGS+=-lpng

PNG23D-OBJ=png23d.o bitmap.o out_pgm.o out_scad.o out_stl.o

.PHONY : all clean

all:png23d

png23d:$(PNG23D-OBJ)

clean:
	${RM} png23d $(PNG23D-OBJ)  *~
