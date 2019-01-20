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

#include "mesh.h"
#include <cmath>
#include <limits>

using pg::Vec3;
using pg::Mat4;

pg::Mesh::Mesh(pg::Plant *plant)
{
	this->plant = plant;
}

void pg::Mesh::generate()
{
	Stem *stem = plant->getRoot();
	segments.clear();
	vertices.clear();
	indices.clear();
	meshes.clear();
	materials.clear();
	mesh = 0;
	addStem(stem, 0);

#ifndef PG_NO_UNIFIED_INDICES
	/* Assumes that all meshes will be stored in the same buffer in the
	 * following order. */
	if (!indices.empty()) {
		unsigned vsize = vertices[0].size();
		unsigned isize = indices[0].size();
		unsigned ioffset = vsize / vertexSize;
		for (unsigned mesh = 1; mesh < indices.size(); mesh++) {
			for (unsigned &index : indices[mesh])
				index += ioffset;
			for (Segment &s : segments[mesh]) {
				s.vertexStart += vsize;
				s.indexStart += isize;
			}
			vsize += vertices[mesh].size();
			isize += indices[mesh].size();
			ioffset = vsize / vertexSize;
		}
	}
#endif
}

std::vector<float> pg::Mesh::getVertices() const
{
	std::vector<float> object;
	for (auto it = vertices.begin(); it != vertices.end(); it++)
		object.insert(object.end(), it->begin(), it->end());
	return object;
}

std::vector<unsigned> pg::Mesh::getIndices() const
{
	std::vector<unsigned> object;
	for (auto it = indices.begin(); it != indices.end(); it++)
		object.insert(object.end(), it->begin(), it->end());
	return object;
}

const std::vector<float> *pg::Mesh::getVertices(int mesh) const
{
	return &vertices[mesh];
}

const std::vector<unsigned> *pg::Mesh::getIndices(int mesh) const
{
	return &indices[mesh];
}

const std::vector<pg::Segment> *pg::Mesh::getSegments(int mesh) const
{
	return &segments[mesh];
}

pg::Segment pg::Mesh::find(pg::Stem *stem) const
{
	Segment segment = {0, 0, 0, 0, 0};
	for (size_t i = 0; i < segments.size(); i++) {
		for (size_t j = 0; j < segments[i].size(); j++) {
			if (segments[i][j].stem == stem) {
				segment = segments[i][j];
				break;
			}
		}
	}
	return segment;
}

Mat4 pg::Mesh::getSectionTransform(Stem *stem, size_t section, float offset)
{
	Vec3 up = {0.0f, 1.0f, 0.0f};
	Vec3 point = stem->getPath().get(section);
	Vec3 direction = stem->getPath().getAverageDirection(section);
	Vec3 location = stem->getLocation() + point + offset * direction;
	Mat4 rotation = pg::rotateIntoVec(up, direction);
	Mat4 translation = pg::translate(location);
	return translation * rotation;
}

