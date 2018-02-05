/* Plant Genererator
 * Copyright (C) 2018  Floris Creyf
 *
 * TreeMaker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TreeMaker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "buffer.h"

using std::vector;

void Buffer::initialize(GLenum mode)
{
	initializeOpenGLFunctions();
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, buffers);
	this->mode = mode;
}

void Buffer::allocatePointMemory(int size)
{
	capacity[Points] = size;
	size *= sizeof(float);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[Points]);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, mode);
	setVertexFormat();
}

void Buffer::allocateIndexMemory(int size)
{
	capacity[Indices] = size;
	size *= sizeof(unsigned);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, mode);
}

void Buffer::load(const Geometry &geometry)
{
	const vector<float> *p = geometry.getPoints();
	const vector<unsigned> *i = geometry.getIndices();
	load(p->data(), p->size(), i->data(), i->size());
}

void Buffer::load(const float *points, int psize, const unsigned *indices,
	int isize)
{
	size[Points] = capacity[Points] = psize;
	psize *= sizeof(float);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[Points]);
	glBufferData(GL_ARRAY_BUFFER, psize, points, mode);
	setVertexFormat();

	if (isize > 0) {
		size[Indices] = capacity[Indices] = isize;
		isize *= sizeof(unsigned);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, isize, indices, mode);
	}
}

void Buffer::update(const Geometry &geometry)
{
	const vector<float> *p = geometry.getPoints();
	const vector<unsigned> *i = geometry.getIndices();
	update(p->data(), p->size(), i->data(), i->size());
}

void Buffer::update(const float *points, int psize, const unsigned *indices,
	int isize)
{
	use();
	size[Points] = psize;
	size[Indices] = isize;
	psize *= sizeof(float);
	isize *= sizeof(unsigned);

	if (psize > capacity[Points])
		allocatePointMemory(psize * 2);
	else
		glBindBuffer(GL_ARRAY_BUFFER, buffers[Points]);

	glBufferSubData(GL_ARRAY_BUFFER, 0, psize, points);

	if (indices != nullptr) {
		if (isize > capacity[Indices])
			allocateIndexMemory(isize * 2);
		else
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Indices]);

		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, isize, indices);
	}
}

void Buffer::setVertexFormat()
{
	GLsizei stride = sizeof(float) * 6;
	GLvoid *ptr = (GLvoid *)(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	glEnableVertexAttribArray(0);
	ptr = (GLvoid *)(sizeof(float) * 3);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	glEnableVertexAttribArray(1);
}

void Buffer::use()
{
	glBindVertexArray(vao);
}

int Buffer::getSize(int type)
{
	return size[type];
}
