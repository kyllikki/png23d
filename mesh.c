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

enum faces {
    FACE_LEFT = 1,
    FACE_RIGHT = 2,
    FACE_TOP = 4,
    FACE_BOT = 8,
    FACE_FRONT = 16,
    FACE_BACK = 32,
};

static void add_facet(struct facets *facets,
                      float vx0,float vy0, float vz0,
                      float vx1,float vy1, float vz1,
                      float vx2,float vy2, float vz2)
{
    struct facet *newfacet;
    pnt a;
    pnt b;

    if ((facets->fcount + 1) > facets->falloc) {
        /* array needs extending */
        facets->f = realloc(facets->f, (facets->falloc + 1000) * sizeof(struct facet));
        facets->falloc += 1000;
    }

    newfacet = facets->f + facets->fcount;
    facets->fcount++;

    /* normal calculation
     * va = v1 - v0
     * vb = v2 - v0
     *
     * n = va x vb (cross product)
     */
    a.x = vx1 - vx0;
    a.y = vy1 - vy0;
    a.z = vz1 - vz0;

    b.x = vx2 - vx0;
    b.y = vy2 - vy0;
    b.z = vz2 - vz0;

    newfacet->n.x = a.y * b.z - a.z * b.y;
    newfacet->n.y = a.z * b.x - a.x * b.z;
    newfacet->n.z = a.x * b.y - a.y * b.x;

    newfacet->v[0].x = vx0;
    newfacet->v[0].y = vy0;
    newfacet->v[0].z = vz0;

    newfacet->v[1].x = vx1;
    newfacet->v[1].y = vy1;
    newfacet->v[1].z = vz1;

    newfacet->v[2].x = vx2;
    newfacet->v[2].y = vy2;
    newfacet->v[2].z = vz2;
}

#define ADDF(xa,ya,za,xb,yb,zb,xc,yc,zc) add_facet(facets, \
    x + (xa * width), y + (ya * height), z + (za * depth), \
    x + (xb * width), y + (yb * height), z + (zb * depth), \
    x + (xc * width), y + (yc * height), z + (zc * depth))


/* generates cube facets for a location */
static void
output_cube(struct facets *facets,
                float x, float y, float z,
                float width, float height, float depth,
                uint32_t faces)
{
    if (faces != 0) {
        facets->cubes++;
    }

    switch (faces & 0xf) {
    case 0: /* empty */
        break;

    case FACE_LEFT:
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        break;

    case FACE_RIGHT:
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_LEFT | FACE_RIGHT) :
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case FACE_TOP:
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        break;

    case (FACE_TOP | FACE_LEFT):
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        break;

    case (FACE_TOP | FACE_RIGHT):
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_TOP | FACE_LEFT | FACE_RIGHT):
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case FACE_BOT:
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        break;

    case (FACE_BOT | FACE_LEFT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        break;

    case (FACE_BOT | FACE_RIGHT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_BOT | FACE_LEFT | FACE_RIGHT) :
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_BOT | FACE_TOP):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        break;

    case (FACE_BOT | FACE_TOP | FACE_LEFT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        break;

    case (FACE_BOT | FACE_TOP | FACE_RIGHT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_BOT | FACE_TOP | FACE_LEFT | FACE_RIGHT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    }

    if ((faces & FACE_FRONT) != 0) {
        ADDF(0,0,0, 0,1,0, 1,0,0);
        ADDF(0,1,0, 1,1,0, 1,0,0);
    }

    if ((faces & FACE_BACK) != 0) {
        ADDF(0,0,1, 1,0,1, 0,1,1);
        ADDF(0,1,1, 1,0,1, 1,1,1);
    }

}

/* generates smoothed facets for a location
 *
 * appies marching squares (ish) to generate facets
 *
 * @todo make this table driven as a 64 entry switch is out of hand
 */
