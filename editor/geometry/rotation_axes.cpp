/* Plant Genererator
 * Copyright (C) 2018  Floris Creyf
 *
 * Plant Generator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Plant Generator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rotation_axes.h"

Geometry RotationAxes::getLines()
{
	unsigned index = 0;
	float radius = 0.8f;
	unsigned resolution = 20;
	Geometry lines;
	Geometry::Segment segment;

	lines.addCircle(radius, resolution, {0.0f, 1.0f, 0.2f});
	while (index < resolution)
		lines.addIndex(index++);
	lines.addIndex(index - resolution);
	lines.addIndex(Geometry::primitiveReset);
	segment = lines.getSegment();

	lines.addCircle(radius, resolution, {0.0f, 0.2f, 1.0f});
	lines.transform(segment.pcount, segment.pcount, {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	});
	while (index < resolution*2)
		lines.addIndex(index++);
	lines.addIndex(index - resolution);
	lines.addIndex(Geometry::primitiveReset);

	lines.addIndex(Geometry::primitiveReset);
	lines.addCircle(radius, resolution, {1.0f, 0.2f, 0.0f});
	lines.transform(segment.pcount*2, segment.pcount, {
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	});
	while (index < resolution*3)
		lines.addIndex(index++);
	lines.addIndex(index - resolution);

	return lines;
}

void RotationAxes::selectAxis(Axis axis)
{
	this->selection = axis;
}

pg::Mat4 RotationAxes::getTransformation(float distance, pg::Vec3 direction)
{
	float m = 1.0f;
	if (scalable)
		m = distance / 15.0f * scale;
	pg::Mat4 transform = {
		m, 0.0f, 0.0f, 0.0f,
		0.0f, m, 0.0f, 0.0f,
		0.0f, 0.0f, m, 0.0f,
		position.x, position.y, position.z, 1.0f
	};
	return transform * pg::rotateIntoVec({0.0f, 1.0f, 0.0f}, direction);
}
