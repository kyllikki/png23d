/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * Routines to output in STL format
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

static inline void output_stl_tri(FILE *outf, const struct facet *facet)
{
    fprintf(outf,
            "  facet normal %.6f %.6f %.6f\n"
            "    outer loop\n"
            "      vertex %.6f %.6f %.6f\n"
            "      vertex %.6f %.6f %.6f\n"
            "      vertex %.6f %.6f %.6f\n"
            "    endloop\n"
            "  endfacet\n",
            facet->n.x, facet->n.y, facet->n.z,
            facet->v[0].x, facet->v[0].y, facet->v[0].z,
            facet->v[1].x, facet->v[1].y, facet->v[1].z,
            facet->v[2].x, facet->v[2].y, facet->v[2].z);
}

/* ascii stl outout */
bool output_flat_scad_polyhedron(bitmap *bm, int fd, options *options)
{
    struct facets *facets;
    unsigned int floop;
    FILE *outf;

    outf = fdopen(dup(fd), "w");

    facets = gen_facets(bm, options);
    if (facets == NULL) {
        fprintf(stderr,"unable to generate triangle mesh\n");
        return false;
    }

    fprintf(stderr, "cubes %d facets %d\n", facets->cubes, facets->count);

    fprintf(outf, "solid png2stl_Model\n");

    for (floop = 0; floop < facets->count; floop++) {
        output_stl_tri(outf, *(facets->v + floop));
    }

    fprintf(outf, "endsolid png2stl_Model\n");

    free_facets(facets);

    fclose(outf);

    return true;
}
