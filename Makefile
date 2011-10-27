#!/usr/bin/make
#
# png23d is a program to convert png images into 3d files
#
# Copyright 2011 Vincent Sanders <vince@kyllikki.org>
#
# Released under the MIT License, 
#   http://www.opensource.org/licenses/mit-license.php

VERSION=100

WARNFLAGS = -W -Wall -Wundef -Wpointer-arith \
        -Wcast-align -Wwrite-strings -Wstrict-prototypes \
        -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
        -Wnested-externs
ifneq ($(GCCVER),2)
  WARNFLAGS += -Wno-unused-parameter 
endif

OPTFLAGS=-O2
#OPTFLAGS=-O0

CFLAGS+=$(WARNFLAGS) -MMD -DVERSION=$(VERSION) $(OPTFLAGS) -g

LDFLAGS+=-lpng

PNG23D_OBJ=png23d.o option.o bitmap.o mesh.o mesh_gen.o mesh_bloom.o out_pgm.o out_scad.o out_pscad.o out_stl.o

.PHONY : all clean

all:png23d

png23d:$(PNG23D_OBJ)

-include $(PNG23D_OBJ:.o=.d)

-include test/Makefile.sub

clean:
	${RM} png23d $(PNG23D_OBJ) *.d *~ 
