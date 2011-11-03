/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * mesh mathermatical helper routines header.
 */

#ifndef PNG23D_MESH_MATH_H
#define PNG23D_MESH_MATH_H 1

/* are two points the same location */
static inline bool
eqpnt(struct pnt *p0, struct pnt *p1)
{
    if ((p0->x == p1->x) &&
        (p0->y == p1->y) &&
        (p0->z == p1->z)) {
        return true;
    } else {
        return false;
    }
}

/* are two points different locations */
static inline bool 
nepnt(struct pnt *p0, struct pnt *p1)
{
    if ((p0->x != p1->x) ||
        (p0->y != p1->y) ||
        (p0->z != p1->z)) {
        return true;
    } else {
        return false;
    }
}

static inline int 
dot_product(pnt *a, pnt *b)
{
    return ((a->x * b->x) + (a->y * b->y) + (a->z * b->z));
}

static inline void 
cross_product(pnt *n, pnt *a, pnt *b)
{
    n->x = a->y * b->z - a->z * b->y;
    n->y = a->z * b->x - a->x * b->z;
    n->z = a->x * b->y - a->y * b->x;
}

/* calculate the surface normal from three points
*
* @return true if triangle degenerate else false;
*/
static inline bool
pnt_normal(pnt *n, pnt *v0, pnt *v1, pnt *v2)
{
    pnt a;
    pnt b;

    /* normal calculation
     * va = v1 - v0
     * vb = v2 - v0
     *
     * n = va x vb (cross product)
     */
    a.x = v1->x - v0->x;
    a.y = v1->y - v0->y;
    a.z = v1->z - v0->z;

    b.x = v2->x - v0->x;
    b.y = v2->y - v0->y;
    b.z = v2->z - v0->z;

    n->x = a.y * b.z - a.z * b.y;
    n->y = a.z * b.x - a.x * b.z;
    n->z = a.x * b.y - a.y * b.x;

    /* check for degenerate triangle */
    if ((n->x == 0.0) &&
        (n->y == 0.0) &&
        (n->z == 0.0)) {
        return true;
    }

    return false;
}

/** check if two vectors are parallel and the same sign magnitude 
 */
static inline bool 
same_normal(pnt *n1, pnt *n2)
{
    pnt cn;

    /* check normal is still same sign magnitude */
    if (dot_product(n1, n2) < 0) {
        return false;
    }

    /* check normals remain parallel */
    cross_product(&cn, n1, n2);
    if (cn.x != 0.0 || cn.y != 0.0 || cn.z != 0.0) {
        /* not parallel */
        return false;
    }

    return true;
}

#endif
