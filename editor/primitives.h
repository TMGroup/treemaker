/* 
 * Copyright (C) 2016 Floris Creyf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "objects.h"
#include "collision.h"

Line createGrid(int sections);
Line createBox(bt_aabb &b);

#endif /* PRIMITIVES_H */
