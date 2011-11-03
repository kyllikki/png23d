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



/* debug mesh dumping */

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
dump_mesh_simplify(struct mesh *mesh,
          bool removing,
          unsigned int start,
          unsigned int end)
{
    unsigned int floop;
    struct vertex *v0;
    struct vertex *v1 = NULL;

    if (mesh->dumpfile == NULL)
        return;


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

/** merge an edge by moving all facets from end of edge to start */
static bool
merge_edge(struct mesh *mesh, idxvtx start, idxvtx end)
{
    struct facet *facet;
    struct vertex *evertex;

    dump_mesh_simplify(mesh, true, start, end);

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

    dump_mesh_simplify(mesh, false, start, end);

    return false;
}

/** determinae if a vertex is topoligcally a removal candidate  */
static bool 
is_candidate(struct mesh *mesh, int ivtx)
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


/* simplify mesh by half edge removal
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
    assert(mesh->v != NULL);

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
