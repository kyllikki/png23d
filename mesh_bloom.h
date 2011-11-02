/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * mesh generation bloom filter.
 */

#ifndef PNG23D_MESH_BLOOM_H
#define PNG23D_MESH_BLOOM_H 1

/** Initialise bloom filter */
bool mesh_bloom_init(struct mesh *mesh, unsigned int entries, unsigned int iterations);

/** Add vertex to indexed list */
idxvtx mesh_add_pnt(struct mesh *mesh, struct pnt *npnt);

#endif

