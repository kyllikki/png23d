/* png2scad.c
 * 
 * convert png to scad file
 *
 * MIT Licence
 *
 * Copyright 2011 V. R. Sanders 
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <png.h>


static uint8_t *
create_gs_bitmap(const char *filename, uint32_t *rwidth, uint32_t *rheight)
{
    int hdr_len = 8;
    FILE *fp; /* input file pointer */
    png_byte header[hdr_len]; /* input firl header bytes to check it is a png */
    uint8_t *bitmap; /* output bitmap */
    int bit_depth;
    int color_type;
    int interlace_method;
    int compression_method;
    int filter_method;
    png_uint_32 width;
    png_uint_32 height;
    int channels;
    png_structp png_ptr; /* png read context */
    png_infop info_ptr;/* png information before decode */
    png_infop end_info; /* png info after decode */
    png_bytep *row_pointers; /* storage for row pointers */
    int row_loop; /* loop to initialise row pointers */

    fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    if (fread(header, 1, hdr_len, fp) != hdr_len) {
        fclose(fp);
        return NULL;
    }

    if (png_sig_cmp(header, 0, hdr_len) != 0) {
        fclose(fp);
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)  {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        fclose(fp);
        return NULL;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);

    png_set_sig_bytes(png_ptr, hdr_len);

    png_read_info(png_ptr, info_ptr);


    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, &interlace_method,
                 &compression_method, &filter_method);

    /* set up input filtering */

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY &&
        bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);

    png_read_update_info(png_ptr, info_ptr);

    /* check filters result in single 8bit channel */
    channels = png_get_channels(png_ptr, info_ptr);

    if (channels != 1) {
        fclose(fp);
        return NULL;
    }
    
    bitmap = malloc(width *height);
    if (bitmap != NULL) {
        row_pointers = malloc(sizeof(png_bytep) * height);
        if (row_pointers != NULL) {

            *rwidth = width;
            *rheight = height;

            for (row_loop = 0; row_loop <height; row_loop++) {
                *(row_pointers + row_loop) = bitmap + (row_loop * width);
            }

            png_read_image(png_ptr, row_pointers);
    
            png_read_end(png_ptr, end_info);

            free(row_pointers);

        } else {
            free(bitmap);
            bitmap = NULL;
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    fclose(fp);

    return bitmap;
}

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

struct stl_facet {
    /* normals */
    float nx;
    float ny;
    float nz;

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
           facet->nx, facet->ny, facet->nz,
           facet->vx[0],facet->vy[0],facet->vz[0],
           facet->vx[1],facet->vy[1],facet->vz[1],
           facet->vx[2],facet->vy[2],facet->vz[2]);
}

static int stl_faces = 0;

struct stl_facets {
    struct stl_facet *v; /* array of facets */
    int facetc; /* number of valid facets in the array */
    int facet_alloc; /* numer of facets currently allocated */
};

static void add_facet(struct stl_facets *facets, struct stl_facet *newfacet)
{
    if ((facets->facetc + 1) > facets->facet_alloc) {
        /* array needs extending */
        facets->v = realloc(facets->v, (facets->facet_alloc + 1000) * sizeof(struct stl_facet *));
        facets->facet_alloc += 1000
    }
    facets->v + facets->facetc = newfacet;
    facets->facetc++;
}


static inline struct stl_facet *
create_facet(float nx,float ny, float nz,
             float vx0,float vy0, float vz0,
             float vx1,float vy1, float vz1,
             float vx2,float vy2, float vz2)
{
    struct stl_facet *newfacet;
    newfacet = malloc(sizeof(struct stl_facet));

    newfacet.nx = nx;
    newfacet.ny = ny;
    newfacet.nz = nz;
    newfacet.vx[0] = vx0;
    newfacet.vy[0] = vy0;
    newfacet.vz[0] = vz0;

    newfacet.vx[1] = vx1;
    newfacet.vy[1] = vy1;
    newfacet.vz[1] = vz1;

    newfacet.vx[2] = vx2;
    newfacet.vy[2] = vy2;
    newfacet.vz[2] = vz2;

    return newfacet;
}

