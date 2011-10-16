#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "option.h"
#include "bitmap.h"
#include "out_pgm.h"

bool output_pgm(bitmap *bm, options *options)
{
    int row_loop;
    int col_loop;
    unsigned int div;

    div = options->transparent / options->levels;

    printf("P2\n# test output\n%u %u\n255\n", bm->width, bm->height);
    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            if (bm->data[(row_loop * bm->width) + col_loop] < options->transparent) { 
                printf("%d ", (bm->data[(row_loop * bm->width) + col_loop]) / div);
            } else {
                /* white is used as transparent in pgm output */
                printf("255 ");
            }
        }
        printf("\n");
    }

    return true;
}
