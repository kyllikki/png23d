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
#include <string.h>

#include "bitmap.h"
#include "out_pgm.h"
#include "out_scad.h"
#include "out_stl.h"

int main(int argc, char **argv)
{
    bool ret;
    bitmap bm;

    ret = create_gs_bitmap(argv[1], &bm);
    if (ret == false) {
        return 1;
    }

    output_pgm(&bm, 210);
    output_flat_scad_cubes(&bm, 210);
    output_flat_stl(&bm, 255);

    return 0;
}
