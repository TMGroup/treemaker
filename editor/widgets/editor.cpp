/* Plant Genererator
 * Copyright (C) 2016-2018  Floris Creyf
 *
 * Plant Genererator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.,
 *
 * Plant Genererator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "editor.h"
#include "../closest.h"
#include "../geometry/geometry.h"
#include <cmath>
#include <QtOpenGL/QGLFormat>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

using pg::Vec3;
using pg::Mat4;
using pg::Ray;

Editor::Editor(SharedResources *shared, QWidget *parent) :
	QOpenGLWidget(parent), mesh(&plant), generator(&plant)
{
	this->shared = shared;
	path.setColor({0.4f, 0.1f, 0.6f}, {0.1f, 0.4f, 0.6f});
	setFocus();
}

void Editor::initializeGL()
{
	initializeOpenGLFunctions();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(Geometry::primitiveReset);

	shared->initialize();
	initializeBuffers();
	generator.grow();
	change();
}

void Editor::initializeBuffers()
{
	Geometry geometry;
	Geometry axesLines = axes.getLines();
	Geometry axesArrows = axes.getArrows();
	Geometry rotationLines = rotationAxes.getLines();

	geometry.addGrid(5, {0.41, 0.41, 0.41}, {0.3, 0.3, 0.3});
	scene.grid = geometry.getSegment();
	scene.axesLines = geometry.append(axesLines);
	scene.axesArrows = geometry.append(axesArrows);
	scene.rotation = geometry.append(rotationLines);

	staticBuffer.initialize(GL_STATIC_DRAW);
	staticBuffer.load(geometry);

	plantBuffer.initialize(GL_DYNAMIC_DRAW);
	plantBuffer.allocatePointMemory(65536);
	plantBuffer.allocateIndexMemory(65536);

	pathBuffer.initialize(GL_DYNAMIC_DRAW);
	pathBuffer.allocatePointMemory(100);
	pathBuffer.allocateIndexMemory(100);
}

void Editor::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Control:
		ctrl = true;
		break;
	case Qt::Key_Shift:
		shift = true;
		break;
	case Qt::Key_C:
		if (mode == Rotate)
			rotationAxes.selectAxis(Axes::Center);
		break;
	case Qt::Key_E:
		if (mode == None || mode == MovePoint) {
			mode = MovePoint;
			extrude();
		}
		break;
	case Qt::Key_R:

		if (selectedStem && selectedPoint >= 0) {
			int d = selectedStem->getPath().getSpline().getDegree();
			if (d == 3 && selectedPoint % 3 == 0)
				mode = Rotate;
			else if (d == 1)
				mode = Rotate;
		}
		setMouseTracking(true);
		update();
		break;
	case Qt::Key_X:
		if (mode == Rotate)
			rotationAxes.selectAxis(Axes::XAxis);
		break;
	case Qt::Key_Y:
		if (mode == Rotate)
			rotationAxes.selectAxis(Axes::YAxis);
		break;
	case Qt::Key_Z:
		if (mode == Rotate)
			rotationAxes.selectAxis(Axes::ZAxis);
		break;
	case Qt::Key_Delete:
		if (selectedStem) {
			mode = None;
			removePoint();
		}
		break;
	}
}

void Editor::keyReleaseEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Control:
		ctrl = false;
		break;
	case Qt::Key_Shift:
		shift = false;
		break;
	case Qt::Key_Tab:
		break;
	}
}

bool Editor::event(QEvent *e)
{
	if (e->type() == QEvent::KeyPress) {
		QKeyEvent *k = static_cast<QKeyEvent *>(e);
		keyPressEvent(k);
		return true;
	}
	return QWidget::event(e);
}

void Editor::mousePressEvent(QMouseEvent *event)
{
	QPoint p = event->pos();
	midButton = false;

	if (mode == Rotate) {
		mode = None;
		rotationAxes.selectCenter();
		setMouseTracking(false);
		update();
	} else if (event->button() == Qt::RightButton) {
		if (mode == None || mode == MovePoint) {
			mode = None;
			pg::Stem *prevStem = selectedStem;

			if (selectedStem)
				selectPoint(p.x(), p.y());
			if (selectedPoint == -1)
				selectStem(p.x(), p.y());
			if (selectedStem != prevStem)
				selectedPoint = -1;

			update();
		}
	} if (event->button() == Qt::MidButton) {
		midButton = true;
		camera.setStartCoordinates(p.x(), p.y());
		if (ctrl && !shift)
			camera.setAction(Camera::Zoom);
		else if (shift && !ctrl)
			camera.setAction(Camera::Pan);
		else
			camera.setAction(Camera::Rotate);
	} else if (event->button() == Qt::LeftButton) {
		if (selectedPoint >= 0 && selectedStem)
			selectAxis(p.x(), p.y());
		else if (selectedStem) {
			selectedPoint = 1;
			mode = MovePoint;
			axes.selectCenter();
			addStem(p.x(), p.y());
			emit selectionChanged();
		}
	}

	setFocus();
}

void Editor::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MidButton)
		camera.setAction(Camera::None);
	if (mode == MovePoint && event->button() == Qt::LeftButton) {
		axes.clearSelection();
		mode = None;
	}
}

void Editor::mouseMoveEvent(QMouseEvent *event)
{
	QPoint point = event->pos();
	camera.executeAction(point.x(), point.y());

	if (mode == MovePoint && axes.getSelection() != Axes::Axis::None)
		movePoint(point.x(), point.y());
	else if (mode == Rotate)
		rotateStem(point.x(), point.y());
	update();
}

void Editor::extrude()
{
	pg::Vec3 location = selectedStem->getLocation();
	pg::Vec3 point = plant.extrude(selectedStem, &selectedPoint);
	point = point + location;
	QPoint p = mapFromGlobal(QCursor::pos());
	setClickOffset(p.x(), p.y(), point);
	axes.selectCenter();
	setMouseTracking(true);
	emit selectionChanged();
}

void Editor::removePoint()
{
	if (selectedStem->getParent()) {
		selectedStem = plant.removePoint(selectedStem, &selectedPoint);
		emit selectionChanged();
		change();
	}
}

void Editor::addStem(int x, int y)
{
	if (selectedStem) {
		pg::Ray ray;
		ray.origin = camera.getPosition();
		ray.direction = camera.getRayDirection(x, y);
		std::vector<pg::Vec3> path = selectedStem->getPath().get();
		for (size_t i = 0; i < path.size(); i++)
			path[i] = path[i] + selectedStem->getLocation();
		float t = closestDistance(path, camera, x, y);
		float radius = selectedStem->getPath().getMaxRadius()/4;
		selectedStem = plant.addStem(selectedStem);
		axes.setPosition(plant.initializeStem(selectedStem, ray,
			camera.getDirection(), t, radius));
		clickOffset[0] = 0;
		clickOffset[1] = 0;
		change();
		update();
	}
}

void Editor::selectStem(int x, int y)
{
	Ray ray;
	ray.direction = camera.getRayDirection(x, y);
	ray.origin = camera.getPosition();
	selectedStem = plant.getStem(ray);
	updateSelection();
	setMouseTracking(false);
	emit selectionChanged();
}

void Editor::selectPoint(int x, int y)
{
	pg::Vec3 location = selectedStem->getLocation();
	pg::Path path = selectedStem->getPath();
	pg::Spline spline = path.getSpline();
	auto controls = spline.getControls();
	int size = controls.size();
	selectedPoint = -1;

	for (int i = 0; i < size; i++) {
		Vec3 point = controls[i] + location;
		axes.setPosition(point);
		rotationAxes.setPosition(point);
		point = camera.toScreenSpace(point);
		float sx = std::pow(point.x - x, 2);
		float sy = std::pow(point.y - y, 2);

		if (std::sqrt(sx + sy) < 10) {
			selectedPoint = i;
			break;
		}
	}

	setMouseTracking(false);
}

void Editor::selectAxis(int x, int y)
{
	Vec3 o = camera.getPosition();
	Vec3 d = camera.getRayDirection(x, y);
	setClickOffset(x, y, axes.getPosition());
	axes.selectAxis({o, d});
	mode = axes.getSelection() == Axes::Axis::None ? None : MovePoint;
}

void Editor::movePoint(int x, int y)
{
	x += clickOffset[0];
	y += clickOffset[1];

	Vec3 direction = camera.getDirection();
	Ray ray = {camera.getPosition(), camera.getRayDirection(x, y)};
	pg::Stem *parentStem = nullptr;

	if (selectedStem)
		parentStem = selectedStem->getParent();

	if (parentStem && selectedPoint == 0) {
		/* Move stem along the parent path if the first point is
		 * moved. */
		std::vector<pg::Vec3> path = parentStem->getPath().get();
		for (size_t i = 0; i < path.size(); i++)
			path[i] = path[i] + parentStem->getLocation();
		float t = closestDistance(path, camera, x, y);
		selectedStem->setPosition(t);
		axes.setPosition(selectedStem->getLocation());
	} else if (selectedPoint != 0) {
		pg::Vec3 location = selectedStem->getLocation();
		pg::VolumetricPath vpath = selectedStem->getPath();
		pg::Spline spline = vpath.getSpline();
		location = axes.move(ray, direction) - location;
		spline.move(selectedPoint, location);
		vpath.setSpline(spline);
		selectedStem->setPath(vpath);
	}

	change();
}

