/* png23d.c
 * 
 * convert png to 3d file
 *
 * MIT Licence
 *
 * Copyright 2011 V. R. Sanders 
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


void output_pgm(uint8_t *bm, uint32_t width, uint32_t height)
{
    printf("P2\n# test output\n%u %u\n255\n",width, height);
    int row_loop;
    int col_loop;
    for (row_loop = 0; row_loop < height; row_loop++) {
        for (col_loop = 0; col_loop < width; col_loop++) {
            printf("%d ",bm[(row_loop * width) + col_loop]);
        }
        printf("\n");
    }
}

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
    /* normal */
    pnt n;

    /* vertecies */
    float vx[3];
    float vy[3];
    float vz[3];
};

static inline void output_stl_tri(const struct stl_facet *facet)
{
    printf("  facet normal %.6f %.6f %.6f\n"
           "    outer loop\n"
           "      vertex %.6f %.6f %.6f\n"
           "      vertex %.6f %.6f %.6f\n"
           "      vertex %.6f %.6f %.6f\n"
           "    endloop\n"
           "  endfacet\n", 
           facet->n.x, facet->n.y, facet->n.z,
           facet->vx[0],facet->vy[0],facet->vz[0],
           facet->vx[1],facet->vy[1],facet->vz[1],
           facet->vx[2],facet->vy[2],facet->vz[2]);
}

struct stl_facets {
    struct stl_facet **v; /* array of facets */
    int count; /* number of valid facets in the array */
    int facet_alloc; /* numer of facets currently allocated */
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

    newfacet->vx[0] = vx0;
    newfacet->vy[0] = vy0;
    newfacet->vz[0] = vz0;

    newfacet->vx[1] = vx1;
    newfacet->vy[1] = vy1;
    newfacet->vz[1] = vz1;

    newfacet->vx[2] = vx2;
    newfacet->vy[2] = vy2;
    newfacet->vz[2] = vz2;

    return newfacet;
}

static void 
output_stl_cube(struct stl_facets *facets, int x,int y, int z, int width, int height, int depth, uint32_t faces)
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

uint32_t get_stl_face(uint8_t *bm, uint32_t width, uint32_t height, int x, int y, uint8_t transparent)
{
    uint32_t faces = 0;
    if (bm[(y * width) + x] < transparent) {
        /* only opaque squares have faces */
        faces = STL_FACE_TOP | STL_FACE_BOT |
                STL_FACE_FRONT | STL_FACE_BACK |
                STL_FACE_LEFT | STL_FACE_RIGHT;

        if ((x > 0) && (bm[(y * width) + (x - 1)] < transparent)) {
            faces = faces & ~STL_FACE_LEFT;
        }

        if ((x < (width -1)) && (bm[(y * width) + (x + 1)] < transparent)) {
            faces = faces & ~STL_FACE_RIGHT;
        }

        if ((y > 0) && (bm[((y - 1) * width) + x ] < transparent)) {
            faces = faces & ~STL_FACE_TOP;
        }

        if ((y < (height -1)) && (bm[((y + 1) * width) + x ] < transparent)) {
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
void stl_gen_facets(struct stl_facets *facets, uint8_t *bm, uint32_t width, uint32_t height, uint8_t transparent)
{
    int row_loop;
    int col_loop;

    int xoff; /* x offset so 3d model is centered */
    int yoff; /* y offset so 3d model is centered */

    int cubes = 0;
    uint32_t faces;

    xoff = (width / 2);
    yoff = (height / 2);

    for (row_loop = 0; row_loop < height; row_loop++) {
        for (col_loop = 0; col_loop < width; col_loop++) {
            faces = get_stl_face(bm, width, height, col_loop, row_loop, transparent);
            if (faces != 0) {
                output_stl_cube(facets, col_loop - xoff, yoff - row_loop, 0, 1, 1, 1, faces); 
                cubes++;
            }
        }

    }

    fprintf(stderr, "cubes %d facets %d\n", cubes, facets->count);

}

void output_flat_stl(uint8_t *bm, uint32_t width, uint32_t height, uint8_t transparent)
{
    struct stl_facets facets;
    int floop;

    memset(&facets, 0 , sizeof(struct stl_facets));

    printf("solid png2stl_Model\n");

    stl_gen_facets(&facets, bm, width, height, transparent);

    for (floop=0; floop < facets.count; floop++) {
        output_stl_tri(*(facets.v + floop));
    }


    printf("endsolid png2stl_Model\n");

}


static void 
output_scad_cube(int x,int y, int z, int width, int height, int depth)
{
    printf("        translate([%d, %d, %d]) cube([%d.01, %d.01, %d.01]);\n",x,y,z,width, height, depth);
}

/* generate scad output as rows of cubes */
void output_flat_scad_cubes(uint8_t *bm, uint32_t width,uint32_t height, uint8_t transparent)
{
    int xoff; /* x offset so 3d model is centered */
    int yoff; /* y offset so 3d model is centered */
    int row_loop;
    int col_loop;
    int col_start; /* start col of run */
    int xmin = width;
    int xmax = 0;
    int ymin = height;
    int ymax = 0;

    xoff = (width / 2);
    yoff = (height / 2);

    printf("// Generated by png2scad\n\n");

    printf("target_width = %d;\n",75);
    printf("target_depth = %d;\n\n",2);

    printf("module image(sx,sy,sz) {\n scale([sx, sy, sz])  union() {\n");

    for (row_loop = 0; row_loop < height; row_loop++) {
        col_start = width;
        for (col_loop = 0; col_loop < width; col_loop++) {
            
            if (bm[(row_loop * width) + col_loop] < transparent) {
                /* this cell is "opaque" */
                if (col_start > col_loop) {
                    /* mark start of run */
                    col_start = col_loop;
                }
                if (col_loop < xmin)
                    xmin = col_loop;
                if (col_loop > xmax)
                    xmax = col_loop;
                if (row_loop < ymin)
                    ymin = row_loop;
                if (row_loop > ymax)
                    ymax = row_loop;

            } else {
                /* cell is transparent */
                if (col_start < col_loop) {
                    /* output previous run */
                    output_scad_cube(col_start - xoff, yoff - row_loop , 0,
                                     col_loop - col_start, 1, 1);
                    col_start = width; /* ready for next run */
                }
            }
        }
        /* need to close any active run at edge of image */
        if (col_start < col_loop) {
            /* output active run */
            output_scad_cube(col_start - xoff, row_loop - yoff, 0,
                             col_loop - col_start, 1, 1);
        }

    }

    printf("    }\n}\n");

    printf("\nimage_width = %d;\nimage_height = %d;\n\n", xmax - xmin, ymax - ymin);

    printf("image(target_width / image_width, target_width / image_width, target_depth);\n");

}

int main(int argc, char **argv)
{
    uint8_t *bm;
    uint32_t width;
    uint32_t height;

    bm = create_gs_bitmap(argv[1], &width, &height);
    if (bm==NULL) {
        return 1;
    }

    //output_pgm(bm,width,height);
    //output_flat_scad_cubes(bm, width, height, 210);
    output_flat_stl(bm, width, height, 255);

    free(bm);
    return 0;
}
