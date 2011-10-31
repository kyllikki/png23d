/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * Routines to handle mesh generation
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


enum faces {
    FACE_LEFT = 1,
    FACE_RIGHT = 2,
    FACE_TOP = 4,
    FACE_BOT = 8,
    FACE_FRONT = 16,
    FACE_BACK = 32,
};

/** calculate which faces of a cube are adjacent solid cubes
 */
static uint32_t
mesh_gen_get_face(bitmap *bm,
                  unsigned int x,
                  unsigned int y,
                  unsigned int z,
                  struct options *options)
{
    uint32_t faces = 0;
    unsigned int transparent = options->transparent;
    unsigned int pxl_lvl;
    uint8_t pxl_val;

#define Z_LVL_VAL(val) ((val) * (256 / options->levels))

    /* value at which pixel is transparent */
    pxl_lvl = Z_LVL_VAL(z);

    /* pixel value at sample point */
    pxl_val = bm->data[(y * bm->width) + x];

    if ((pxl_val == transparent) ||
        (pxl_val < pxl_lvl)) {
        /* only opaque squares can have faces */
        return faces;
    }

    /* start with all faces set */
    faces = FACE_TOP | FACE_BOT |
            FACE_BACK | FACE_FRONT |
            FACE_LEFT | FACE_RIGHT;

    /* z axis faces */

    /* only the bottom layer has a front face */
    if (z > 0) {
        faces = faces & ~FACE_FRONT;
    }

    if ((z < (options->levels - 1)) &&
        (pxl_val >= Z_LVL_VAL(z + 1))) {
        faces = faces & ~FACE_BACK;
    }

    /* x axis faces */
    if (x > 0) {
        /* pixel value left of sample point */
        pxl_val = bm->data[(y * bm->width) + (x - 1)];
        if ((pxl_val != transparent) &&
            (pxl_val >= pxl_lvl)) {
            faces = faces & ~FACE_LEFT;
        }
    }

    if (x < (bm->width - 1)) {
        /* pixel value right of sample point */
        pxl_val = bm->data[(y * bm->width) + (x + 1)];
        if ((pxl_val != transparent) &&
            (pxl_val >= pxl_lvl)) {
            faces = faces & ~FACE_RIGHT;
        }
    }

    if (y > 0) {
        pxl_val = bm->data[((y - 1) * bm->width) + x ];
        if ((pxl_val != transparent) &&
            (pxl_val >= pxl_lvl)) {
            faces = faces & ~FACE_TOP;
        }
    }

    if (y < (bm->height - 1)) {
        pxl_val = bm->data[((y + 1) * bm->width) + x ];
        if ((pxl_val != transparent) &&
            (pxl_val >= pxl_lvl)) {
            faces = faces & ~FACE_BOT;
        }
    }




    return faces;
}

#define ADDF(xa,ya,za,xb,yb,zb,xc,yc,zc) mesh_add_facet(mesh,           \
        x + (xa * width), y + (ya * height), z + (za * depth),          \
        x + (xb * width), y + (yb * height), z + (zb * depth),          \
        x + (xc * width), y + (yc * height), z + (zc * depth))

