/* Plant Generator
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

#include "axes.h"

Axes::Axes() : scale(1.0f), position(), selection(None)
{

}

void Axes::setScale(float scale)
{
	this->scale = scale;
}

void Axes::setPosition(pg::Vec3 position)
{
	this->position = position;
}

pg::Vec3 Axes::getPosition()
{
	return position;
}

void Axes::selectCenter()
{
	selection = Center;
}

Axes::Axis Axes::getSelection()
{
	return selection;
}

void Axes::setAxis(Axis axis)
{
	this->selection = axis;
}

void Axes::clearSelection()
{
	selection = None;
}
