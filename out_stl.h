/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d. 
 * 
 * STL format output header.
 */

#ifndef PNG23D_OUT_STL_H
#define PNG23D_OUT_STL_H 1

bool output_flat_stl(bitmap *bm, int fd, options *options);
bool output_flat_astl(bitmap *bm, int fd, options *options);

#endif
