#!/usr/bin/make

WARNFLAGS = -W -Wall -Wundef -Wpointer-arith \
        -Wcast-align -Wwrite-strings -Wstrict-prototypes \
        -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
        -Wnested-externs
ifneq ($(GCCVER),2)
  WARNFLAGS += -Wno-unused-parameter 
endif

CFLAGS+=$(WARNFLAGS) -O2 -g
#CFLAGS+=$(WARNFLAGS) -O0 -g

LDFLAGS+=-lpng

PNG23D_OBJ=png23d.o bitmap.o mesh.o mesh_gen.o out_pgm.o out_scad.o out_pscad.o out_stl.o

.PHONY : all clean

all:png23d

png23d:$(PNG23D_OBJ)

-include $(PNG23D_OBJ:.o=.d)

# compile and generate dependency info
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) $*.c > $*.d


clean:
	${RM} png23d $(PNG23D_OBJ) *.d *~ 
