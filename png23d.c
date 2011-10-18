/* png23d.c
 *
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * convert png to 3d file
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "option.h"

#include "bitmap.h"
#include "out_pgm.h"
#include "out_scad.h"
#include "out_stl.h"

static options *
set_options(int argc, char **argv)
{
    int opt;
    options *options;

    options = calloc(1, sizeof(struct options));

    options->type = OUTPUT_ASTL;
    options->finish = FINISH_RAW;
    options->transparent = 128;
    options->levels = 1;
    options->width = 0.0;
    options->height = 0.0;
    options->depth = 1.0;

    while ((opt = getopt(argc, argv, "vf:w:d:h:l:q:t:")) != -1) {
        switch (opt) {

        case 'f': /* output finish */
            if (strcmp(optarg, "raw") == 0) {
                options->finish = FINISH_RAW;
            } else if (strcmp(optarg, "smooth") == 0) {
                options->finish = FINISH_SMOOTH;
            } else {
                fprintf(stderr, "Unknown output finish %s\n", optarg);
                exit(EXIT_FAILURE);
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
            } else if (strcmp(optarg, "pscad") == 0) {
                options->type = OUTPUT_PSCAD;
            } else if (strcmp(optarg, "stl") == 0) {
                options->type = OUTPUT_STL;
            } else if (strcmp(optarg, "astl") == 0) {
                options->type = OUTPUT_ASTL;
            } else {
                fprintf(stderr, "Unknown output type %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;

        case 'v':
                fprintf(stderr, "Version 1.0\n");
                exit(EXIT_SUCCESS);


        default: /* '?' */
            fprintf(stderr,
                    "Usage: %s [-l transparent] [-q levels] [-t output type] infile outfile\n"
                    "          infile The input file\n"
                    "          outfile The output file or - for stdout\n"
                    "          -t     output type. One of pgm, scad, stl, astl\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* files */
    if ((optind +1) >= argc) {
        fprintf(stderr, "input and output files must be specified\n");
            exit(EXIT_FAILURE);
    }
    options->infile = strdup(argv[optind]);
    options->outfile = strdup(argv[optind + 1]);

    return options;
}

int main(int argc, char **argv)
{
    bool ret;
    bitmap *bm;
    options *options;
    int fd;

    options = set_options(argc, argv);

    /* read input */
    bm = create_bitmap(options->infile);
    if (bm == NULL) {
        fprintf(stderr, "Error creating bitmap\n");
        return EXIT_FAILURE;
    }

    /* open output */
    if (strcmp(options->outfile, "-") == 0) {
        fd = STDOUT_FILENO;
    } else {
        fd = open(options->outfile, 
                  O_WRONLY | O_CREAT | O_TRUNC, 
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    }

    if (fd < 0) {
        fprintf(stderr, "Error opening output\n");
        free_bitmap(bm);
        return EXIT_FAILURE;
    }


    /* if user did not specify output dimensions assume those from the bitmap */
    if (options->width == 0) {
        options->width = bm->width;
    }

    if (options->height == 0) {
        options->height = bm->height;
    }

    /* generate output */
    switch (options->type) {
    case OUTPUT_PGM:
        ret = output_pgm(bm, fd, options);
        break;

    case OUTPUT_SCAD:
        ret = output_flat_scad_cubes(bm, fd, options);
        break;

    case OUTPUT_PSCAD:
        ret = output_flat_scad_polyhedron(bm, fd, options);
        break;

    case OUTPUT_STL:
        ret = output_flat_stl(bm, fd, options);
        break;

    case OUTPUT_ASTL:
        ret = output_flat_astl(bm, fd, options);
        break;

    default:
        ret = false;
        break;

    }

    free_bitmap(bm);

    close(fd);

    if (ret != true) {
        fprintf(stderr, "Error generating output\n");
        return EXIT_FAILURE;
    }

    return 0;
}
