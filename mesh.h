/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * mesh routines header.
 */

#ifndef PNG23D_MESH_H
#define PNG23D_MESH_H 1

/* 3d point */
typedef struct pnt {
    float x;
    float y;
    float z;
} pnt;

/** facet
 *
 * A facet is a triangle with its normal 
 */
struct facet {
    pnt n; /**< surface normal */
    pnt v[3]; /**< triangle vertexes */
    uint16_t attribute;
} __attribute__((packed));

struct facets {
    struct facet **v; /* array of facets */
    uint32_t count; /* number of valid facets in the array */
    uint32_t facet_alloc; /* numer of facets currently allocated */
    
    uint32_t cubes; /* number of cubes with at lease one face */
};

struct facets *gen_facets(bitmap *bm, options *options);
void free_facets(struct facets *facets);

#endif
