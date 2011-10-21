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
        if ((mesh->p[idx].p->x == pnt->x) &&
            (mesh->p[idx].p->y == pnt->y) &&
            (mesh->p[idx].p->z == pnt->z)) {
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

        mesh->p[mesh->pcount].p = npnt;
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

/* determinae if a vertex is a removal candidate */
static bool is_candidate(struct vertex *vtx)
{
    unsigned int floop; /* facet loop */

    /* walk the normals to check tehy are all the same */
    for (floop = 1; floop < vtx->fcount; floop++) {
        if (nepnt(&vtx->facets[floop - 1]->n, &vtx->facets[floop]->n))
            return false;
    }

    return true;
}

static bool
check_adjacent(struct mesh *mesh, unsigned int ivtx, unsigned int *avtx)
{
    unsigned int floop; /* facet loop */
    unsigned int vloop; /* vertex within facets */
    struct vertex *vtx = mesh->p + ivtx; /* initial vertex */

    for (floop = 0; floop < vtx->fcount; floop++) {
        for (vloop = 0; vloop < 3; vloop++) {
            if (vtx->facets[floop]->i[vloop] == ivtx) {
                continue; /* skip given vertex */
            }
            if (is_candidate(mesh->p + vtx->facets[floop]->i[vloop])) {
                *avtx = vtx->facets[floop]->i[vloop];
                return true;
            }

        }
    }
    return false; /* no match */
}


static bool
merge_vertex(unsigned int ivtx0, unsigned int ivtx1)
{
    fprintf(stderr,"remove edge %u,%u\n", ivtx0, ivtx1);
    return false;
}

/* simplify mesh by edge removal
*
* algorithm is:
* find vertex where all facets have the same normal
* search each vertex of each attached facet for one where all its facets have teh same normal
* merge second vertex into first
* delete degenerate facets(two of their vertecies will be the same
*/
bool
simplify_mesh(struct mesh *mesh)
{
    unsigned int vloop;
    unsigned int vtx1;

    /* ensure index tables are up to date */
    if (mesh->p == NULL) {
        index_mesh(mesh);
    }

    for (vloop = 0; vloop < mesh->pcount; vloop++) {
        if (is_candidate(mesh->p + vloop)) {
            /* check adjacent vertecies */
            if (check_adjacent(mesh, vloop, &vtx1)) {
                /* collapse verticies */
                merge_vertex(vloop, vtx1);
            }
        }
    }
    return true;
}
