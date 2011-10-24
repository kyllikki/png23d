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
    struct mesh *mesh;
    unsigned int ploop;
    unsigned int tloop; /* triangle loop */
    FILE *outf;

    outf = fdopen(dup(fd), "w");

    mesh = new_mesh();
    if (mesh == NULL) {
        fprintf(stderr,"unable to create mesh\n");
        return false;
    }

    debug_mesh_init(mesh, options->meshdebug);

    if (mesh_from_bitmap(mesh, bm, options) == false) {
        fprintf(stderr,"unable to convert bitmap to mesh\n");
        return false;
    }

    if (options->optimise > 0) {
        simplify_mesh(mesh);
    } else {
        index_mesh(mesh);
    }

    fprintf(outf, "polyhedron(points = [\n");

    for (ploop = 0; ploop < mesh->pcount; ploop++) {
        fprintf(outf, "[%f,%f,%f],\n", 
                mesh->p[ploop].pnt.x, 
                mesh->p[ploop].pnt.y, 
                mesh->p[ploop].pnt.z);
    }

    fprintf(outf, "], triangles = [\n");

    for (tloop = 0; tloop < mesh->fcount; tloop++) {
        fprintf(outf, "[%u,%u,%u],\n",
                mesh->f[tloop].i[0],
                mesh->f[tloop].i[1],
                mesh->f[tloop].i[2] );
    }


    fprintf(outf, "]);");

    free_mesh(mesh);

    fclose(outf);

    return true;
}
