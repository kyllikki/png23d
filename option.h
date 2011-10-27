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

#include <time.h>

/* Using PRId64 yeilds a compile error if used in string concatination */
#define D64F "%zd"

#define INFO(...) if (options->verbose) printf(__VA_ARGS__) 

enum output_type {
    OUTPUT_PGM,
    OUTPUT_SCAD,
    OUTPUT_PSCAD,
    OUTPUT_STL,
    OUTPUT_ASTL,
};

enum output_finish {
    FINISH_RAW,
    FINISH_SMOOTH,
};

typedef struct options {
    time_t start_time;

    enum output_type type; /* the type of output to produce */

    enum output_finish finish;

    unsigned int optimise; /* amount of mesh optimisation to apply */

    unsigned int transparent; /* the grey level value at which object is transparent */
    unsigned int levels; /* the number of levels to quantise into below transparent */

    unsigned int bloom_complexity; /* The size and number of iterations used
                                    * for the bloom filter
                                    */

    bool verbose; /* make tool verbose about operations */

    char *infile; /* input filename */
    char *outfile; /* output filename */

    float width; /* the target width */
    float height; /* the target height */
    float depth; /* the target depth */

    char *meshdebug; /* filename for mesh debug output */

} options;


options *read_options(int argc, char **argv);


#endif