static void
output_marching_squares(struct facets *facets,
                float x, float y, float z,
                float width, float height, float depth,
                uint32_t faces)
{
    if (faces != 0) {
        facets->cubes++;
    }

    switch (faces & 0xf) {
    case 0: /* empty */
        break;

    case FACE_LEFT:
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        break;

    case FACE_RIGHT:
        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_LEFT | FACE_RIGHT) :
        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);

        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case FACE_TOP:
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        break;

    case (FACE_TOP | FACE_LEFT):
        ADDF(0,0,0, 0,0,1, 1,1,0);
        ADDF(1,1,0, 0,0,1, 1,1,1);

        if ((faces & FACE_FRONT) != 0) {
            ADDF(0,0,0, 1,1,0, 1,0,0);
            faces = faces & ~FACE_FRONT;
        }

        if ((faces & FACE_BACK) != 0) {
            ADDF(0,0,1, 1,0,1, 1,1,1);
            faces = faces & ~FACE_BACK;
        }

        break;

    case (FACE_TOP | FACE_RIGHT):
        ADDF(1,0,0, 0,1,0, 1,0,1);
        ADDF(0,1,0, 0,1,1, 1,0,1);

        if ((faces & FACE_FRONT) != 0) {
            ADDF(1,0,0, 0,0,0, 0,1,0);
            faces = faces & ~FACE_FRONT;
        }

        if ((faces & FACE_BACK) != 0) {
            ADDF(1,0,1, 0,1,1, 0,0,1);
            faces = faces & ~FACE_BACK;
        }
        break;

    case (FACE_TOP | FACE_LEFT | FACE_RIGHT):
        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);

        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);

        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case FACE_BOT:
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);
        break;

    case (FACE_BOT | FACE_LEFT):
        ADDF(0,1,0, 1,0,0, 1,0,1);
        ADDF(0,1,0, 1,0,1, 0,1,1);

        if ((faces & FACE_FRONT) != 0) {
            ADDF(1,0,0, 0,1,0, 1,1,0);
            faces = faces & ~FACE_FRONT;
        }

        if ((faces & FACE_BACK) != 0) {
            ADDF(1,0,1, 1,1,1, 0,1,1);
            faces = faces & ~FACE_BACK;
        }

        break;

    case (FACE_BOT | FACE_RIGHT):
        ADDF(0,0,0, 1,1,0, 0,0,1);
        ADDF(0,0,1, 1,1,0, 1,1,1);

        if ((faces & FACE_FRONT) != 0) {
            ADDF(0,0,0, 0,1,0, 1,1,0);
            faces = faces & ~FACE_FRONT;
        }

        if ((faces & FACE_BACK) != 0) {
            ADDF(0,0,1, 1,1,1, 0,1,1);
            faces = faces & ~FACE_BACK;
        }

        break;

    case (FACE_BOT | FACE_LEFT | FACE_RIGHT) :
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);

        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);

        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_BOT | FACE_TOP):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);

        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);
        break;

    case (FACE_BOT | FACE_TOP | FACE_LEFT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);

        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);

        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);
        break;

    case (FACE_BOT | FACE_TOP | FACE_RIGHT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);

        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);

        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    case (FACE_BOT | FACE_TOP | FACE_LEFT | FACE_RIGHT):
        ADDF(0,0,0, 1,0,0, 0,0,1);
        ADDF(0,0,1, 1,0,0, 1,0,1);

        ADDF(0,1,0, 0,1,1, 1,1,0);
        ADDF(0,1,1, 1,1,1, 1,1,0);

        ADDF(0,0,0, 0,0,1, 0,1,0);
        ADDF(0,1,0, 0,0,1, 0,1,1);

        ADDF(1,0,0, 1,1,0, 1,0,1);
        ADDF(1,1,0, 1,1,1, 1,0,1);
        break;

    }

    if ((faces & FACE_FRONT) != 0) {
        ADDF(0,0,0, 0,1,0, 1,0,0);
        ADDF(0,1,0, 1,1,0, 1,0,0);
    }

    if ((faces & FACE_BACK) != 0) {
        ADDF(0,0,1, 1,0,1, 0,1,1);
        ADDF(0,1,1, 1,0,1, 1,1,1);
    }

}

