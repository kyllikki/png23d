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

#define FACETPNT_CNT 32

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

/** An indexed vertex within the mesh.
 */
struct vertex {
    struct pnt pnt; /**< the location of this vertex */
    unsigned int fcount; /**< the number of facets that use this vertex */
    struct facet *facets[FACETPNT_CNT]; /**< facets that use this vertex */
};

/** A 3d triangle mesh
 */
struct mesh {
    /* facets */
    struct facet *f; /**< array of facets */
    uint32_t fcount; /**< number of valid facets in the array */
    uint32_t falloc; /**< numer of facets currently allocated */

    /* indexed vertices */
    struct vertex *p; /**< array of vertices */
    uint32_t pcount; /**< number of valid vertices in the array */
    uint32_t palloc; /**< numer of vertices currently allocated */

    /* stats and meta info */
    uint32_t width; /* conversion source width */
    uint32_t height; /* conversion source height */
    uint32_t cubes; /**< number of cubes with at least one face */
    unsigned int bloom_miss;
    unsigned int find_count; /* number of linear lookups */
    int64_t find_cost;

    /* bloom filter */
    uint8_t *bloom_table; /* table for bloom filter */
    unsigned int bloom_table_entries; /* Number of entries (bits) it bloom */
    unsigned int bloom_iterations; /* number of times the hash is applied with
                                    * a differnt salt value (sometimes referred
                                    * to as the number of functions?)
                                    */



    /* debug */
    int dumpno;
    FILE *dumpfile;

};

/* create new empty mesh */
struct mesh *new_mesh(void);

/** Convert raster image into triangle mesh
 *
 * consider each pixel in the raster image:
 *   - generate a bitfield indicating on which sides of the pixel faces need to
 *     be covered to generate a convex manifold.
 *   - add triangle facets to list for each face present
 *
 * @todo This could probably be better converted to a marching cubes solution
 *       instead of this simple 2d extrusion of modified marching squares
 *       http://en.wikipedia.org/wiki/Marching_cubes
 */
bool mesh_from_bitmap(struct mesh *mesh, bitmap *bm, options *options);

/** free mesh and all resources it holds */
void free_mesh(struct mesh *mesh);

/** update the mesh geometry index representation */
bool index_mesh(struct mesh *mesh, unsigned int bloom_complexity);

/** remove uneccessary verticies */
bool simplify_mesh(struct mesh *mesh, unsigned int bloom_complexity);

/** initialise debugging on mesh */
void debug_mesh_init(struct mesh *mesh, const char* filename);

bool mesh_add_facet(struct mesh *mesh,
                    float vx0,float vy0, float vz0,
                    float vx1,float vy1, float vz1,
                    float vx2,float vy2, float vz2);


#endif
