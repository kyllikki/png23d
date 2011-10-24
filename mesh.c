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

#define DEBUG_DUMP_MESH 1


/* are two points the same location */
static inline bool
eqpnt(struct pnt *p0, struct pnt *p1)
{
    if ((p0->x == p1->x) &&
        (p0->y == p1->y) &&
        (p0->z == p1->z)) {
        return true;
    } else {
        return false;
    }
}

/* are two points different locations */
static inline
bool nepnt(struct pnt *p0, struct pnt *p1)
{
    if ((p0->x != p1->x) ||
        (p0->y != p1->y) ||
        (p0->z != p1->z)) {
        return true;
    } else {
        return false;
    }
}

static inline int dot_product(pnt *a, pnt *b)
{
    return ((a->x * b->x) + (a->y * b->y) + (a->z * b->z));
}

static inline void cross_product(pnt *n, pnt *a, pnt *b)
{
    n->x = a->y * b->z - a->z * b->y;
    n->y = a->z * b->x - a->x * b->z;
    n->z = a->x * b->y - a->y * b->x;
}

/* calculate the surface normal from three points
*
* @return true if triangle degenerate else false;
*/
static inline bool
pnt_normal(pnt *n, pnt *v0, pnt *v1, pnt *v2)
{
    pnt a;
    pnt b;

    /* normal calculation
     * va = v1 - v0
     * vb = v2 - v0
     *
     * n = va x vb (cross product)
     */
    a.x = v1->x - v0->x;
    a.y = v1->y - v0->y;
    a.z = v1->z - v0->z;

    b.x = v2->x - v0->x;
    b.y = v2->y - v0->y;
    b.z = v2->z - v0->z;

    n->x = a.y * b.z - a.z * b.y;
    n->y = a.z * b.x - a.x * b.z;
    n->z = a.x * b.y - a.y * b.x;

    /* check for degenerate triangle */
    if ((n->x == 0.0) && 
        (n->y == 0.0) && 
        (n->z == 0.0)) {
        return true;
    }

    return false;
}

/* check if two vectors are parallel and the same sign magnitude */
static bool same_normal(pnt *n1, pnt *n2)
{
    pnt cn;

    /* check normal is still same sign magnitude */
    if (dot_product(n1, n2) < 0) {
        return false;
    }

    /* check normals remain parallel */
    cross_product(&cn, n1, n2);
    if (cn.x != 0.0 || cn.y != 0.0 || cn.z != 0.0) {
        /* not parallel */
        return false;     
    }

    return true;
}

#ifdef DEBUG_DUMP_MESH

static int dumpno=0;
static FILE *dumpfile;

#define SVGPX(loc) ( (loc) ) * 60
#define SVGPY(loc) (60 - ((loc) * 60))
//#define SVGPX(loc) (loc) * 4
//#define SVGPY(loc) (60 - ((loc) * 4))

static void
dump_mesh_init(struct mesh *mesh)
{
    dumpfile = fopen("index.html", "w");

    fprintf(dumpfile,"<html>\n<body>");
    fprintf(dumpfile,
            "<h1>simplifying %d facets with %d vertexes</h1>\n",
            mesh->fcount, mesh->pcount);

    fprintf(dumpfile,
            "<table><tr>\n");

}