static uint32_t
get_face(bitmap *bm, unsigned int x, unsigned int y, uint8_t transparent)
{
    uint32_t faces = 0;
    if (bm->data[(y * bm->width) + x] < transparent) {
        /* only opaque squares have faces */
        faces = FACE_TOP | FACE_BOT |
                FACE_FRONT | FACE_BACK |
                FACE_LEFT | FACE_RIGHT;

        if ((x > 0) && (bm->data[(y * bm->width) + (x - 1)] < transparent)) {
            faces = faces & ~FACE_LEFT;
        }

        if ((x < (bm->width - 1)) &&
            (bm->data[(y * bm->width) + (x + 1)] < transparent)) {
            faces = faces & ~FACE_RIGHT;
        }

        if ((y > 0) &&
            (bm->data[((y - 1) * bm->width) + x ] < transparent)) {
            faces = faces & ~FACE_TOP;
        }

        if ((y < (bm->height - 1)) &&
            (bm->data[((y + 1) * bm->width) + x ] < transparent)) {
            faces = faces & ~FACE_BOT;
        }


    }
    return faces;
}

/** convert raster image into facets
 *
 * consider each pixel in the raster image:
 *   - generate a bitfield indicating on which sides of the pixel faces need to
 *     be covered to generate a convex manifold.
 *   - add triangle facets to list for each face present
 *
 * @todo This could probably be better converted to a marching cubes solution
 *   or as this is a simple 2d extrusion perhaps modified marching squares
 *   http://en.wikipedia.org/wiki/Marching_cubes
 */
struct facets *
gen_facets(bitmap *bm, options *options)
{
    unsigned int row_loop;
    unsigned int col_loop;
    float scale = options->width / bm->width;

    float xoff; /* x offset so 3d model is centered */
    float yoff; /* y offset so 3d model is centered */

    uint32_t faces;

    struct facets *facets;

    facets = calloc(1, sizeof(struct facets));
    if (facets == NULL) {
        return facets;
    }

    xoff = (options->width / 2);
    yoff = ((bm->height * scale) / 2);

    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            faces = get_face(bm, col_loop, row_loop, options->transparent);

            if (options->finish == FINISH_RAW) {
                output_cube(facets,
                                (col_loop * scale) - xoff,
                                yoff - (row_loop * scale),
                                0,
                                scale, scale, options->depth,
                                faces);
            } else {
                output_marching_squares(facets,
                                            (col_loop * scale) - xoff,
                                            yoff - (row_loop * scale),
                                            0,
                                            scale, scale, options->depth,
                                            faces);
            }
        }
    }

    return facets;
}

void free_facets(struct facets *facets)
{
    free(facets->f);
}

static uint32_t find_pnt(struct facets *facets, struct pnt *pnt)
{
    uint32_t idx;
    for (idx = 0; idx < facets->pcount; idx++) {
        if (((*(facets->p + idx))->x == pnt->x) &&
            ((*(facets->p + idx))->y == pnt->y) &&
            ((*(facets->p + idx))->z == pnt->z)) {
            break;
        }

    }
    return idx;
}

static idxpnt add_pnt(struct facets *facets, struct pnt *npnt)
{
    uint32_t idx;

    idx = find_pnt(facets, npnt);
    if (idx == facets->pcount) {
        /* not in array already */
        if ((facets->pcount + 1) > facets->palloc) {
            /* pnt array needs extending */
            facets->p = realloc(facets->p,
                                 (facets->palloc + 1000) * sizeof(struct pnt *));
            facets->palloc += 1000;
        }

        *(facets->p + facets->pcount) = npnt;
        facets->pcount++;
    }
    return idx;
}

bool
update_indexing(struct facets *facets)
{
    unsigned int floop;

    /* manufacture pointlist and update indexed geometry */
    for (floop = 0; floop < facets->fcount; floop++) {
        /* update facet with indexed points */
        facets->f[floop].i[0] = add_pnt(facets, &facets->f[floop].v[0]);
        facets->f[floop].i[1] = add_pnt(facets, &facets->f[floop].v[1]);
        facets->f[floop].i[2] = add_pnt(facets, &facets->f[floop].v[2]);
    }

    return true;;
}
