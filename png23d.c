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
#include <unistd.h>

#include "option.h"

#include "bitmap.h"
#include "out_pgm.h"
#include "out_scad.h"
#include "out_stl.h"

options *
set_options(int argc, char **argv)
{
    int opt;
    options *options;

    options = calloc(1, sizeof(options));

    options->type = OUTPUT_ASTL;
    options->transparent = 128;
    options->levels = 1;
    options->width = 0;
    options->height = 0;
    options->depth = 1;

    while ((opt = getopt(argc, argv, "w:d:h:l:q:t:")) != -1) {
        switch (opt) {

        case 'w': /* output width */
            options->width = strtof(optarg, NULL);
            break;

        case 'h': /* output height */
            options->height = strtof(optarg, NULL);
            break;

        case 'd': /* output depth */
            options->depth = strtof(optarg, NULL);
            break;

        case 'l': /* transparent level */
            options->transparent = strtoul(optarg, NULL, 0);
            if (options->transparent > 255) {
                fprintf(stderr, "transparent level must be between 0 and 255\n");
                exit(EXIT_FAILURE);
                
            }
            break;

        case 'q': /* quantisation levels */
            options->levels = strtoul(optarg, NULL, 0);
            if (options->levels > options->transparent) {
                fprintf(stderr, "quantisation levels cannot exceed transparent level\n");
                exit(EXIT_FAILURE);
                
            }
            break;

        case 't': /* output type */
            if (strcmp(optarg, "pgm") == 0) {
                options->type = OUTPUT_PGM;
            } else if (strcmp(optarg, "scad") == 0) {
                options->type = OUTPUT_SCAD;
            } else if (strcmp(optarg, "stl") == 0) {
                options->type = OUTPUT_STL;
            } else if (strcmp(optarg, "astl") == 0) {
                options->type = OUTPUT_ASTL;
            } else {
                fprintf(stderr, "Unknown output type %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;

        default: /* '?' */
            fprintf(stderr, 
                    "Usage: %s [-l transparent] [-q levels] [-t output type] infile [outfile]\n"
                    "          -t     output type. One of pgm, scad, stl, astl\n", 
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "input file must be specified\n");
            exit(EXIT_FAILURE);
    }
    options->infile = strdup(argv[optind]);

    return options;
}

int main(int argc, char **argv)
{
    bool ret;
    bitmap *bm;
    options *options;

    options = set_options(argc, argv);

    bm = create_bitmap(options->infile);
    if (bm == NULL) {
        return EXIT_FAILURE;
    }

    /* if user did not specify output dimensions assume those from the bitmap */
    if (options->width == 0) {
        options->width = bm->width;
    }

    if (options->height == 0) {
        options->height = bm->height;
    }


    switch (options->type) {
    case OUTPUT_PGM:
        ret = output_pgm(bm, options);
        break;

    case OUTPUT_SCAD:
        ret = output_flat_scad_cubes(bm, options);
        break;

    case OUTPUT_STL:
        ret = output_flat_stl(bm, options);
        break;

    case OUTPUT_ASTL:
        ret = output_flat_astl(bm, options);
        break;

    default:
        ret = false;
        break;

    }

    if (ret != true) {
        fprintf(stderr, "Error generating output\n");
        return EXIT_FAILURE;        
    }

    return 0;
}