void Editor::rotateStem(int x, int y)
{
	pg::Vec3 cameraDirection = camera.getDirection();
	pg::Ray ray = {camera.getPosition(), camera.getRayDirection(x, y)};
	pg::VolumetricPath path = selectedStem->getPath();
	pg::Spline spline = path.getSpline();
	pg::Vec3 direction = spline.getDirection(selectedPoint);
	pg::Mat4 t = rotationAxes.rotate(ray, cameraDirection, direction);
	plant.rotate(selectedStem, selectedPoint, t);
	change();
}

void Editor::setClickOffset(int x, int y, Vec3 point)
{
	Vec3 s = camera.toScreenSpace(point);
	clickOffset[0] = s.x - x;
	clickOffset[1] = s.y - y;
}

pg::Plant *Editor::getPlant()
{
	return &plant;
}

pg::Stem *Editor::getSelectedStem()
{
	return selectedStem;
}

const pg::Mesh *Editor::getMesh()
{
	return &mesh;
}

void Editor::resizeGL(int width, int height)
{
	float ratio = static_cast<float>(width) / static_cast<float>(height);
	camera.setWindowSize(width, height);
	camera.setPerspective(45.0f, 0.1f, 100.0f, ratio);
	axes.setScale(600.0f / height);
	glViewport(0, 0, width, height);
	paintGL();
}

