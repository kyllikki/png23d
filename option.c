/* png23d.c
 *
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Released under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * command line option handling.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "option.h"

options *
read_options(int argc, char **argv)
{
    int opt;
    options *options;

    options = calloc(1, sizeof(struct options));
    if (options == NULL) {
        return NULL;
    }

    /* keep record of start time */
    options->start_time = time(NULL);

    /* default values */
    options->type = OUTPUT_ASTL;
    options->finish = FINISH_SMOOTH;
    options->optimise = 1;
    options->transparent = 255;
    options->levels = 1;
    options->width = 0.0;
    options->height = 0.0;
    options->depth = 1.0;
    options->bloom_complexity = 2;

    /* parse comamndline options */
    while ((opt = getopt(argc, argv, "Vvf:w:d:h:m:t:l:o:O:b:")) != -1) {
        switch (opt) {

        case 't': /* transparent colour */
            if (*optarg == 'x') {
                options->transparent = 256; /* disabled */
            } else {
                options->transparent = strtoul(optarg, NULL, 0);
                if (options->transparent > 255) {
                    fprintf(stderr,
                            "transparent level must be between 0 and 255\n");
                    goto read_options_error;
                }
            }
            break;

        case 'l': /* quantisation levels */
            options->levels = strtoul(optarg, NULL, 0);
            if (options->levels > 256) {
                fprintf(stderr, "quantisation levels cannot exceed 256\n");
                goto read_options_error;
            }
            break;

        case 'w': /* output width */
            options->width = strtof(optarg, NULL);
            break;

        case 'h': /* output height */
            options->height = strtof(optarg, NULL);
            break;

        case 'd': /* output depth */
            options->depth = strtof(optarg, NULL);
            break;

        case 'o': /* output type */
            if (strcmp(optarg, "pgm") == 0) {
                options->type = OUTPUT_PGM;
            } else if (strcmp(optarg, "cscad") == 0) {
                options->type = OUTPUT_SCAD;
            } else if (strcmp(optarg, "pscad") == 0) {
                options->type = OUTPUT_PSCAD;
            } else if (strcmp(optarg, "stl") == 0) {
                options->type = OUTPUT_STL;
            } else if (strcmp(optarg, "astl") == 0) {
                options->type = OUTPUT_ASTL;
            } else {
                fprintf(stderr, "Unknown output type %s\n", optarg);
                goto read_options_error;
            }
            break;

        case 'f': /* output finish */
            if (strcmp(optarg, "raw") == 0) {
                options->finish = FINISH_RAW;
            } else if (strcmp(optarg, "smooth") == 0) {
                options->finish = FINISH_SMOOTH;
            } else {
                fprintf(stderr, "Unknown output finish %s\n", optarg);
                goto read_options_error;
            }
            break;

        case 'O': /* optimisation level */
            options->optimise = strtoul(optarg, NULL,0);
            break;

        case 'b': /* bloom filter complexity */
            options->bloom_complexity = strtoul(optarg, NULL, 0);
            if (options->bloom_complexity > 16) {
                fprintf(stderr, "bloom complexity must be between 0 and 16\n");
                goto read_options_error;
            }
            break;

        case 'm': /* mesh debug output filename */
            options->meshdebug = strdup(optarg);
            break;

        case 'V':
            fprintf(stderr, "png23d version %d.%02d\n", 
                    VERSION / 100, VERSION % 100);
                exit(EXIT_SUCCESS);

        case 'v':
            options->verbose = true;
            break;


        default: /* '?' */
            goto read_options_error;
        }
    }

    /* files */
    if ((optind +1) >= argc) {
        fprintf(stderr, "input and output files must be specified\n");
        goto read_options_error;
    }
    options->infile = strdup(argv[optind]);
    options->outfile = strdup(argv[optind + 1]);

    return options;

read_options_error:
    fprintf(stderr,
            "Usage: png23d [-t transparent] [-V] [-v] [-f finish] [-O optimisation]\n"
            "              [-w width] [-h height] [-d depth] [-l levels] [-o outtype]\n"
            "              [-b complexity] [-m filename] infile outfile\n\n"
            "\tinfile\tThe input file\n"
            "\toutfile\tThe output file or - for stdout\n"
            "\t-l\tNumber of levels to quantise the heightmap into.\n"
            "\t-o\tThe output file type. One of pgm, bscad, pscad, stl, astl\n");

    free(options);
    return NULL;
}
