/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * Routines to output in scad polyhedron format
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "option.h"
#include "bitmap.h"
#include "mesh.h"
#include "out_pscad.h"


/* ascii stl outout */
bool output_flat_scad_polyhedron(bitmap *bm, int fd, options *options)
{
    struct facets *facets;
    struct idxlist *idxlist;
    unsigned int ploop;
    unsigned int tloop;
    FILE *outf;

    outf = fdopen(dup(fd), "w");

    facets = gen_facets(bm, options);
    if (facets == NULL) {
        fprintf(stderr,"unable to generate triangle mesh\n");
        return false;
    }

    idxlist = gen_idxlist(facets);

    /* fprintf(stderr, "cubes %d facets %d vertexes %u\n", facets->cubes, facets->count, idxlist->pcount); */

    fprintf(outf, "polyhedron(points = [\n");

    for (ploop = 0; ploop < idxlist->pcount; ploop++) {
        struct pnt *pnt;
        pnt = *(idxlist->p + ploop);
        fprintf(outf, "[%f,%f,%f],\n", pnt->x, pnt->y, pnt->z);
    }

    fprintf(outf, "], triangles = [\n");

    for (tloop = 0; tloop < idxlist->tcount; tloop++) {
        fprintf(outf, "[%u,%u,%u],\n", idxlist->t[tloop].v[0], idxlist->t[tloop].v[1], idxlist->t[tloop].v[2] );
    }


    fprintf(outf, "]);");

    free_facets(facets);

    fclose(outf);

    return true;
}
