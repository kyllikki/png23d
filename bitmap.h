/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * bitmap format.
 */

#ifndef PNG23D_BITMAP_H
#define PNG23D_BITMAP_H 1

/** 8bpp greyscale bitmap representation of image */
typedef struct bitmap {
    uint8_t *data; /**< bitmap data */
    uint32_t width; /**< width of data */
    uint32_t height; /**< height of data */
} bitmap;

bitmap *create_bitmap(const char *filename);

void free_bitmap(bitmap *bm);

#endif
