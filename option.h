/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * output options.
 */

#ifndef PNG23D_OPTIONS_H
#define PNG23D_OPTIONS_H 1

enum output_type {
    OUTPUT_PGM,
    OUTPUT_SCAD,
    OUTPUT_STL,
    OUTPUT_ASTL,
};

typedef struct options {
    enum output_type type; /* the type of output to produce */

    unsigned int transparent; /* the grey level value at which object is transparent */
    unsigned int levels; /* the number of levels to quantise into below transparent */

    char *infile; /* input filename */
    char *outfile; /* output filename */

    float width; /* the target width */
    float height; /* the target height */
    float depth; /* the target depth */

} options;

#endif
