/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * Routines to output in PGM format
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "option.h"
#include "bitmap.h"
#include "out_pgm.h"

bool output_pgm(bitmap *bm, int fd, options *options)
{
    unsigned int row_loop;
    unsigned int col_loop;
    unsigned int div;
    uint8_t pixel;
    FILE *outf;

    outf = fdopen(dup(fd), "w");

    div = options->transparent / options->levels;

    fprintf(outf, "P2\n# test output\n%u %u\n255\n", bm->width, bm->height);
    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            pixel = bm->data[(row_loop * bm->width) + col_loop];
            if (pixel < options->transparent) { 
                fprintf(outf, "%d ", pixel / div);
            } else {
                /* white is used as transparent in pgm output */
                fprintf(outf, "255 ");
            }
        }
        fprintf(outf, "\n");
    }

    fclose(outf);

    return true;
}
