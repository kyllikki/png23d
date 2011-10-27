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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "option.h"
#include "bitmap.h"
#include "out_pgm.h"
#include "out_scad.h"
#include "out_pscad.h"
#include "out_stl.h"


int main(int argc, char **argv)
{
    bool ret;
    bitmap *bm;
    options *options;
    int fd = STDOUT_FILENO;

    options = read_options(argc, argv);
    if (options == NULL) {
        return EXIT_FAILURE;        
    }

    /* read input */
    INFO("Reading from png file \"%s\"\n", options->infile);
    bm = create_bitmap(options->infile);
    if (bm == NULL) {
        fprintf(stderr, "Error creating bitmap\n");
        return EXIT_FAILURE;
    }

    /* open output */
    INFO("Writing output to \"%s\"\n", options->outfile);
    if (strcmp(options->outfile, "-") != 0) {
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
        INFO("Generating PGM\n");
        ret = output_pgm(bm, fd, options);
        break;

    case OUTPUT_SCAD:
        INFO("Generating scad cubes\n");
        ret = output_flat_scad_cubes(bm, fd, options);
        break;

    case OUTPUT_PSCAD:
        INFO("Generating scad ploygon\n");
        ret = output_flat_scad_polyhedron(bm, fd, options);
        break;

    case OUTPUT_STL:
        INFO("Generating binary STL\n");
        ret = output_flat_stl(bm, fd, options);
        break;

    case OUTPUT_ASTL:
        INFO("Generating ASCII STL\n");
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

    INFO("Completed in %llds\n", 
         (long long)(time(NULL) - options->start_time));

    return 0;
}
