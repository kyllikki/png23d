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


#define DUMP_SVG_SIZE 500


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
dump_mesh_simplify_init(struct mesh *mesh)
{
    if (mesh->dumpfile == NULL)
        return;

    fprintf(mesh->dumpfile,
            "<h2>Mesh Simplify</h2><p>Starting with %d facets and %d vertexes.\n",
            mesh->fcount, mesh->vcount);

    fprintf(mesh->dumpfile, "<table><tr>\n");
}

static void
dump_mesh(struct mesh *mesh,
          bool removing,
          unsigned int start,
          unsigned int end)
{
    unsigned int floop;
    struct vertex *v0;
    struct vertex *v1 = NULL;

    if (mesh->dumpfile == NULL)
        return;

#define SVGPX(loc) ( (loc) ) * (DUMP_SVG_SIZE / mesh->width)
#define SVGPY(loc) (mesh->height - SVGPX(loc))

    v0 = vertex_from_index(mesh, start);

    if (removing) {
        v1 = vertex_from_index(mesh, end);
        fprintf(mesh->dumpfile,
                "<tr><th>Operation %d Removing %u->%u</th>",
                mesh->dumpno, start, end);
        mesh->dumpno++;
    }

    fprintf(mesh->dumpfile, "<td><svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n", DUMP_SVG_SIZE, DUMP_SVG_SIZE);

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

    if (v1 != NULL) {
        fprintf(mesh->dumpfile,
                "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" style=\"stroke:red;stroke-width:5\"/>\n",
                SVGPX(v0->pnt.x), SVGPY(v0->pnt.y),
                SVGPX(v1->pnt.x), SVGPY(v1->pnt.y));

        fprintf(mesh->dumpfile,
                "<text x=\"%.1f\" y=\"%.1f\" fill=\"black\">%u</text>\n",
                SVGPX(v1->pnt.x) + 5, SVGPY(v1->pnt.y) + 5,  end);

    }

    fprintf(mesh->dumpfile,
            "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"10\" fill=\"blue\"/>\n",
            SVGPX(v0->pnt.x), SVGPY(v0->pnt.y));

    fprintf(mesh->dumpfile,
            "<text x=\"%.1f\" y=\"%.1f\" fill=\"black\">%u</text>\n",
            SVGPX(v0->pnt.x) + 10, SVGPY(v0->pnt.y) + 5,  start);


    fprintf(mesh->dumpfile, "</svg></td>");

    if (!removing) {
        fprintf(mesh->dumpfile, "</tr>");
    }
}



