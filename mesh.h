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

#define FACETPNT_CNT 16

/* 3d point */
typedef struct pnt {
    float x;
    float y;
    float z;
} pnt;


/* an indexed point */
typedef uint32_t idxpnt;

/** facet
 *
 * A facet is a triangle with its normal 
 */
struct facet {
    pnt n; /**< surface normal */
    pnt v[3]; /**< triangle vertices */
    idxpnt i[3]; /** triangle indexed vertices */
};

struct vertex {
    struct pnt *p; /* the location of this vertex */
    unsigned int fcount; /* the number of facets that use this vertex */
    struct facet *facets[FACETPNT_CNT]; /* facets that use this vertex */
};

struct facets {
    /* facets */
    struct facet *f; /**< array of facets */
    uint32_t fcount; /**< number of valid facets in the array */
    uint32_t falloc; /**< numer of facets currently allocated */

    /* indexed points */
    struct vertex *p; /* array of verticies */
    uint32_t pcount; /* number of valid verticies in the array */
    uint32_t palloc; /* numer of verticies currently allocated */
    
    uint32_t cubes; /* number of cubes with at lease one face */
};


/** Generate list of triangle facets from a bitmap
 *
 * 
 */
struct facets *gen_facets(bitmap *bm, options *options);
void free_facets(struct facets *facets);

/* update the geometry with index representation */
bool update_indexing(struct facets *facets);

/* remove uneccessary verticies */
bool simplify_mesh(struct facets *facets);

#endif
