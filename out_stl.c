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
#include "out_stl.h"

enum stl_faces {
    STL_FACE_TOP = 1,
    STL_FACE_BOT = 2,
    STL_FACE_FRONT = 4,
    STL_FACE_BACK = 8,
    STL_FACE_LEFT = 16,
    STL_FACE_RIGHT = 32,
};

/* 3d point */
typedef struct pnt {
    float x;
    float y;
    float z;
} pnt;

struct stl_facet {
    pnt n; /**< surface normal */
    pnt v[3]; /**< triangle vertexes */
    uint16_t attribute;
} __attribute__((packed));

struct stl_facets {
    struct stl_facet **v; /* array of facets */
    uint32_t count; /* number of valid facets in the array */
    uint32_t facet_alloc; /* numer of facets currently allocated */
};


static void add_facet(struct stl_facets *facets, struct stl_facet *newfacet)
{
    if ((facets->count + 1) > facets->facet_alloc) {
        /* array needs extending */
        facets->v = realloc(facets->v, (facets->facet_alloc + 1000) * sizeof(struct stl_facet *));
        facets->facet_alloc += 1000;
    }
    *(facets->v + facets->count) = newfacet;
    facets->count++;
}


static inline struct stl_facet *
create_facet(float vx0,float vy0, float vz0,
             float vx1,float vy1, float vz1,
             float vx2,float vy2, float vz2)
{
    pnt a;
    pnt b;
    struct stl_facet *newfacet;

    newfacet = malloc(sizeof(struct stl_facet));

    /* va = v1 - v0
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

    return newfacet;
}

static void 
output_stl_cube(struct stl_facets *facets, 
                float x, float y, float z, 
                float width, float height, float depth, 
                uint32_t faces)
{
    if ((faces & STL_FACE_TOP) != 0) {
        add_facet(facets, 
                  create_facet(x        , y + height, z        ,
                               x        , y + height, z + depth,
                               x + width, y + height, z));

        add_facet(facets, 
                  create_facet(x        , y + height, z + depth,
                               x + width, y + height, z + depth,
                               x + width, y + height, z));

    }

    if ((faces & STL_FACE_BOT) != 0) {
        add_facet(facets, 
                  create_facet(x        , y         , z        ,
                               x + width, y         , z        ,
                               x        , y         , z + depth));

        add_facet(facets, 
                  create_facet(x        , y         , z + depth,
                               x + width, y         , z        ,
                               x + width, y         , z + depth));
    }

    if ((faces & STL_FACE_FRONT) != 0) {
        add_facet(facets, 
                  create_facet(x        , y         , z        ,
                               x        , y + height, z        ,
                               x + width, y         , z        ));

        add_facet(facets, 
                  create_facet(x        , y + height, z        ,
                               x + width, y + height, z        ,
                               x + width, y         , z        ));

    }

    if ((faces & STL_FACE_BACK) != 0) {
        add_facet(facets, 
                  create_facet(x        , y         , z + depth,
                               x + width, y         , z + depth,
                               x        , y + height, z + depth));

        add_facet(facets, 
                  create_facet(x        , y + height, z + depth,
                               x + width, y         , z + depth,
                               x + width, y + height, z + depth));
    }

    if ((faces & STL_FACE_LEFT) != 0) {
        add_facet(facets, 
                  create_facet(x        , y         , z        ,
                               x        , y         , z + depth,
                               x        , y + height, z        ));

        add_facet(facets, 
                  create_facet(x        , y + height, z        ,
                               x        , y         , z + depth,
                               x        , y + height, z + depth));

    }

    if ((faces & STL_FACE_RIGHT) != 0) {
        add_facet(facets, 
                  create_facet(x + width, y         , z        ,
                               x + width, y + height, z        ,
                               x + width, y         , z + depth));

        add_facet(facets, 
                  create_facet(x + width, y + height, z        ,
                               x + width, y + height, z + depth,
                               x + width, y         , z + depth));

    }


}

static uint32_t get_stl_face(bitmap *bm, unsigned int x, unsigned int y, uint8_t transparent)
{
    uint32_t faces = 0;
    if (bm->data[(y * bm->width) + x] < transparent) {
        /* only opaque squares have faces */
        faces = STL_FACE_TOP | STL_FACE_BOT |
                STL_FACE_FRONT | STL_FACE_BACK |
                STL_FACE_LEFT | STL_FACE_RIGHT;

        if ((x > 0) && (bm->data[(y * bm->width) + (x - 1)] < transparent)) {
            faces = faces & ~STL_FACE_LEFT;
        }

        if ((x < (bm->width - 1)) && 
            (bm->data[(y * bm->width) + (x + 1)] < transparent)) {
            faces = faces & ~STL_FACE_RIGHT;
        }

        if ((y > 0) && 
            (bm->data[((y - 1) * bm->width) + x ] < transparent)) {
            faces = faces & ~STL_FACE_TOP;
        }

        if ((y < (bm->height - 1)) && 
            (bm->data[((y + 1) * bm->width) + x ] < transparent)) {
            faces = faces & ~STL_FACE_BOT;
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
static void 
stl_gen_facets(struct stl_facets *facets, bitmap *bm, options *options)
{
    unsigned int row_loop;
    unsigned int col_loop;
    float scale = options->width / bm->width;

    float xoff; /* x offset so 3d model is centered */
    float yoff; /* y offset so 3d model is centered */

    int cubes = 0;
    uint32_t faces;

    /*fprintf(stderr,"scale %f options->width %f bm->width %d\n", scale, options->width, bm->width);*/


    xoff = (options->width / 2);
    yoff = ((bm->height * scale) / 2);

    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            faces = get_stl_face(bm, col_loop, row_loop, options->transparent);
            if (faces != 0) {
                output_stl_cube(facets, 
                                (col_loop * scale) - xoff, yoff - (row_loop * scale), 0, 
                                scale, scale, options->depth, 
                                faces); 
                cubes++;
            }
        }

    }

    fprintf(stderr, "cubes %d facets %d\n", cubes, facets->count);

}

static void free_facets(struct stl_facets *facets)
{
    unsigned int floop;

    for (floop = 0; floop < facets->count; floop++) {
        free(*(facets->v + floop));
    }
    free(facets->v);
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
    struct stl_facets facets;
    int floop;
    uint8_t header[80];

    assert(sizeof(struct stl_facet) == 50); /* this is foul and nasty */

    memset(&facets, 0 , sizeof(struct stl_facets));

    stl_gen_facets(&facets, bm, options);

    memset(header, 0, 80);

    snprintf((char *)header, 80, "Binary STL generated by png23d from %s", options->infile);

    write(fd, header, 80);
    write(fd, &facets.count, sizeof(uint32_t));

    for (floop=0; floop < facets.count; floop++) {
        write(fd, *(facets.v + floop), sizeof(struct stl_facet));
    }

    free_facets(&facets);

    return true;
}

static inline void output_stl_tri(FILE *outf, const struct stl_facet *facet)
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
bool output_flat_astl(bitmap *bm, int fd, options *options)
{
    struct stl_facets facets;
    unsigned int floop;
    FILE *outf;

    outf = fdopen(dup(fd), "w");

    memset(&facets, 0 , sizeof(struct stl_facets));

    stl_gen_facets(&facets, bm, options);

    fprintf(outf, "solid png2stl_Model\n");

    for (floop = 0; floop < facets.count; floop++) {
        output_stl_tri(outf, *(facets.v + floop));
    }

    fprintf(outf, "endsolid png2stl_Model\n");

    free_facets(&facets);

    fclose(outf);

    return true;
}