void pg::Mesh::addSection(Stem *stem, size_t section, float &textureOffset,
	float pathOffset)
{
	Mat4 transform = getSectionTransform(stem, section, pathOffset);
	float radius = stem->getPath().getRadius(section);
	float length = stem->getPath().getIntermediateDistance(section);
	const float rotation = 2.0f * M_PI / stem->getResolution();
	const float uSegment = 1.0f / stem->getResolution();
	float angle = 0.0f;
	Vec2 uv = {1.0f, -length/(radius * 2.0f * (float)M_PI) + textureOffset};
	textureOffset = uv.y;

#ifdef PG_NO_VERTEX_SEAM
	for (int i = 0; i < stem->getResolution(); i++) {
#else
	for (int i = 0; i <= stem->getResolution(); i++) {
#endif
		Vec3 point = {std::cos(angle), 0.0f, std::sin(angle)};
		Vec3 normal = pg::normalize(point);
		point = radius * point;

		point = pg::toVec3(transform * pg::toVec4(point, 1.0f));
		normal = pg::toVec3(transform * pg::toVec4(normal, 0.0f));
		normal = pg::normalize(normal);
		addPoint(point, normal, uv);
		uv.x -= uSegment;
		angle += rotation;
	}
}

void pg::Mesh::addRectangle(size_t *s1, size_t *s2)
{
	indices[mesh].push_back(*s2);
	indices[mesh].push_back(++(*s2));
	indices[mesh].push_back(*s1);

	indices[mesh].push_back(*s1);
	indices[mesh].push_back(*s2);
	indices[mesh].push_back(++(*s1));
}

void pg::Mesh::addLastRectangle(size_t a, size_t b, size_t iA, size_t iB)
{
	addTriangle(iB, iA, a);
	addTriangle(a, b, iB);
}

void pg::Mesh::addTriangleRing(size_t csA, size_t csB, int divisions)
{
#ifdef PG_NO_VERTEX_SEAM
	size_t initCsA = csA;
	size_t initCsB = csB;
#endif

	for (int i = 0; i <= divisions - 1; i++)
		addRectangle(&csA, &csB);

#ifdef PG_NO_VERTEX_SEAM
	addLastRectangle(csA, csB, initCsA, initCsB);
#endif
}

void pg::Mesh::addRectangles(size_t csA, size_t csB, int start, int end,
	int divisions)
{
	size_t initCsA = csA;
	size_t initCsB = csB;
	int i;

	for (i = 0; i < divisions-1; i++)
		if (i >= start && i <= end)
			addRectangle(&csA, &csB);
		else {
			csA++;
			csB++;
		}

	if (i >= start && i <= end) {
		csA = initCsA + divisions - 1;
		csB = initCsB + divisions - 1;
		addLastRectangle(csA, csB, initCsA, initCsB);
	}
}

void pg::Mesh::capStem(Stem *stem, int stemMesh, size_t section)
{
	int divisions = stem->getResolution();
	int index;

#ifndef PG_NO_VERTEX_SEAM
	const float rotation = 2.0f * M_PI / divisions;
	float angle = 0.0f;
	index = section;
	section = vertices[mesh].size();
	for (int i = 0; i <= divisions; i++) {
		Vec3 point;
		Vec3 normal;
		Vec2 tex;
		point.x = vertices[stemMesh][index];
		point.y = vertices[stemMesh][index+1];
		point.z = vertices[stemMesh][index+2];
		normal.x = vertices[stemMesh][index+3];
		normal.y = vertices[stemMesh][index+4];
		normal.z = vertices[stemMesh][index+5];
		tex.x = std::cos(angle) * 0.5f + 0.5f;
		tex.y = std::sin(angle) * 0.5f + 0.5f;
		angle += rotation;
		addPoint(point, normal, tex);
		index += vertexSize;
	}
#endif /* PG_NO_VERTEX_SEAM */

	size_t vertex = section / vertexSize;
	for (index = 0; index < divisions/2 - 1; index++) {
		indices[mesh].push_back(vertex + index);
		indices[mesh].push_back(vertex + divisions - index - 1);
		indices[mesh].push_back(vertex + index + 1);

		indices[mesh].push_back(vertex + index + 1);
		indices[mesh].push_back(vertex + divisions - index - 1);
		indices[mesh].push_back(vertex + divisions - index - 2);
	}

	if ((divisions & 1) != 0 /* is odd */) {
		indices[mesh].push_back(vertex + index);
		indices[mesh].push_back(vertex + index + 2);
		indices[mesh].push_back(vertex + index + 1);
	}
}

void pg::Mesh::addStem(Stem *stem, int mesh, float pathOffset)
{
	if (std::isnan(stem->getLocation().x))
		return;

	mesh = selectBuffer(stem->getMaterial(Stem::Outer), mesh);
	this->mesh = mesh;

	float textureOffset = 0.0f;
	pg::Segment bufferSegment;
	bufferSegment.stem = stem;
	bufferSegment.vertexStart = vertices[mesh].size();
	bufferSegment.indexStart = indices[mesh].size();

	{ /* first cross section */
		size_t origIndex = vertices[mesh].size() / vertexSize;
		addSection(stem, 0, textureOffset, pathOffset);
		size_t currentIndex = vertices[mesh].size() / vertexSize;
		addTriangleRing(origIndex, currentIndex, stem->getResolution());
	}

	for (int i = 1; i < stem->getPath().getSize() - 1; i++) {
		size_t origIndex = vertices[mesh].size() / vertexSize;
		addSection(stem, i, textureOffset);
		size_t currentIndex = vertices[mesh].size() / vertexSize;
		addTriangleRing(origIndex, currentIndex, stem->getResolution());
	}

	{ /* last cross section */
		size_t lastSection = vertices[mesh].size();
		int i = stem->getPath().getSize() - 1;
		addSection(stem, i, textureOffset);

		int capMesh = stem->getMaterial(Stem::Inner);
		this->mesh = selectBuffer(capMesh, mesh);
		capStem(stem, mesh, lastSection);
	}

	bufferSegment.vertexCount = vertices[mesh].size();
	bufferSegment.vertexCount -= bufferSegment.vertexStart;
	bufferSegment.indexCount = indices[mesh].size();
	bufferSegment.indexCount -= bufferSegment.indexStart;
	segments[mesh].push_back(bufferSegment);

	addLeaves(stem);

	{
		Stem *child = stem->getChild();
		while (child != nullptr) {
			addStem(child, 0.0f);
			child = child->getSibling();
		}
	}
}

void pg::Mesh::addLeaves(Stem *stem)
{
	for (int i = 0; i < stem->getLeafCount(); i++) {
		Leaf *leaf = stem->getLeaf(i);
		VolumetricPath path = stem->getPath();
		Vec3 normal = {0.0f, 1.0f, 0.0f};
		Vec3 p[4];

		{
			pg::Vec2 s = leaf->getScale();
			p[0] = {0.5f * s.x, 0.0f, 0.0f};
			p[1] = {0.5f * s.x, 0.0f, 1.0f * s.y};
			p[2] = {-0.5f * s.x, 0.0f, 1.0f * s.y};
			p[3] = {-0.5f * s.x, 0.0f, 0.0f};
		}

		{
			Mat4 m = rotateXY(leaf->getTilt(), 0.0f);
			p[1] = pg::toVec3(m * pg::toVec4(p[1], 1.0f));
			p[2] = pg::toVec3(m * pg::toVec4(p[2], 1.0f));
		}

		Vec3 location = stem->getLocation();
		float position = leaf->getPosition();
		if (position >= 0.0f && position < path.getLength()) {
			Vec3 dir = path.getIntermediateDirection(position);
			rotateSideLeaf(p, normal, dir);
			location += path.getIntermediate(position);
		} else {
			Vec3 dir = path.getDirection(path.getSize() - 1);
			rotateEndLeaf(p, normal, dir);
			location += path.get().back();
		}

		for (int i = 0; i < 4; i++)
			p[i] += location;

		size_t index = vertices[mesh].size() / vertexSize;
		addPoint(p[0], normal, {0.0f, 0.0f});
		addPoint(p[1], normal, {0.0f, 0.0f});
		addPoint(p[2], normal, {0.0f, 0.0f});
		addPoint(p[3], normal, {0.0f, 0.0f});
		addTriangle(index, index + 1, index + 3);
		addTriangle(index + 1, index + 2, index + 3);
	}
}

void pg::Mesh::rotateSideLeaf(Vec3 (&p)[4], Vec3 &normal, Vec3 direction)
{
	Vec3 leafDirection = pg::normalize(pg::cross(direction, normal));
	Vec3 d = {0.0f, 0.0f, 1.0f};
	Mat4 m = pg::rotateIntoVec(d, leafDirection);

	for (int i = 0; i < 4; i++)
		p[i] = pg::toVec3(m * pg::toVec4(p[i], 1.0f));

	{
		Vec3 up = {0.0f, -1.0f, 0.0f};
		Vec3 d = pg::cross(up, direction);
		d = pg::cross(d, direction);
		d = pg::normalize(d);
		Mat4 m = pg::rotateIntoVec(normal, d);
		for (int i = 0; i < 4; i++)
			p[i] = pg::toVec3(m * pg::toVec4(p[i], 1.0f));
		normal = d;
	}
}

void pg::Mesh::rotateEndLeaf(Vec3 (&p)[4], Vec3 &normal, Vec3 direction)
{
	Vec3 d = {0.0f, 0.0f, 1.0f};
	Mat4 m = pg::rotateIntoVec(d, direction);

	normal = pg::toVec3(m * pg::toVec4(normal, 1.0f));
	for (int i = 0; i < 4; i++)
		p[i] = pg::toVec3(m * pg::toVec4(p[i], 1.0f));

	{
		Vec3 up = {0.0f, 1.0f, 0.0f};
		Vec3 leafDirection = pg::cross(direction, up);
		leafDirection = pg::cross(leafDirection, direction);
		leafDirection = pg::normalize(leafDirection);
		Mat4 m = pg::rotateIntoVec(normal, leafDirection);
		for (int i = 0; i < 4; i++)
			p[i] = pg::toVec3(m * pg::toVec4(p[i], 1.0f));
		normal = leafDirection;
	}
}

void pg::Mesh::addPoint(Vec3 point, Vec3 normal, Vec2 texture)
{
	vertices[mesh].push_back(point.x);
	vertices[mesh].push_back(point.y);
	vertices[mesh].push_back(point.z);
	vertices[mesh].push_back(normal.x);
	vertices[mesh].push_back(normal.y);
	vertices[mesh].push_back(normal.z);
	vertices[mesh].push_back(texture.x);
	vertices[mesh].push_back(texture.y);
}

void pg::Mesh::addTriangle(int a, int b, int c)
{
	indices[mesh].push_back(a);
	indices[mesh].push_back(b);
	indices[mesh].push_back(c);
}

int pg::Mesh::selectBuffer(int material, int mesh)
{
	if (material != 0) {
		auto it = meshes.find(material);
		if (it == meshes.end()) {
			vertices.push_back(std::vector<float>());
			indices.push_back(std::vector<unsigned>());
			segments.push_back(std::vector<Segment>());
			meshes[material] = indices.size() - 1;
			materials.push_back(material);
		}
		mesh = meshes[material];
	} else if (materials.empty()) {
		materials.push_back(0);
		vertices.push_back(std::vector<float>());
		indices.push_back(std::vector<unsigned>());
		segments.push_back(std::vector<Segment>());
	}
	return mesh;
}

int pg::Mesh::getMeshCount() const
{
	return indices.size();
}

int pg::Mesh::getVertexCount() const
{
	int size = 0;
	for (auto &mesh : vertices)
		size += mesh.size();
	return size;
}

int pg::Mesh::getIndexCount() const
{
	int size = 0;
	for (auto &mesh : indices)
		size += mesh.size();
	return size;
}

unsigned pg::Mesh::getMaterialId(int mesh)
{
	return materials.at(mesh);
}