static void 
output_stl_cube(struct stl_facets *facets, int x,int y, int z, int width, int height, int depth, uint32_t faces)
{
    struct stl_facet facet;

    if ((faces & STL_FACE_TOP) != 0) {
        stl_faces+=2;

        add_facet(facets, 
                  create_facet(0        , 1.0       , 0        ,
                               x        , y + height, z        ,
                               x        , y + height, z + depth,
                               x + width, y + height, z));

        add_facet(facets, 
                  create_facet(0        , 1.0       , 0        ,
                               x        , y + height, z + depth,
                               x + width, y + height, z + depth,
                               x + width, y + height, z));

        facet.nx = 0;
        facet.ny = 1.0;
        facet.nz = 0;
        facet.vx[0] = x;
        facet.vy[0] = y + height;
        facet.vz[0] = z;

        facet.vx[1] = x;
        facet.vy[1] = y + height;
        facet.vz[1] = z + depth;

        facet.vx[2] = x + width;
        facet.vy[2] = y + height;
        facet.vz[2] = z;

        output_stl_tri(&facet);
        facet.vz[0] = z + depth;
        facet.vx[1] = x + width;
        output_stl_tri(&facet);
    }

    if ((faces & STL_FACE_BOT) != 0) {
        stl_faces+=2;

        facet.nx = 0;
        facet.ny = -1.0;
        facet.nz = 0;
        facet.vx[0] = x;
        facet.vy[0] = y;
        facet.vz[0] = z;
        facet.vx[1] = x;
        facet.vy[1] = y;
        facet.vz[1] = z + depth;
        facet.vx[2] = x + width;
        facet.vy[2] = y;
        facet.vz[2] = z;
        output_stl_tri(&facet);
        facet.vz[0] = z + depth;
        facet.vx[1] = x + width;
        output_stl_tri(&facet);
    }

    if ((faces & STL_FACE_FRONT) != 0) {
        stl_faces+=2;

        facet.nx = 0;
        facet.ny = 0;
        facet.nz = -1.0;
        facet.vx[0] = x;
        facet.vy[0] = y;
        facet.vz[0] = z;
        facet.vx[1] = x;
        facet.vy[1] = y + height;
        facet.vz[1] = z;
        facet.vx[2] = x + width;
        facet.vy[2] = y;
        facet.vz[2] = z;
        output_stl_tri(&facet);
        facet.vy[0] = y + height;
        facet.vx[1] = x + width;
        output_stl_tri(&facet);
    }

    if ((faces & STL_FACE_BACK) != 0) {
        stl_faces+=2;

        facet.nx = 0;
        facet.ny = 0;
        facet.nz = 1.0;
        facet.vx[0] = x;
        facet.vy[0] = y;
        facet.vz[0] = z + depth;
        facet.vx[1] = x;
        facet.vy[1] = y + height;
        facet.vz[1] = z + depth;
        facet.vx[2] = x + width;
        facet.vy[2] = y;
        facet.vz[2] = z + depth;
        output_stl_tri(&facet);
        facet.vy[0] = y + height;
        facet.vx[1] = x + width;
        output_stl_tri(&facet);
    }

    if ((faces & STL_FACE_LEFT) != 0) {
        stl_faces+=2;

        facet.nx = -1.0;
        facet.ny = 0;
        facet.nz = 0;
        facet.vx[0] = x;
        facet.vy[0] = y;
        facet.vz[0] = z;
        facet.vx[1] = x;
        facet.vy[1] = y + height;
        facet.vz[1] = z;
        facet.vx[2] = x;
        facet.vy[2] = y;
        facet.vz[2] = z + depth;
        output_stl_tri(&facet);
        facet.vy[0] = y + height;
        facet.vz[1] = z + depth;
        output_stl_tri(&facet);
    }

    if ((faces & STL_FACE_RIGHT) != 0) {
        stl_faces+=2;

        facet.nx = 1.0;
        facet.ny = 0;
        facet.nz = 0;
        facet.vx[0] = x + width;
        facet.vy[0] = y;
        facet.vz[0] = z;
        facet.vx[1] = x + width;
        facet.vy[1] = y + height;
        facet.vz[1] = z;
        facet.vx[2] = x + width;
        facet.vy[2] = y;
        facet.vz[2] = z + depth;
        output_stl_tri(&facet);
        facet.vy[0] = y + height;
        facet.vz[1] = z + depth;
        output_stl_tri(&facet);
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

    fprintf(stderr, "cubes %d faces %d\n", cubes, stl_faces);

}

void output_flat_stl(uint8_t *bm, uint32_t width, uint32_t height, uint8_t transparent)
{
    struct stl_facets facets;

    memset(&facets, 0 , sizeof(struct stl_facets));

    printf("solid png2stl_Model\n");

    stl_gen_facets(&facets, bm, width, height, transparent);

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
