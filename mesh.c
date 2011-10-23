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

static void add_facet(struct mesh *mesh,
                      float vx0,float vy0, float vz0,
                      float vx1,float vy1, float vz1,
                      float vx2,float vy2, float vz2)
{
    struct facet *newfacet;
    pnt a;
    pnt b;

    if ((mesh->fcount + 1) > mesh->falloc) {
        /* array needs extending */
        mesh->f = realloc(mesh->f, (mesh->falloc + 1000) * sizeof(struct facet));
        mesh->falloc += 1000;
    }

    newfacet = mesh->f + mesh->fcount;
    mesh->fcount++;

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

#define ADDF(xa,ya,za,xb,yb,zb,xc,yc,zc) add_facet(mesh, \
    x + (xa * width), y + (ya * height), z + (za * depth), \
    x + (xb * width), y + (yb * height), z + (zb * depth), \
    x + (xc * width), y + (yc * height), z + (zc * depth))


/* generates cube facets for a location */
static void
output_cube(struct mesh *mesh,
                float x, float y, float z,
                float width, float height, float depth,
                uint32_t faces)
{
    if (faces != 0) {
        mesh->cubes++;
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
output_marching_squares(struct mesh *mesh,
                float x, float y, float z,
                float width, float height, float depth,
                uint32_t faces)
{
    if (faces != 0) {
        mesh->cubes++;
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

/* exported method docuemnted in mesh.h */
struct mesh *
generate_mesh(bitmap *bm, options *options)
{
    unsigned int row_loop;
    unsigned int col_loop;
    float scale = options->width / bm->width;

    float xoff; /* x offset so 3d model is centered */
    float yoff; /* y offset so 3d model is centered */

    uint32_t faces;

    struct mesh *mesh;

    mesh = calloc(1, sizeof(struct mesh));
    if (mesh == NULL) {
        return mesh;
    }

    xoff = (options->width / 2);
    yoff = ((bm->height * scale) / 2);

    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            faces = get_face(bm, col_loop, row_loop, options->transparent);

            if (options->finish == FINISH_RAW) {
                output_cube(mesh,
                                (col_loop * scale) - xoff,
                                yoff - (row_loop * scale),
                                0,
                                scale, scale, options->depth,
                                faces);
            } else {
                output_marching_squares(mesh,
                                            (col_loop * scale) - xoff,
                                            yoff - (row_loop * scale),
                                            0,
                                            scale, scale, options->depth,
                                            faces);
            }
        }
    }

    return mesh;
}

void free_mesh(struct mesh *mesh)
{
    free(mesh->f);
}

static uint32_t find_pnt(struct mesh *mesh, struct pnt *pnt)
{
    uint32_t idx;
    for (idx = 0; idx < mesh->pcount; idx++) {
        if ((mesh->p[idx].pnt.x == pnt->x) &&
            (mesh->p[idx].pnt.y == pnt->y) &&
            (mesh->p[idx].pnt.z == pnt->z)) {
            break;
        }

    }
    return idx;
}

static idxpnt 
add_pnt(struct mesh *mesh, struct pnt *npnt)
{
    uint32_t idx;

    idx = find_pnt(mesh, npnt);
    if (idx == mesh->pcount) {
        /* not in array already */
        if ((mesh->pcount + 1) > mesh->palloc) {
            /* pnt array needs extending */
            mesh->p = realloc(mesh->p,
                                (mesh->palloc + 1000) *
                                sizeof(struct vertex));
            mesh->palloc += 1000;
        }

        mesh->p[mesh->pcount].pnt = *npnt;
        mesh->p[mesh->pcount].fcount = 0;
        mesh->pcount++;
    }
    return idx;
}

bool
index_mesh(struct mesh *mesh)
{
    unsigned int floop;

    /* manufacture pointlist and update indexed geometry */
    for (floop = 0; floop < mesh->fcount; floop++) {
        idxpnt p0, p1, p2;
        /* update facet with indexed points */
        p0 = mesh->f[floop].i[0] = add_pnt(mesh, &mesh->f[floop].v[0]);
        p1 = mesh->f[floop].i[1] = add_pnt(mesh, &mesh->f[floop].v[1]);
        p2 = mesh->f[floop].i[2] = add_pnt(mesh, &mesh->f[floop].v[2]);

        mesh->p[p0].facets[mesh->p[p0].fcount++] = &mesh->f[floop];
        mesh->p[p1].facets[mesh->p[p1].fcount++] = &mesh->f[floop];
        mesh->p[p2].facets[mesh->p[p2].fcount++] = &mesh->f[floop];
    }

    return true;;
}

/* are points the same value */
static inline bool eqpnt(struct pnt *p0, struct pnt *p1)
{
    if ((p0->x == p1->x) && (p0->y == p1->y) && (p0->z == p1->z)) {
        return true;
    } else {
        return false;
    }
}

/* do points differ */
static inline bool nepnt(struct pnt *p0, struct pnt *p1)
{
    if ((p0->x != p1->x) || (p0->y != p1->y) || (p0->z != p1->z)) {
        return true;
    } else {
        return false;
    }
}

/* determinae if a vertex is topoligcally a removal candidate  */
static bool is_candidate(struct vertex *vtx)
{
    unsigned int floop; /* facet loop */

    /* check edge end normals are all the same */
    for (floop = 1; floop < vtx->fcount; floop++) {
        if (nepnt(&vtx->facets[floop - 1]->n, &vtx->facets[floop]->n))
            return false;
    }

    return true;
}

/** check each adjacent vertex for suitability to be removed.
 */
static bool
check_adjacent(struct mesh *mesh, unsigned int ivtx, unsigned int *avtx)
{
    unsigned int floop; /* facet loop */
    unsigned int vloop; /* vertex within facets */
    struct vertex *vtx = mesh->p + ivtx; /* initial vertex */
    unsigned int civtx; /* candidate vertex index */

    for (floop = 0; floop < vtx->fcount; floop++) {
        for (vloop = 0; vloop < 3; vloop++) {
            civtx = vtx->facets[floop]->i[vloop];

            if (civtx == ivtx) {
                continue; /* skip given vertex */
            }

            if (is_candidate(mesh->p + civtx)) {
                /* cannot merge edge verticies if it makes the mesh too
                 * complicated to represent
                 */
                if (((vtx->fcount + mesh->p[civtx].fcount) - 2) <= FACETPNT_CNT) {
                    *avtx = civtx;
                    return true;
                }
            }

        }
    }
    return false; /* no match */
}

static bool 
remove_facet_from_vertex(struct facet *facet, struct vertex *vertex)
{
    unsigned int floop;

    for (floop = 0; floop < vertex->fcount; floop++) {
        if (vertex->facets[floop] == facet) {
            vertex->fcount--;
            for (; floop < vertex->fcount; floop++) {
                vertex->facets[floop] = vertex->facets[floop + 1];
            }
            return true;
        }
    }
    fprintf(stderr,"failed to remove facet from vertex\n");

    return false;
}

/* move a vertex of a facet 
*
* changes one vertex of a factet to a new facet and updates the destination
* vertex to reference this facet. 
* 
* @return false if from vertex was not in facet else true
 */
static bool 
move_facet_vertex(struct mesh *mesh, 
                  struct facet *facet,
                  unsigned int from, 
                  unsigned int to)
{
    if (facet->i[0] == from) {
        facet->i[0] = to;
        facet->v[0] = mesh->p[to].pnt;
    } else if (facet->i[1] == from) {
        facet->i[1] = to;
        facet->v[1] = mesh->p[to].pnt;
    } else if (facet->i[2] == from) {
        facet->i[2] = to;
        facet->v[2] = mesh->p[to].pnt;
    } else {
        return false;
    }
    /* add ourselves to destination vertex */
    mesh->p[to].facets[mesh->p[to].fcount++] = facet;

    /* remove from old one */
    remove_facet_from_vertex(facet, mesh->p + from);

    return true;
}

static bool facet_on_vertex(struct facet *facet, struct vertex *vertex)
{
    unsigned int floop;

    for (floop = 0; floop < vertex->fcount; floop++) {
        if (vertex->facets[floop] == facet) 
            return true;
    }

    return false;
}


static bool remove_facet(struct mesh *mesh, struct facet *facet)
{
    struct facet *rfacet;
    /* remove facet from all three vertecies */
    remove_facet_from_vertex(facet, mesh->p + facet->i[0]);
    remove_facet_from_vertex(facet, mesh->p + facet->i[1]);
    remove_facet_from_vertex(facet, mesh->p + facet->i[2]);

    /* only way to efficiently remove a facet is to move the one at the end of
     * the list here instead 
     */
    mesh->fcount--;
    rfacet = mesh->f + mesh->fcount;
    if (rfacet != facet) {
        /* was not already the end entry, have to do some work */
        memcpy(facet, rfacet, sizeof(struct facet));
        /* fix up vertex pointers */
        remove_facet_from_vertex(rfacet, mesh->p + facet->i[0]);
        remove_facet_from_vertex(rfacet, mesh->p + facet->i[1]);
        remove_facet_from_vertex(rfacet, mesh->p + facet->i[2]);

        mesh->p[facet->i[0]].facets[mesh->p[facet->i[0]].fcount++] = facet;
        mesh->p[facet->i[1]].facets[mesh->p[facet->i[1]].fcount++] = facet;
        mesh->p[facet->i[2]].facets[mesh->p[facet->i[2]].fcount++] = facet;

    }
    return true;
}
#if 1
#if 0
static void dump_mesh(struct mesh *mesh, struct vertex *v0, struct vertex *v1)
{
    unsigned int floop;
    unsigned int vloop;

    printf("--MESH-DUMP--\nINF mesh has %d facets and %d vertex\n", 
           mesh->fcount, mesh->pcount);

    for (vloop=0; vloop < mesh->pcount; vloop++) {
        printf("VTX %2d: (%.1f, %.1f, %.1f) ", vloop, 
               mesh->p[vloop].pnt.x, 
               mesh->p[vloop].pnt.y, 
               mesh->p[vloop].pnt.z);
        for (floop=0; floop < mesh->p[vloop].fcount; floop++) {
            printf("[%2u,%2u,%2u] ", 
                   mesh->p[vloop].facets[floop]->i[0],
                   mesh->p[vloop].facets[floop]->i[1],
                   mesh->p[vloop].facets[floop]->i[2]);
        }
        printf("\n");
    }

    for (floop=0; floop < mesh->fcount; floop++) {
        printf("FCT %2d: [%2u,%2u,%2u] (%.1f, %.1f, %.1f) / (%.1f, %.1f, %.1f) / (%.1f, %.1f, %.1f)\n", 
               floop, 
               mesh->f[floop].i[0], mesh->f[floop].i[1], mesh->f[floop].i[2],
               mesh->f[floop].v[0].x, mesh->f[floop].v[0].y, mesh->f[floop].v[0].z, 
               mesh->f[floop].v[1].x, mesh->f[floop].v[1].y, mesh->f[floop].v[1].z, 
               mesh->f[floop].v[2].x, mesh->f[floop].v[2].y, mesh->f[floop].v[2].z 
               );
    }
    printf("\n");
}
#else

static int dumpno=0;
static FILE *dumpfile;

#define SVGP(loc) (20 + (loc) ) * 10

static void dump_mesh_init(struct mesh *mesh)
{
    dumpfile = fopen("index.html", "w");

    fprintf(dumpfile,"<html>\n<body><table><tr>\n");

}

static void 
dump_mesh(struct mesh *mesh, bool removing, unsigned int start, unsigned int end)
{
    unsigned int floop;
    struct vertex *v0 = mesh->p + start;
    struct vertex *v1 = NULL;

    if (removing) {
        v1 = mesh->p + end;
        fprintf(dumpfile, "<tr><th>Operation %d Removing %u->%u</th>", dumpno, start, end);
        dumpno++;
    }

    fprintf(dumpfile, "<td><svg width=\"500\" height=\"500\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n");

    for (floop = 0; floop < mesh->fcount; floop++) {
        if (eqpnt(&mesh->f[floop].n, &v0->facets[0]->n)) {

            fprintf(dumpfile,
                    "<polygon points=\"%.1f,%.1f %.1f,%.1f %.1f,%.1f\" style=\"fill:lime;stroke:black;stroke-width=1\"/>\n", 
                    SVGP(mesh->f[floop].v[0].x), SVGP(mesh->f[floop].v[0].y),
                    SVGP(mesh->f[floop].v[1].x), SVGP(mesh->f[floop].v[1].y),
                    SVGP(mesh->f[floop].v[2].x), SVGP(mesh->f[floop].v[2].y));

            /* label facet at centroid */
            fprintf(dumpfile,
                    "<text x=\"%.1f\" y=\"%.1f\" fill=\"blue\">%u</text>\n",
                    (SVGP(mesh->f[floop].v[0].x) + 
                     SVGP(mesh->f[floop].v[1].x) + 
                     SVGP(mesh->f[floop].v[2].x)) / 3,
                    (SVGP(mesh->f[floop].v[0].y) + 
                     SVGP(mesh->f[floop].v[1].y) + 
                     SVGP(mesh->f[floop].v[2].y)) / 3,
                    floop);


        }
    }

    if (v1 != NULL) {
        fprintf(dumpfile,
                "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" style=\"stroke:red;stroke-width:5\"/>\n",
                SVGP(v0->pnt.x), SVGP(v0->pnt.y), 
                SVGP(v1->pnt.x), SVGP(v1->pnt.y));

            fprintf(dumpfile,
                    "<text x=\"%.1f\" y=\"%.1f\" fill=\"black\">%u</text>\n", 
                    SVGP(v1->pnt.x) + 5, SVGP(v1->pnt.y) + 5,  end);

    } 

    fprintf(dumpfile,
            "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"10\" fill=\"blue\"/>\n",
            SVGP(v0->pnt.x), SVGP(v0->pnt.y));

    fprintf(dumpfile,
            "<text x=\"%.1f\" y=\"%.1f\" fill=\"black\">%u</text>\n", 
            SVGP(v0->pnt.x) + 10, SVGP(v0->pnt.y) + 5,  start);


    fprintf(dumpfile, "</svg></td>");

    if (!removing) {
        fprintf(dumpfile, "</tr>");
    }
}

static void dump_mesh_fini(struct mesh *mesh)
{
    fprintf(dumpfile,"</table></body>\n</html>\n");

    fclose(dumpfile);

}

#endif
#else
static void dump_mesh(struct mesh *mesh, struct vertex *v0, struct vertex *v1)
{
}
static void dump_mesh_init(struct mesh *mesh)
{
}
static void dump_mesh_fini(struct mesh *mesh)
{
}
#endif

/* merge an edge by moving all facets from end of edge to start */
static bool
merge_edge(struct mesh *mesh, unsigned int start, unsigned int end)
{
    struct facet *facet;

    //    printf("remove edge %u,%u\n\n", start, end);

    dump_mesh(mesh, true, start, end);

    /* change all the facets on second vertex (ivtx1) to point at first virtex
     * instead 
     *
     * delete degenerate facets(two of their vertecies will be the same
     */
    while (mesh->p[end].fcount > 0) {
        facet = mesh->p[end].facets[0];
        if (facet_on_vertex(facet, mesh->p + start)) {
            remove_facet(mesh, facet); /* remove degenerate facet */
        } else {
            move_facet_vertex(mesh, facet, end, start);
        }        
    }

    dump_mesh(mesh, false, start, end);

    return false;
}

/* simplify mesh by edge removal
*
* algorithm is:
* find vertex where all facets have the same normal
* search each vertex of each attached facet for one where all its facets have teh same normal
* merge second vertex into first
*/
bool
simplify_mesh(struct mesh *mesh)
{
    unsigned int vloop = 0;
    unsigned int vtx1;

    /* ensure index tables are up to date */
    if (mesh->p == NULL) {
        index_mesh(mesh);
    }

    printf("simplifying %d facets with %d vertexes\n", mesh->fcount, mesh->pcount);

    dump_mesh_init(mesh);

    while (vloop < mesh->pcount) {
        /* find a candidate edge */
        if (is_candidate(mesh->p + vloop) && 
            check_adjacent(mesh, vloop, &vtx1)) {

            /* collapse verticies */
            merge_edge(mesh, vloop, vtx1);

            /* do *not* advance past this vertex as we may have just modified
             * it! 
             */
        } else {
            vloop++;
        }
    }

    dump_mesh_fini(mesh);

    return true;
}
