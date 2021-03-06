/* Copyright 2017 Floris Creyf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PG_MATH_CURVE_H
#define PG_MATH_CURVE_H

#include "vec3.h"
#include <vector>
#include <cstddef>

namespace pg {
	Vec3 getBezier(float t, const Vec3 *points, int size);
	Vec3 getLinearBezier(float t, Vec3 x, Vec3 y);
	Vec3 getQuadraticBezier(float t, Vec3 x, Vec3 y, Vec3 z);
	Vec3 getCubicBezier(float t, Vec3 x, Vec3 y, Vec3 z, Vec3 w);
}

#endif