static void
dump_mesh(struct mesh *mesh,
          bool removing,
          unsigned int start,
          unsigned int end)
{
    unsigned int floop;
    struct vertex *v0 = mesh->p + start;
    struct vertex *v1 = NULL;
    return;
    if (removing) {
        v1 = mesh->p + end;
        fprintf(dumpfile, "<tr><th>Operation %d Removing %u->%u</th>", dumpno, start, end);
        dumpno++;
    }

    fprintf(dumpfile, "<td><svg width=\"500\" height=\"500\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n");

    for (floop = 0; floop < mesh->fcount; floop++) {
        if (same_normal(&mesh->f[floop].n, &v0->facets[0]->n)) {

            fprintf(dumpfile,
                    "<polygon points=\"%.1f,%.1f %.1f,%.1f %.1f,%.1f\" style=\"fill:lime;stroke:black;stroke-width=1\"/>\n",
                    SVGPX(mesh->f[floop].v[0].x), SVGPY(mesh->f[floop].v[0].y),
                    SVGPX(mesh->f[floop].v[1].x), SVGPY(mesh->f[floop].v[1].y),
                    SVGPX(mesh->f[floop].v[2].x), SVGPY(mesh->f[floop].v[2].y));

            /* label facet at centroid */
            fprintf(dumpfile,
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

    if (v1 != NULL) {
        fprintf(dumpfile,
                "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" style=\"stroke:red;stroke-width:5\"/>\n",
                SVGPX(v0->pnt.x), SVGPY(v0->pnt.y),
                SVGPX(v1->pnt.x), SVGPY(v1->pnt.y));

        fprintf(dumpfile,
                "<text x=\"%.1f\" y=\"%.1f\" fill=\"black\">%u</text>\n",
                SVGPX(v1->pnt.x) + 5, SVGPY(v1->pnt.y) + 5,  end);

    }

    fprintf(dumpfile,
            "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"10\" fill=\"blue\"/>\n",
            SVGPX(v0->pnt.x), SVGPY(v0->pnt.y));

    fprintf(dumpfile,
            "<text x=\"%.1f\" y=\"%.1f\" fill=\"black\">%u</text>\n",
            SVGPX(v0->pnt.x) + 10, SVGPY(v0->pnt.y) + 5,  start);


    fprintf(dumpfile, "</svg></td>");

    if (!removing) {
        fprintf(dumpfile, "</tr>");
    }
}



static void dump_mesh_fini(struct mesh *mesh, unsigned int start)
{
    unsigned int floop;
    struct vertex *v0 = mesh->p + start;

    fprintf(dumpfile,"</table>");

    fprintf(dumpfile, "<svg width=\"1000\" height=\"1000\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n");

    for (floop = 0; floop < mesh->fcount; floop++) {
        if (same_normal(&mesh->f[floop].n, &v0->facets[0]->n)) {

            fprintf(dumpfile,
                    "<polygon points=\"%.1f,%.1f %.1f,%.1f %.1f,%.1f\" style=\"fill:lime;stroke:black;stroke-width=1\"/>\n",
                    SVGPX(mesh->f[floop].v[0].x), SVGPY(mesh->f[floop].v[0].y),
                    SVGPX(mesh->f[floop].v[1].x), SVGPY(mesh->f[floop].v[1].y),
                    SVGPX(mesh->f[floop].v[2].x), SVGPY(mesh->f[floop].v[2].y));

            /* label facet at centroid */
            fprintf(dumpfile,
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

    fprintf(dumpfile,"</body>\n</html>\n");

    fclose(dumpfile);

}

#else
#define dump_mesh(...)
#define dump_mesh_init(...)
#define dump_mesh_fini(...)
#endif


static bool
add_facet(struct mesh *mesh,
          float vx0,float vy0, float vz0,
          float vx1,float vy1, float vz1,
          float vx2,float vy2, float vz2)
{
    struct facet *newfacet;
    bool degenerate = false;

    if ((mesh->fcount + 1) > mesh->falloc) {
        /* array needs extending */
        mesh->f = realloc(mesh->f, (mesh->falloc + 1000) * sizeof(struct facet));
        mesh->falloc += 1000;
    }

    newfacet = mesh->f + mesh->fcount;

    newfacet->v[0].x = vx0;
    newfacet->v[0].y = vy0;
    newfacet->v[0].z = vz0;

    newfacet->v[1].x = vx1;
    newfacet->v[1].y = vy1;
    newfacet->v[1].z = vz1;

    newfacet->v[2].x = vx2;
    newfacet->v[2].y = vy2;
    newfacet->v[2].z = vz2;

    degenerate = pnt_normal(&newfacet->n, 
                            &newfacet->v[0], 
                            &newfacet->v[1], 
                            &newfacet->v[2]);

    /* do not add degenerate facets */
    if (!degenerate) {
        mesh->fcount++;
    }

    return degenerate;
}

#define ADDF(xa,ya,za,xb,yb,zb,xc,yc,zc) add_facet(mesh,                \
        x + (xa * width), y + (ya * height), z + (za * depth),          \
        x + (xb * width), y + (yb * height), z + (zb * depth),          \
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


/* determinae if a vertex is topoligcally a removal candidate  */
static bool is_candidate(struct mesh *mesh, int ivtx)
{
    unsigned int floop; /* facet loop */
    struct vertex *vtx = mesh->p + ivtx;

    /* Every facet at the end of the edge must have a normal which is parallel
     * and the same sign magnitude 
     */
    for (floop = 1; floop < vtx->fcount; floop++) {
        if (!same_normal(&vtx->facets[floop - 1]->n, &vtx->facets[floop]->n))
            return false;
    }

    return true;
}


static bool
check_move_ok(struct mesh *mesh, unsigned int from, unsigned int to)
{
    unsigned int floop; /* facet loop */
    struct vertex *fvtx = mesh->p + from; /* from vertex */
    struct vertex *tvtx = mesh->p + to; /* to vertex */
    bool degenerate = false; 
    pnt nn;
    pnt *v0;
    pnt *v1;
    pnt *v2;

    for (floop = 0; floop < fvtx->fcount; floop++) {
        if (fvtx->facets[floop]->i[0] == from) {
            /* from vertex is position 0 */
            v0 = &tvtx->pnt;
            v1 = &fvtx->facets[floop]->v[1];
            v2 = &fvtx->facets[floop]->v[2];
        } else if (fvtx->facets[floop]->i[1] == from) {
            /* from vertex is position 1 */
            v0 = &fvtx->facets[floop]->v[0];
            v1 = &tvtx->pnt;
            v2 = &fvtx->facets[floop]->v[2];
        } else if (fvtx->facets[floop]->i[2] == from) {
            /* from vertex is position 2 */
            v0 = &fvtx->facets[floop]->v[0];
            v1 = &fvtx->facets[floop]->v[1];
            v2 = &tvtx->pnt;
        } else {
            /* none of the facets verticies are the from vertex - uh oh */
            fprintf(stderr, "none of the facets verticies are the from vertex\n");
            return false;
        }

        degenerate = pnt_normal(&nn, v0, v1, v2);

        /* only allow creation of degenerate facets with common verticies */
        if (degenerate) {
            if ((nepnt(v0, v1)) && (nepnt(v1, v2)) && (nepnt(v2, v0))) {
                return false;
            }
        } else {
            if (!same_normal(&nn, &fvtx->facets[floop]->n)) {
                //fprintf(stderr, "normal changed (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n",fvtx->facets[floop]->n.x,fvtx->facets[floop]->n.y,fvtx->facets[floop]->n.z, nn.x,nn.y,nn.z);
                return false;
            }
        }
        
    }
    return true;
}

/** find an adjacent vertex suitabile for removal.
 */
static bool
find_adjacent(struct mesh *mesh, unsigned int ivtx, unsigned int *avtx)
{
    unsigned int floop; /* facet loop */
    unsigned int vloop; /* vertex within facets */
    struct vertex *vtx = mesh->p + ivtx; /* initial vertex */
    unsigned int civtx; /* candidate vertex index */

    /* examine each facet attached to starting vertex */
    for (floop = 0; floop < vtx->fcount; floop++) {
        /* check each vertex of this facet has */
        for (vloop = 0; vloop < 3; vloop++) {
            civtx = vtx->facets[floop]->i[vloop];

            if (civtx == ivtx) {
                continue; /* skip starting vertex */
            }

            if (!is_candidate(mesh, civtx)) {
                continue; /* skep non candidate verticies */
            }

            /* cannot merge edge verticies if it makes the mesh too
             * complicated to represent
             */
            if (((vtx->fcount + mesh->p[civtx].fcount) - 2) > FACETPNT_CNT) {
                continue;
            }

            if(!check_move_ok(mesh, civtx, ivtx)) {
                continue;
            }

            /* found something suitable */
            *avtx = civtx;
            return true;



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


/* move a vertex of a facet
 *
 * changes one vertex of a factet to a new facet and updates the destination
 * vertex to reference this facet.
 *
 * @return false if from vertex was not in facet or facet could not be removed
 * from old vertex
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
    if(remove_facet_from_vertex(facet, mesh->p + from) == false) {
        return false;
    }
    
    /* recompute normal */
    if (pnt_normal(&facet->n, &facet->v[0], &facet->v[1], &facet->v[2])) {
        /* triangle has become degenerate */
        fprintf(stderr, 
                "Degenerate facet %ld on vertex move\n", 
                (facet - mesh->f));
        return false;
    }
    
    return true;
}

static bool
facet_on_vertex(struct facet *facet, struct vertex *vertex)
{
    unsigned int floop;

    for (floop = 0; floop < vertex->fcount; floop++) {
        if (vertex->facets[floop] == facet)
            return true;
    }

    return false;
}




/* merge an edge by moving all facets from end of edge to start */
static bool
merge_edge(struct mesh *mesh, unsigned int start, unsigned int end)
{
    struct facet *facet;

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


static void verify_mesh(struct mesh *mesh)
{
    unsigned int floop; /* facet loop */
    for (floop = 0; floop < mesh->fcount;floop++) {
        if ((mesh->f[floop].i[0] == mesh->f[floop].i[1]) &&
            (mesh->f[floop].i[1] == mesh->f[floop].i[2])) {
            fprintf(stderr,"Indexed facet %u has no surface area\n", floop);
        }

        if ((mesh->f[floop].i[0] == mesh->f[floop].i[1]) ||
            (mesh->f[floop].i[1] == mesh->f[floop].i[2]) ||
            (mesh->f[floop].i[2] == mesh->f[floop].i[0])) {
            fprintf(stderr,"Indexed Facet %u is degenerate\n", floop);
        }

        if ((eqpnt(&mesh->f[floop].v[0], &mesh->f[floop].v[1])) ||
            (eqpnt(&mesh->f[floop].v[1], &mesh->f[floop].v[2])) ||
            (eqpnt(&mesh->f[floop].v[2], &mesh->f[floop].v[0]))) {
            fprintf(stderr,"Facet %u is degenerate\n", floop);
        }

    }
}


/* exported method docuemnted in mesh.h */
struct mesh *
generate_mesh(bitmap *bm, options *options)
{
    unsigned int row_loop;
    unsigned int col_loop;

    uint32_t faces;

    struct mesh *mesh;

    mesh = calloc(1, sizeof(struct mesh));
    if (mesh == NULL) {
        return mesh;
    }

    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            faces = get_face(bm, col_loop, row_loop, options->transparent);

            if (options->finish == FINISH_RAW) {
                output_cube(mesh,
                            col_loop, -(float)row_loop, 0,
                            1, 1, options->depth,
                            faces);
            } else {
                output_marching_squares(mesh,
                                        col_loop, -(float)row_loop, 0,
                                        1, 1, options->depth,
                                        faces);
            }
        }
    }

    return mesh;
}

/* exported method docuemnted in mesh.h */
void free_mesh(struct mesh *mesh)
{
    free(mesh->f);
}

/* exported method docuemnted in mesh.h */
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

    dump_mesh_init(mesh);

    while (vloop < mesh->pcount) {
        /* find a candidate edge */
        if (is_candidate(mesh, vloop) && find_adjacent(mesh, vloop, &vtx1)) {

            /* collapse verticies */
            merge_edge(mesh, vloop, vtx1);

            /* do *not* advance past this vertex as we may have just modified
             * it!
             */
        } else {
            vloop++;
        }
    }

    dump_mesh_fini(mesh, 4);

    verify_mesh(mesh);

    return true;
}
