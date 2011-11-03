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

/** add a facet to a vndexed vertex */
bool add_facet_to_vertex(struct mesh *mesh, struct facet *facet, idxvtx ivertex);

/** remove a facet to a vndexed vertex */
bool remove_facet_from_vertex(struct mesh *mesh, struct facet *facet, idxvtx ivertex);

/** update the mesh geometry index representation */
bool index_mesh(struct mesh *mesh, unsigned int bloom_complexity, unsigned int vertex_fcount);

#endif

