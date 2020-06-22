/* Plant Generator
 * Copyright (C) 2019  Floris Creyf
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

#include "mesh_editor.h"
#include "plant_generator/file.h"

MeshEditor::MeshEditor(
	SharedResources *shared, Editor *editor, QWidget *parent) :
	ObjectEditor(parent)
{
	this->shared = shared;
	this->editor = editor;
	setFocusPolicy(Qt::StrongFocus);

	this->meshViewer = new MeshViewer(shared, this);
	this->meshViewer->setMinimumHeight(200);
	this->layout->addWidget(this->meshViewer);
	connect(this->meshViewer, SIGNAL(ready()), this, SLOT(select()));

	QVBoxLayout *buttonLayout = new QVBoxLayout();
	buttonLayout->setMargin(5);
	buttonLayout->setSpacing(2);
	initFields(buttonLayout);
	this->layout->addLayout(buttonLayout);
	this->layout->addStretch(1);
}

QSize MeshEditor::sizeHint() const
{
	return QSize(350, 200);
}

void MeshEditor::initFields(QVBoxLayout *layout)
{
	this->customButton = new QPushButton("Custom", this);
	this->customButton->setFixedHeight(22);
	this->planeButton = new QPushButton("Plane", this);
	this->planeButton->setFixedHeight(22);
	this->perpPlaneButton = new QPushButton("Perpendicular Planes", this);
	this->perpPlaneButton->setFixedHeight(22);
	this->emptyButton = new QPushButton("Empty", this);
	this->emptyButton->setFixedHeight(22);

	QString style = tr("text-align:left;padding:0 5px;");
	this->customButton->setStyleSheet(style);
	this->planeButton->setStyleSheet(style);
	this->perpPlaneButton->setStyleSheet(style);
	this->emptyButton->setStyleSheet(style);

	layout->addWidget(this->customButton);
	layout->addWidget(this->planeButton);
	layout->addWidget(this->perpPlaneButton);
	layout->addWidget(this->emptyButton);

	connect(this->customButton, SIGNAL(clicked()),
		this, SLOT(loadCustom()));
	connect(this->planeButton, SIGNAL(clicked()),
		this, SLOT(loadPlane()));
	connect(this->perpPlaneButton, SIGNAL(clicked()),
		this, SLOT(loadPerpPlane()));
	connect(this->emptyButton, SIGNAL(clicked()),
		this, SLOT(loadEmpty()));
}

void MeshEditor::clear()
{
	pg::Plant *plant = this->editor->getPlant();
	int count = this->selectionBox->count();
	while (count > 0) {
		plant->removeLeafMesh(count-1);
		emit meshRemoved(count-1);
		count--;
	}
	this->selectionBox->clear();
}

const MeshViewer *MeshEditor::getViewer() const
{
	return this->meshViewer;
}

void MeshEditor::init(std::vector<pg::Geometry> geometry)
{
	clear();
	this->selectionBox->blockSignals(true);
	for (pg::Geometry geom : geometry) {
		QString name = QString::fromStdString(geom.getName());
		this->selectionBox->addItem(name);
		emit meshAdded(geom);
	}
	this->selectionBox->blockSignals(false);
	select();
}

void MeshEditor::add()
{
	pg::Geometry geom;
	std::string name;
	QString qname;

	for (int i = 1; true; i++) {
		name = "Mesh " + std::to_string(i);
		qname = QString::fromStdString(name);
		if (this->selectionBox->findText(qname) == -1)
			break;
	}

	geom.setName(name);
	geom.setPerpendicularPlanes();
	add(geom);
}

void MeshEditor::add(pg::Geometry geom)
{
	pg::Plant *plant = this->editor->getPlant();
	plant->addLeafMesh(geom);
	this->selectionBox->blockSignals(true);
	QString name = QString::fromStdString(geom.getName());
	this->selectionBox->addItem(name);
	this->selectionBox->setCurrentIndex(this->selectionBox->findText(name));
	this->selectionBox->blockSignals(false);
	this->meshViewer->updateMesh(geom);
	emit meshAdded(geom);
}

void MeshEditor::loadCustom()
{
	if (this->selectionBox->count() > 0) {
		QString filename = QFileDialog::getOpenFileName(
			this, tr("Open File"), "", tr("Wavefront OBJ (*.obj)"));

		if (!filename.isNull()) {
			pg::Plant *plant = this->editor->getPlant();
			unsigned index = this->selectionBox->currentIndex();
			pg::Geometry geom = plant->getLeafMesh(index);
			pg::File file;
			std::string s = filename.toStdString();
			file.importObj(s.c_str(), &geom);
			modify(geom, index);
		}
	}
}

void MeshEditor::loadPlane()
{
	if (this->selectionBox->count() > 0) {
		pg::Plant *plant = editor->getPlant();
		unsigned index = this->selectionBox->currentIndex();
		pg::Geometry geom = plant->getLeafMesh(index);
		geom.setPlane();
		modify(geom, index);
	}
}

void MeshEditor::loadPerpPlane()
{
	if (this->selectionBox->count() > 0) {
		pg::Plant *plant = this->editor->getPlant();
		unsigned index = this->selectionBox->currentIndex();
		pg::Geometry geom = plant->getLeafMesh(index);
		geom.setPerpendicularPlanes();
		modify(geom, index);
	}
}

void MeshEditor::loadEmpty()
{
	if (this->selectionBox->count() > 0) {
		pg::Plant *plant = this->editor->getPlant();
		unsigned index = this->selectionBox->currentIndex();
		pg::Geometry geom = plant->getLeafMesh(index);
		geom.clear();
		modify(geom, index);
	}
}

void MeshEditor::modify(pg::Geometry &geom, unsigned index)
{
	pg::Plant *plant = this->editor->getPlant();
	plant->updateLeafMesh(geom, index);
	this->meshViewer->updateMesh(geom);
	emit meshModified(geom, index);
	this->editor->change();
}

void MeshEditor::select()
{
	if (this->selectionBox->count()) {
		pg::Plant *plant = editor->getPlant();
		unsigned index = this->selectionBox->currentIndex();
		pg::Geometry geom = plant->getLeafMesh(index);
		this->meshViewer->updateMesh(geom);
	}
}

void MeshEditor::rename()
{
	unsigned index = this->selectionBox->currentIndex();
	QString name = this->selectionBox->itemText(index);
	pg::Plant *plant = this->editor->getPlant();
	pg::Geometry geom = plant->getLeafMesh(index);
	geom.setName(name.toStdString());
	plant->updateLeafMesh(geom, index);
	emit meshModified(geom, index);
}

void MeshEditor::remove()
{
	 if (this->selectionBox->count() > 1) {
		unsigned index = this->selectionBox->currentIndex();
		QString name = this->selectionBox->currentText();
		pg::Plant *plant = this->editor->getPlant();
		plant->removeLeafMesh(index);
		this->selectionBox->removeItem(index);
		select();
		this->editor->change();
		emit meshRemoved(index);
	}
}
