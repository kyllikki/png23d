/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * mesh generation routines header.
 */

#ifndef PNG23D_MESH_GEN_H
#define PNG23D_MESH_GEN_H 1

#define FACETPNT_CNT 32

uint32_t
mesh_gen_get_face(bitmap *bm, 
                  unsigned int x, 
                  unsigned int y, 
                  unsigned int z, 
                  struct options *options);


typedef void (meshgenerator)(struct mesh *mesh,
                        float x, float y, float z,
                        float width, float height, float depth,
                          uint32_t faces);


meshgenerator mesh_gen_cube;
meshgenerator mesh_gen_marching_squares;


#endif