void Editor::paintGL()
{
	Mat4 vp = camera.getVP();
	Vec3 cp = camera.getPosition();

	glDepthFunc(GL_LEQUAL);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* paint grid */
	staticBuffer.use();
	glUseProgram(shared->getShader(Shader::Flat));
	glUniformMatrix4fv(0, 1, GL_FALSE, &vp[0][0]);
	glDrawArrays(GL_LINES, scene.grid.pstart, scene.grid.pcount);

	/* paint plant */
	plantBuffer.use();
	glUseProgram(shared->getShader(Shader::Model));
	glUniformMatrix4fv(0, 1, GL_FALSE, &vp[0][0]);
	glUniform4f(1, cp.x, cp.y, cp.z, 0.0f);

	{
		GLvoid *start = 0;
		GLsizei count = plantBuffer.getSize(Buffer::Indices);
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, start);
	}

	glUseProgram(shared->getShader(Shader::Wire));
	glUniformMatrix4fv(0, 1, GL_FALSE, &vp[0][0]);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	{
		GLvoid *s = (GLvoid *)(scene.selection.istart *
			sizeof(unsigned));
		GLsizei c = scene.selection.icount;
		glDrawElements(GL_TRIANGLES, c, GL_UNSIGNED_INT, s);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (selectedStem) {
		/* paint path lines */
		glDepthFunc(GL_ALWAYS);
		glPointSize(8);

		pathBuffer.use();
		glUseProgram(shared->getShader(Shader::Line));
		glUniformMatrix4fv(0, 1, GL_FALSE, &vp[0][0]);
		glUniform2f(1, QWidget::width(), QWidget::height());

		{
			Geometry::Segment segment = path.getLineSegment();
			GLvoid *start = (GLvoid *)(segment.istart *
				sizeof(unsigned));
			glDrawElements(GL_LINE_STRIP, segment.icount,
				GL_UNSIGNED_INT, start);
		}

		auto texture = shared->getTexture(shared->DotTexture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUseProgram(shared->getShader(Shader::Point));
		glUniformMatrix4fv(0, 1, GL_FALSE, &vp[0][0]);

		{
			Geometry::Segment s = path.getPointSegment();
			glDrawArrays(GL_POINTS, s.pstart, s.pcount);
		}

		if (selectedPoint >= 0)
			paintAxes();
	}

	glFlush();
}

void Editor::paintAxes()
{
	Mat4 vp = camera.getVP();
	Vec3 cp = camera.getPosition();
	GLvoid *s;
	GLsizei c;

	/* paint axes */
	glClear(GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(shared->getShader(Shader::Flat));
	staticBuffer.use();

	if (mode == Rotate) {
		pg::Spline spline = selectedStem->getPath().getSpline();
		pg::Vec3 d = spline.getDirection(selectedPoint);
		pg::Mat4 m = vp * rotationAxes.getTransformation(cp, d);
		glUniformMatrix4fv(0, 1, GL_FALSE, &m[0][0]);

		s = (GLvoid *)((scene.rotation.istart) * sizeof(unsigned));
		c = scene.rotation.icount;
		glDrawElements(GL_LINE_STRIP, c, GL_UNSIGNED_INT, s);
	} else {
		pg::Mat4 m = vp * axes.getTransformation(cp);
		glUniformMatrix4fv(0, 1, GL_FALSE, &m[0][0]);
		glDrawArrays(GL_LINES, scene.axesLines.pstart,
			scene.axesLines.pcount);

		s = (GLvoid *)((scene.axesArrows.istart) * sizeof(unsigned));
		c = scene.axesArrows.icount;
		glDrawElements(GL_TRIANGLES, c, GL_UNSIGNED_INT, s);
	}
}

void Editor::updateSelection()
{
	pg::Segment selection = mesh.find(selectedStem);
	scene.selection.pstart = selection.vertexStart;
	scene.selection.pcount = selection.vertexCount;
	scene.selection.istart = selection.indexStart;
	scene.selection.icount = selection.indexCount;

	if (selectedStem) {
		pg::Vec3 location = selectedStem->getLocation();
		pg::Path spath = selectedStem->getPath();
		pg::Spline spline = spath.getSpline();
		int degree = spline.getDegree();
		std::vector<pg::Vec3> ppoints = spath.get();
		std::vector<pg::Vec3> cpoints = spline.getControls();
		for (size_t i = 0; i < ppoints.size(); i++)
			ppoints[i] = ppoints[i] + location;
		for (size_t i = 0; i < cpoints.size(); i++)
			cpoints[i] = cpoints[i] + location;
		path.set(ppoints, cpoints, degree);
		const Geometry *geometry = path.getGeometry();
		pathBuffer.update(*geometry);
	}
}

void Editor::change()
{
	mesh.generate();
	const std::vector<float> *p = mesh.getVertices();
	const std::vector<unsigned> *i = mesh.getIndices();
	plantBuffer.update(p->data(), p->size(), i->data(), i->size());
	updateSelection();
	update();
}

void Editor::changePathDegree(int i)
{
	int degree = i == 1 ? 3 : 1;
	pg::VolumetricPath path = selectedStem->getPath();
	pg::Spline spline = path.getSpline();
	if (spline.getDegree() != degree) {
		selectedPoint = spline.adjust(degree, selectedPoint);
		pg::Vec3 p = spline.getControls()[selectedPoint];
		p += selectedStem->getLocation();
		axes.setPosition(p);
	}
	path.setSpline(spline);
	selectedStem->setPath(path);
	change();
}

void Editor::changeResolution(int i)
{
	pg::Stem *stem = getSelectedStem();
	stem->setResolution(i);
	change();
}

void Editor::changeDivisions(int i)
{
	pg::Stem *stem = getSelectedStem();
	pg::VolumetricPath vpath = stem->getPath();
	vpath.setResolution(i);
	stem->setPath(vpath);
	change();
}

void Editor::changeRadius(double d)
{
	pg::Stem *stem = getSelectedStem();
	pg::VolumetricPath vpath = stem->getPath();
	vpath.setMaxRadius(d);
	stem->setPath(vpath);
	change();
}

void Editor::changeRadiusCurve(pg::Spline &spline)
{
	pg::Stem *stem = getSelectedStem();
	pg::VolumetricPath vp = stem->getPath();
	vp.setRadius(spline);
	stem->setPath(vp);
	change();
}
