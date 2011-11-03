/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * Routines to handle meshes
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
#include "mesh_gen.h"
#include "mesh_index.h"
#include "mesh_simplify.h"
#include "mesh_math.h"

void
debug_mesh_init(struct mesh *mesh, const char* filename)
{
    if (filename == NULL)
        return;

    mesh->dumpfile = fopen(filename, "w");

    if (mesh->dumpfile == NULL)
        return;

    fprintf(mesh->dumpfile,"<html>\n<body>");

}

static void
debug_mesh_fini(struct mesh *mesh, unsigned int start)
{
    unsigned int floop;
    struct vertex *v0 = vertex_from_index(mesh, start);

    if (mesh->dumpfile == NULL)
        return;

    fprintf(mesh->dumpfile, "<h2>Final mesh</h2>");

    fprintf(mesh->dumpfile,
            "<p>Final mesh had %d facets and %d vertexes.</p>\n",
            mesh->fcount, mesh->vcount);

    fprintf(mesh->dumpfile,"<p>Mesh of all facets with common normal</p>\n<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n", DUMP_SVG_SIZE, DUMP_SVG_SIZE);

    for (floop = 0; floop < mesh->fcount; floop++) {
        if (same_normal(&mesh->f[floop].n, &v0->facets[0]->n)) {

            fprintf(mesh->dumpfile,
                    "<polygon points=\"%.1f,%.1f %.1f,%.1f %.1f,%.1f\" style=\"fill:lime;stroke:black;stroke-width=1\"/>\n",
                    SVGPX(mesh->f[floop].v[0].x), SVGPY(mesh->f[floop].v[0].y),
                    SVGPX(mesh->f[floop].v[1].x), SVGPY(mesh->f[floop].v[1].y),
                    SVGPX(mesh->f[floop].v[2].x), SVGPY(mesh->f[floop].v[2].y));

            /* label facet at centroid */
            fprintf(mesh->dumpfile,
                    "<text x=\"%.1f\" y=\"%.1f\" fill=\"blue\">%u</text>\n",
                    (SVGPX(mesh->f[floop].v[0].x) +
                     SVGPX(mesh->f[floop].v[1].x) +
                     SVGPX(mesh->f[floop].v[2].x)) / 3,
                    (SVGPY(mesh->f[floop].v[0].y) +
                     SVGPY(mesh->f[floop].v[1].y) +
                     SVGPY(mesh->f[floop].v[2].y)) / 3,
                    floop);


        }
    }

    fprintf(mesh->dumpfile,"</body>\n</html>\n");

    fclose(mesh->dumpfile);

    mesh->dumpfile = NULL;
}



/* exported method documented in mesh.h */
struct mesh *new_mesh(void)
{
    struct mesh *mesh;

    mesh = calloc(1, sizeof(struct mesh));

    return mesh;
}



/* exported method documented in mesh.h */
void free_mesh(struct mesh *mesh)
{
    debug_mesh_fini(mesh, 4);
    free(mesh->bloom_table);
    free(mesh->f);
}



