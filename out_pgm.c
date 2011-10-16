#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "bitmap.h"
#include "out_pgm.h"

bool output_pgm(bitmap *bm, uint8_t transparent)
{
    int row_loop;
    int col_loop;

    printf("P2\n# test output\n%u %u\n255\n", bm->width, bm->height);
    for (row_loop = 0; row_loop < bm->height; row_loop++) {
        for (col_loop = 0; col_loop < bm->width; col_loop++) {
            printf("%d ",bm->data[(row_loop * bm->width) + col_loop]);
        }
        printf("\n");
    }
}