/* generates cube facets for a location */
static void
mesh_gen_cube(struct mesh *mesh,
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
mesh_gen_marching_squares(struct mesh *mesh,
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


typedef void (meshgenerator)(struct mesh *mesh,
                        float x, float y, float z,
                        float width, float height, float depth,
                          uint32_t faces);

bool mesh_gen_squares(struct mesh *mesh, bitmap *bm, options *options)
{
    unsigned int yloop;
    unsigned int xloop;
    unsigned int zloop;
    uint32_t faces;
    meshgenerator *meshgen;

    if (options->levels == 1) {
        meshgen = &mesh_gen_marching_squares;
    } else {
        meshgen = &mesh_gen_cube;
    }

    for (zloop = 0; zloop < options->levels; zloop++) {
        for (yloop = 0; yloop < bm->height; yloop++) {
            for (xloop = 0; xloop < bm->width; xloop++) {
                faces = mesh_gen_get_face(bm, xloop, yloop, zloop, options);
                meshgen(mesh, xloop, -(float)yloop, zloop, 1, 1, 1, faces);
            }
        }
    }
    
    return true;
}

bool mesh_gen_cubes(struct mesh *mesh, bitmap *bm, options *options)
{
    unsigned int yloop;
    unsigned int xloop;
    unsigned int zloop;
    uint32_t faces;

    for (zloop = 0; zloop < options->levels; zloop++) {
        for (yloop = 0; yloop < bm->height; yloop++) {
            for (xloop = 0; xloop < bm->width; xloop++) {
                faces = mesh_gen_get_face(bm, xloop, yloop, zloop, options);
                mesh_gen_cube(mesh, xloop, -(float)yloop, zloop, 1, 1, 1, faces);
            }
        }
    }
    
    return true;
}



static inline float 
surfacegen_calcp(bitmap *bm,
                 int x, 
                 int y,
                 options *options)
{
    uint8_t pxl_val;
    int res;

    /* range check, casts are safe because value cannot be -ve from previous
     * test.
     */
    if ((x < 0) || ((unsigned int)x >= bm->width) || 
        (y < 0) || ((unsigned int)y >= bm->height)) {
        return 0.0f;
    }
        
    pxl_val = bm->data[(y * bm->width) + x];

    if (pxl_val == options->transparent) {
        return 0.0f;
    } 

    res = 1+(pxl_val + 1) / (256 / options->levels);

    return res;
}

#define ADDSF(xa,ya,za,xb,yb,zb,xc,yc,zc) mesh_add_facet(mesh,           \
        x + (xa * width), y + (ya * height), za * points[xa][ya],          \
        x + (xb * width), y + (yb * height), zb * points[xb][yb],          \
        x + (xc * width), y + (yc * height), zc * points[xc][yc])

static void
gen_surface(struct mesh *mesh,
                 float x, float y, 
                 float width, float height, 
                 bool evenp, float points[2][2])
{
    if (evenp) {
        if ( (points[0][0] != 0) || 
             (points[1][1] != 0) || 
             (points[0][1] != 0)) {
            /* top surface */
            ADDSF(0,0,1, 0,1,1, 1,1,1);

            /* flat bottom */
            ADDSF(0,0,0, 1,1,0, 0,1,0);
        }

        if ( (points[0][0] != 0) || 
             (points[1][0] != 0) || 
             (points[1][1] != 0)) {

            /* top surface */
            ADDSF(0,0,1, 1,1,1, 1,0,1);

            /* flat bottom */
            ADDSF(0,0,0, 1,0,0, 1,1,0);
        }

    } else {
        if ( (points[0][0] != 0) || 
             (points[1][0] != 0) || 
             (points[0][1] != 0)) {

            /* top surface */
            ADDSF(0,0,1, 0,1,1, 1,0,1);

            /* flat bottom */
            ADDSF(0,0,0, 1,0,0, 0,1,0);
        }

        if ( (points[1][0] != 0) || 
             (points[1][1] != 0) || 
             (points[0][1] != 0)) {
            /* top surface */
            ADDSF(1,0,1, 0,1,1, 1,1,1);

            /* flat bottom */
            ADDSF(1,0,0, 1,1,0, 0,1,0);
        }
    }
}


bool mesh_gen_surface(struct mesh *mesh, bitmap *bm, options *options)
{
    unsigned int yloop;
    unsigned int xloop;
    float points[2][2];

    for (yloop = 0; yloop <= bm->height; yloop++) {
        for (xloop = 0; xloop <= bm->width; xloop++) {

            points[0][0] = surfacegen_calcp(bm, xloop - 1, yloop - 1, options);
            points[1][0] = surfacegen_calcp(bm, xloop, yloop - 1, options);
            points[0][1] = surfacegen_calcp(bm, xloop - 1, yloop, options);
            points[1][1] = surfacegen_calcp(bm, xloop, yloop, options);

            gen_surface(mesh,
                             xloop, -(float)yloop,
                             1, -1, 
                             (((xloop + yloop) & 1) == 0), 
                             points);

        }
    }
    return true;
}