static void
dump_mesh_simplify_fini(struct mesh *mesh)
{
    if (mesh->dumpfile == NULL)
        return;

    fprintf(mesh->dumpfile,"</table>");
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





/* determinae if a vertex is topoligcally a removal candidate  */
static bool is_candidate(struct mesh *mesh, int ivtx)
{
    unsigned int floop; /* facet loop */
    struct vertex *vtx = vertex_from_index(mesh, ivtx);

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
    struct vertex *fvtx; /* from vertex */
    struct vertex *tvtx; /* to vertex */
    bool degenerate = false;
    pnt nn;
    pnt *v0;
    pnt *v1;
    pnt *v2;

    fvtx = vertex_from_index(mesh, from);
    tvtx = vertex_from_index(mesh, to);

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
    struct vertex *vtx; /* initial vertex */
    unsigned int civtx; /* candidate vertex index */
    struct vertex *cvtx; /* candidate vertex */

    vtx = vertex_from_index(mesh, ivtx);

    /* examine each facet attached to starting vertex */
    for (floop = 0; floop < vtx->fcount; floop++) {
        /* check each vertex of this facet has */
        for (vloop = 0; vloop < 3; vloop++) {
            civtx = vtx->facets[floop]->i[vloop];

            if (civtx == ivtx) {
                continue; /* skip starting vertex */
            }

            if (!is_candidate(mesh, civtx)) {
                continue; /* skip non candidate verticies */
            }

            cvtx = vertex_from_index(mesh, civtx);

            /* cannot merge edge verticies if it makes the mesh too
             * complicated to represent
             */
            if (((vtx->fcount + cvtx->fcount) - 2) > mesh->vertex_fcount) {
                continue;
            }

            if (!check_move_ok(mesh, civtx, ivtx)) {
                continue;
            }

            /* found something suitable */
            *avtx = civtx;
            return true;



        }
    }
    return false; /* no match */
}


static bool remove_facet(struct mesh *mesh, struct facet *facet)
{
    struct facet *rfacet;

    /* remove facet from all three vertecies */
    remove_facet_from_vertex(mesh, facet, facet->i[0]);
    remove_facet_from_vertex(mesh, facet, facet->i[1]);
    remove_facet_from_vertex(mesh, facet, facet->i[2]);

    /* only way to efficiently remove a facet is to move the one at the end of
     * the list here instead
     */
    mesh->fcount--;
    rfacet = mesh->f + mesh->fcount;
    if (rfacet != facet) {
        /* was not already the end entry, have to do some work */
        memcpy(facet, rfacet, sizeof(struct facet));
        /* fix up vertex indexes */
        remove_facet_from_vertex(mesh, rfacet, facet->i[0]);
        remove_facet_from_vertex(mesh, rfacet, facet->i[1]);
        remove_facet_from_vertex(mesh, rfacet, facet->i[2]);

        add_facet_to_vertex(mesh, facet, facet->i[0]);
        add_facet_to_vertex(mesh, facet, facet->i[1]);
        add_facet_to_vertex(mesh, facet, facet->i[2]);

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
                  idxvtx from,
                  idxvtx to)
{
    struct vertex *tvtx;
    tvtx = vertex_from_index(mesh, to);

    if (facet->i[0] == from) {
        facet->i[0] = to;
        facet->v[0] = tvtx->pnt;
    } else if (facet->i[1] == from) {
        facet->i[1] = to;
        facet->v[1] = tvtx->pnt;
    } else if (facet->i[2] == from) {
        facet->i[2] = to;
        facet->v[2] = tvtx->pnt;
    } else {
        return false;
    }
    /* add facet to destination vertex */
    add_facet_to_vertex(mesh, facet, to);

    /* remove facet from original vertex */
    if (remove_facet_from_vertex(mesh, facet, from) == false) {
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
facet_on_vertex(struct mesh *mesh, struct facet *facet, idxvtx ivertex)
{
    unsigned int floop;

    struct vertex *vertex = vertex_from_index(mesh, ivertex);

    for (floop = 0; floop < vertex->fcount; floop++) {
        if (vertex->facets[floop] == facet)
            return true;
    }

    return false;
}




/* merge an edge by moving all facets from end of edge to start */
static bool
merge_edge(struct mesh *mesh, idxvtx start, idxvtx end)
{
    struct facet *facet;
    struct vertex *evertex;

    dump_mesh(mesh, true, start, end);

    evertex = vertex_from_index(mesh, end);

    /* change all the facets on end vertex to point at start virtex
     * instead
     *
     * delete degenerate facets(two of their vertecies will be the same
     */
    while (evertex->fcount > 0) {
        facet = evertex->facets[0];

        if (facet_on_vertex(mesh, facet, start)) {
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





/* exported method documented in mesh.h */
bool
mesh_add_facet(struct mesh *mesh,
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


/* exported method documented in mesh.h */
struct mesh *new_mesh(void)
{
    struct mesh *mesh;

    mesh = calloc(1, sizeof(struct mesh));

    return mesh;
}


/* exported method documented in mesh.h */
bool
mesh_from_bitmap(struct mesh *mesh, bitmap *bm, options *options)
{
    bool res = false;

    mesh->height = bm->height;
    mesh->width = bm->width;

    INFO("Generating mesh from bitmap of size %dx%d with %d levels\n",
         bm->width, bm->height, options->levels);

    switch (options->finish) {
    case FINISH_SURFACE:
        res = mesh_gen_surface(mesh, bm, options);
        break;

    case FINISH_SMOOTH:
        res = mesh_gen_squares(mesh, bm, options);
        break;

    case FINISH_RAW:
        res = mesh_gen_cubes(mesh, bm, options);
        break;
    }

    return res;
}

/* exported method documented in mesh.h */
void free_mesh(struct mesh *mesh)
{
    debug_mesh_fini(mesh, 4);
    free(mesh->bloom_table);
    free(mesh->f);
}



/* simplify mesh by edge removal
 *
 * algorithm is:
 * find vertex where all facets have the same normal
 * search each vertex of each attached facet for one where all its facets have teh same normal
 * merge second vertex into first
 */
bool
simplify_mesh(struct mesh *mesh,
              unsigned int bloom_complexity,
              unsigned int vertex_fcount)
{
    unsigned int vloop = 0;
    unsigned int vtx1;

    /* ensure index tables are up to date */
    if (mesh->v == NULL) {
        index_mesh(mesh, bloom_complexity, vertex_fcount);
    }

    dump_mesh_simplify_init(mesh);

    while (vloop < mesh->vcount) {
        /* find a candidate edge */
        if (is_candidate(mesh, vloop) &&
            find_adjacent(mesh, vloop, &vtx1)) {

            /* collapse verticies */
            merge_edge(mesh, vloop, vtx1);

            /* do *not* advance past this vertex as we may have just modified
             * it!
             */
        } else {
            vloop++;
        }
    }

    dump_mesh_simplify_fini(mesh);

    verify_mesh(mesh);

    return true;
}
