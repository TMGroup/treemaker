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

#include "material_editor.h"
#include "plant_generator/math/math.h"

using pg::Vec3;
using pg::Mat4;

MaterialEditor::MaterialEditor(
	SharedResources *shared, Editor *editor, QWidget *parent) :
	ObjectEditor(parent)
{
	this->shared = shared;
	this->editor = editor;

	this->materialViewer = new MaterialViewer(shared, this);
	this->materialViewer->setMinimumHeight(200);
	this->layout->addWidget(this->materialViewer);

	QFormLayout *form = new QFormLayout();
	form->setMargin(5);
	form->setSpacing(2);
	initFields(form);
	this->layout->addLayout(form);
	this->layout->addStretch(1);
}

QSize MaterialEditor::sizeHint() const
{
	return QSize(350, 200);
}

void MaterialEditor::initFields(QFormLayout *form)
{
	QHBoxLayout *diffuseLayout = new QHBoxLayout();
	this->diffuseBox = new QLineEdit(this);
	this->diffuseBox->setFixedHeight(22);
	this->diffuseBox->setReadOnly(true);
	this->addDiffuseButton = new QPushButton("+", this);
	this->addDiffuseButton->setFixedWidth(22);
	this->addDiffuseButton->setFixedHeight(22);
	this->removeDiffuseButton = new QPushButton("-", this);
	this->removeDiffuseButton->setFixedWidth(22);
	this->removeDiffuseButton->setFixedHeight(22);

	QWidget *sizeWidget = new QWidget();
	sizeWidget->setLayout(diffuseLayout);
	diffuseLayout->setSpacing(0);
	diffuseLayout->setMargin(0);
	diffuseLayout->addWidget(this->diffuseBox);
	diffuseLayout->addWidget(this->removeDiffuseButton);
	diffuseLayout->addWidget(this->addDiffuseButton);
	form->addRow(tr("Diffuse"), sizeWidget);

	form->setFormAlignment(Qt::AlignRight | Qt::AlignTop);
	form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	form->setLabelAlignment(Qt::AlignRight | Qt::AlignCenter);

	connect(this->addDiffuseButton, SIGNAL(clicked()),
		this, SLOT(openDiffuseFile()));
	connect(this->removeDiffuseButton, SIGNAL(clicked()),
		this, SLOT(removeDiffuseFile()));
}

const MaterialViewer *MaterialEditor::getViewer() const
{
	return this->materialViewer;
}

void MaterialEditor::init(const std::vector<pg::Material> &materials)
{
	clear();
	for (pg::Material material : materials) {
		ShaderParams params(material);
		QString name = QString::fromStdString(params.getName());
		this->shared->addMaterial(params);
		this->selectionBox->addItem(name);
		if (!material.getTexture().empty()) {
			QString filename =
				QString::fromStdString(material.getTexture());
			params.loadTexture(0, filename);
		}
	}
	select();
}

/** Add an existing material to the material list. */
void MaterialEditor::add(pg::Material material)
{
	ShaderParams params(material);
	QString name = QString::fromStdString(params.getName());
	this->shared->addMaterial(params);
	this->selectionBox->addItem(name);
	this->selectionBox->setCurrentIndex(this->selectionBox->count() - 1);

	if (!material.getTexture().empty()) {
		QString filename;
		filename = QString::fromStdString(material.getTexture());
		this->diffuseBox->setText(filename);
		params.loadTexture(0, filename);
	}

	this->editor->getPlant()->addMaterial(params.getMaterial());
	this->materialViewer->updateMaterial(params);
}

/** Add an empty material with a unique name to the material list. */
void MaterialEditor::add()
{
	ShaderParams params;
	std::string name;
	QString qname;

	for (int i = 1; true; i++) {
		name = "Material " + std::to_string(i);
		qname = QString::fromStdString(name);
		if (this->selectionBox->findText(qname) == -1)
			break;
	}

	params.setName(name);
	this->shared->addMaterial(params);
	this->editor->getPlant()->addMaterial(params.getMaterial());
	this->materialViewer->updateMaterial(params);

	this->diffuseBox->setText(tr(""));
	this->selectionBox->addItem(qname);
	this->selectionBox->setCurrentIndex(
		this->selectionBox->findText(qname));
}

void MaterialEditor::clear()
{
	pg::Plant *plant = this->editor->getPlant();
	int count = this->selectionBox->count();
	while (count > 0) {
		plant->removeMaterial(count-1);
		count--;
	}
	this->selectionBox->clear();
	this->shared->clearMaterials();
	this->diffuseBox->clear();
}

void MaterialEditor::rename()
{
	unsigned index = this->selectionBox->currentIndex();
	QString value = this->selectionBox->itemText(index);
	ShaderParams params = this->shared->getMaterial(index);
	params.setName(value.toStdString());
	update(params, index);
}

void MaterialEditor::select()
{
	if (this->selectionBox->count()) {
		unsigned index = this->selectionBox->currentIndex();
		ShaderParams params = this->shared->getMaterial(index);
		std::string filename = params.getMaterial().getTexture();
		QString qfilename = QString::fromStdString(filename);
		this->diffuseBox->setText(qfilename);
		this->materialViewer->updateMaterial(params);
	}
}

void MaterialEditor::remove()
{
	if (this->selectionBox->count() > 1) {
		unsigned index = this->selectionBox->currentIndex();
		this->selectionBox->removeItem(index);
		this->shared->removeMaterial(index);
		this->editor->getPlant()->removeMaterial(index);
		this->editor->change();
		select();
	}
}

void MaterialEditor::openDiffuseFile()
{
	unsigned index = this->selectionBox->currentIndex();
	ShaderParams params = this->shared->getMaterial(index);

	QString filename = QFileDialog::getOpenFileName(
		this, tr("Open File"), "",
		tr("All Files (*);;PNG (*.png);;JPEG (*.jpg);;SVG (*.svg)"));

	if (!filename.isNull() || !filename.isEmpty())
		if (params.loadTexture(0, filename)) {
			this->diffuseBox->setText(filename);
			update(params, index);
		}
}

void MaterialEditor::removeDiffuseFile()
{
	unsigned index = this->selectionBox->currentIndex();
	ShaderParams params = this->shared->getMaterial(index);
	params.removeTexture(0);
	this->diffuseBox->clear();
	update(params, index);
}

void MaterialEditor::update(ShaderParams params, unsigned index)
{
	this->shared->updateMaterial(params, index);
	this->editor->getPlant()->updateMaterial(params.getMaterial(), index);
	this->editor->change();
	this->materialViewer->updateMaterial(params);
}
