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

bool mesh_gen_squares(struct mesh *mesh, bitmap *bm, options *options);
bool mesh_gen_cubes(struct mesh *mesh, bitmap *bm, options *options);
bool mesh_gen_surface(struct mesh *mesh, bitmap *bm, options *options);

#endif
