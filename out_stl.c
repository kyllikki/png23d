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
#include "mesh_gen.h"
#include "mesh_index.h"
#include "mesh_simplify.h"
#include "out_stl.h"


static struct mesh *stl_mesh(bitmap *bm, int fd, options *options)
{
    struct mesh *mesh;

    mesh = new_mesh();
    if (mesh == NULL) {
        fprintf(stderr,"unable to create mesh\n");
        return NULL;
    }

    debug_mesh_init(mesh, options->meshdebug);

    if (mesh_from_bitmap(mesh, bm, options) == false) {
        fprintf(stderr,"unable to convert bitmap to mesh with requested finish\n");
        free_mesh(mesh);
        return NULL;
    }

    if (options->optimise > 0) {
        uint32_t start_vcount = mesh->fcount * 3; /* each facet has 3 vertex */

        INFO("Indexing %d vertices\n", start_vcount);
        index_mesh(mesh, options->bloom_complexity, options->vertex_complexity);

        INFO("Simplification of mesh with %d facets using %d unique verticies\n",
             mesh->fcount, start_vcount);

        simplify_mesh(mesh);

        INFO("Bloom filter prevented %d (%d%%) lookups\n",
             start_vcount - mesh->find_count,
             ((start_vcount - mesh->find_count) * 100) / start_vcount);

        INFO("Bloom filter had %d (%d%%) false positives\n",
             mesh->bloom_miss,
             (mesh->bloom_miss * 100) / (mesh->find_count));

        INFO("Indexing required %d lookups with mean search cost " D64F " comparisons\n",
             mesh->find_count,
             mesh->find_cost / mesh->find_count);

        INFO("Result mesh has %d facets using %d unique verticies\n",
             mesh->fcount, mesh->vcount);
    }

    INFO("width bitmap:%d output:%f\n",bm->width, options->width);
    INFO("width scale is 1:%f\n", options->width / bm->width);

    INFO("height bitmap:%d output:%f\n",options->levels, options->depth);
    INFO("height scale is 1:%f\n", options->depth / options->levels);

    return mesh;
}


/* binary stl output
 *
 * UINT8[80] – Header
 * UINT32 – Number of triangles
 *
 * foreach triangle
 * REAL32[3] – Normal vector
 * REAL32[3] – Vertex 1
 * REAL32[3] – Vertex 2
 * REAL32[3] – Vertex 3
 * UINT16 – Attribute byte count
 * end
 *
 */
bool output_flat_stl(bitmap *bm, int fd, options *options)
{
    struct mesh *mesh;
    unsigned int floop;
    uint8_t header[80];
    bool ret = true;
    struct binstltri {
            pnt n; /**< surface normal */
            pnt v[3]; /**< triangle vertices */
            uint16_t attribute;
    } __attribute__((packed)) binstltri;
    float xscale = options->width / bm->width;
    float zscale = options->depth / options->levels;

    assert(sizeof(struct binstltri) == 50); /* this is foul and nasty */

    mesh = stl_mesh(bm, fd, options);
    if (mesh == NULL) {
        return false;
    }

    INFO("Writing Binary STL output\n");

    /* write file header */
    memset(header, 0, 80);
    snprintf((char *)header, 80,
             "Binary STL generated by png23d from %s", options->infile);
    if (write(fd, header, 80) != 80) {
        ret = false;
        goto output_flat_stl_error;
    }

    /* write number of triangles in file */
    if (write(fd, &mesh->fcount, sizeof(uint32_t)) != sizeof(uint32_t)) {
        ret = false;
        goto output_flat_stl_error;
    }

    binstltri.attribute = 0;

    /* write each triangle after scaling */
    for (floop=0; floop < mesh->fcount; floop++) {
        /* copy vertex points with scaling */
        binstltri.n = mesh->f[floop].n;
        binstltri.v[0].x = mesh->f[floop].v[0].x * xscale;
        binstltri.v[0].y = mesh->f[floop].v[0].y * xscale;
        binstltri.v[0].z = mesh->f[floop].v[0].z * zscale;
        binstltri.v[1].x = mesh->f[floop].v[1].x * xscale;
        binstltri.v[1].y = mesh->f[floop].v[1].y * xscale;
        binstltri.v[1].z = mesh->f[floop].v[1].z * zscale;
        binstltri.v[2].x = mesh->f[floop].v[2].x * xscale;
        binstltri.v[2].y = mesh->f[floop].v[2].y * xscale;
        binstltri.v[2].z = mesh->f[floop].v[2].z * zscale;

        if (write(fd, &binstltri, sizeof(struct binstltri)) != sizeof(struct binstltri)) {
            ret = false;
            break;
        }
    }

output_flat_stl_error:
    free_mesh(mesh);

    return ret;
}

static inline void output_stl_tri(FILE *outf, const struct facet *facet, float xscale, float zscale)
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
            facet->v[0].x * xscale, facet->v[0].y * xscale, facet->v[0].z * zscale,
            facet->v[1].x * xscale, facet->v[1].y * xscale, facet->v[1].z * zscale,
            facet->v[2].x * xscale, facet->v[2].y * xscale, facet->v[2].z * zscale);
}

/* ascii stl outout */
bool output_flat_astl(bitmap *bm, int fd, options *options)
{
    struct mesh *mesh;
    unsigned int floop;
    FILE *outf;

    mesh = stl_mesh(bm, fd, options);
    if (mesh == NULL) {
        return false;
    }

    INFO("Writing ASCII STL output\n");
    outf = fdopen(dup(fd), "w");

    fprintf(outf, "solid png2stl_Model\n");

    for (floop = 0; floop < mesh->fcount; floop++) {
        output_stl_tri(outf,
                       mesh->f + floop,
                       options->width / bm->width,
                       options->depth / options->levels );
    }

    fprintf(outf, "endsolid png2stl_Model\n");

    free_mesh(mesh);

    fclose(outf);

    return true;
}
