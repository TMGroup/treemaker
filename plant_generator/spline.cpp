/* Copyright 2017-2018 Floris Creyf
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

#include "spline.h"
#include "math/curve.h"

using pg::Vec3;

bool pg::Spline::operator==(const pg::Spline &spline) const
{
	return controls == spline.controls && degree == spline.degree;
}

bool pg::Spline::operator!=(const pg::Spline &spline) const
{
	return controls != spline.controls || degree != spline.degree;
}

void pg::Spline::setControls(std::vector<Vec3> controls)
{
	this->controls = controls;
}

std::vector<Vec3> pg::Spline::getControls() const
{
	return controls;
}

int pg::Spline::getSize() const
{
	return controls.size();
}

int pg::Spline::getCurveCount() const
{
	return controls.size() == 0 ? 0 : (controls.size() - 1) / degree;
}

void pg::Spline::setDegree(int degree)
{
	this->degree = degree;
}

int pg::Spline::getDegree() const
{
	return degree;
}

Vec3 pg::Spline::getPoint(float t) const
{
	Vec3 point;
	for (size_t i = 0; i < controls.size()-degree; i += degree) {
		if (controls[i].x <= t && controls[i+degree].x >= t) {
			t -= controls[i].x;
			t /= (controls[i+degree].x - controls[i].x);
			point = getBezier(t, &controls[i], (degree + 1));
			break;
		}
	}
	return point;
}

Vec3 pg::Spline::getPoint(int curve, float t) const
{
	int index = degree * curve;
	return getBezier(t, &controls[index], (degree + 1));
}

Vec3 pg::Spline::getDirection(unsigned index)
{
	if (index == controls.size() - 1)
		return pg::normalize(controls[index] - controls[index - 1]);
	else
		return pg::normalize(controls[index + 1] - controls[index]);
}

int pg::Spline::insert(unsigned index, Vec3 point)
{
	if (degree == 1) {
		controls.insert(controls.begin() + index + 1, point);
		return index + 1;
	} else if (degree == 3) {
		return insertCubic(index, point);
	} else
		return 0;
}

int pg::Spline::insertCubic(int index, Vec3 point)
{
	Vec3 curve[3];
	int center;

	if (index % 3 == 0) {
		if (index == (int)controls.size() - 1) {
			Vec3 d = controls[index-1] - controls[index];
			curve[0] = -1.0f * d + point;
			curve[1] = d + point;
			curve[2] = point;
			center = index + 3;
			++index;
		} else {
			Vec3 d;
			if (index == 0)
				d = -1.0f * controls[index];
			else
				d = controls[index-1] - controls[index];
			curve[1] = point;
			d = controls[index+1] - controls[index];
			curve[2] = d + point;
			curve[0] = -1.0f * d + point;
			center = index + 3;
			index += 2;
		}
	} else if (index % 3 == 1) {
		if (index == 1) {
			Vec3 d = controls[index] - controls[index-1];
			curve[0] = -1.0f * d + point;
			curve[1] = point;
			curve[2] = d + point;
			center = index + 2;
			++index;
		} else {
			Vec3 d = controls[index-1] - controls[index];
			curve[0] = d + point;
			curve[1] = point;
			d = controls[index-1] - controls[index-2];
			curve[2] = d + point;
			center = index + 2;
			++index;
		}
	} else if (index % 3 == 2) {
		if (index == (int)controls.size() - 2) {
			Vec3 d = controls[index+1] - controls[index];
			curve[0] = -1.0f * d + point;
			curve[1] = point;
			curve[2] = d + point;
			center = index + 1;
		} else {
			Vec3 d = controls[index] - controls[index+1];
			curve[0] = d + point;
			curve[1] = point;
			d = controls[index+2] - controls[index+1];
			curve[2] = d + point;
			center = index + 1;
		}
	}

	controls.insert(controls.begin() + index, curve, &curve[3]);
	return center;
}

void pg::Spline::remove(unsigned index)
{
	std::vector<Vec3> copy;

	if (degree == 1)
		controls.erase(controls.begin() + index);
	else if (degree == 3) {
		if (controls.size() == 4) {
			controls.clear();
		} else if (index < 2) {
			controls.erase(controls.begin(), controls.begin() + 2);
		} else if (index > controls.size() - 3) {
			controls.erase(controls.end() - 3, controls.end());
		} else {
			if (index % 3 == 0)
				index -= 1;
			else if (index % 3 == 1)
				index -= 2;
			auto first = controls.begin() + index;
			auto last = controls.begin() + index + 3;
			controls.erase(first, last);
		}
	}
}

void pg::Spline::adjust(int degree)
{
	if (degree == 1) {
		for (size_t i = 0; i + 3 < controls.size(); i++)
			controls.erase(
				controls.begin() + 1 + i,
				controls.begin() + 3 + i);
	} else if (degree == 3) {
		{
			Vec3 direction = (controls[1] - controls[0]);
			float t = magnitude(controls[1] - controls[0]) / 4.0f;
			Vec3 point = controls[0] + t * direction;
			controls.insert(controls.begin() + 1, point);
		}

		for (size_t i = 2; i < controls.size()-1; i += 3) {
			Vec3 d1 = controls[i-2] - controls[i];
			Vec3 d2 = controls[i] - controls[i+1];
			float t1 = magnitude(d1) / 4.0f;
			float t2 = -magnitude(d2) / 4.0f;
			Vec3 direction = normalize(d1 + d2);

			Vec3 points[2];
			points[0] = controls[i] + t1 * direction;
			points[1] = controls[i] + t2 * direction;
			controls.insert(controls.begin() + i + 1, points[1]);
			controls.insert(controls.begin() + i, points[0]);
		}

		{
			int l = controls.size() - 1;
			Vec3 direction = normalize(controls[l] - controls[l-1]);
			float t = magnitude(controls[l] - controls[l-2]) / 4.0f;
			Vec3 point = controls[l] - t * direction;
			controls.insert(controls.end() - 1, point);
		}
	}

	this->degree = degree;
}

void pg::Spline::move(unsigned index, Vec3 location, bool parallel)
{
	if (degree == 3) {
		if (index % 3 == 0) {
			if (index != 0) {
				controls[index-1] -= controls[index];
				controls[index-1] += location;
			}
			if (index != controls.size() - 1) {
				controls[index+1] -= controls[index];
				controls[index+1] += location;
			}
			controls[index] = location;
		} else if (index % 3 == 1) {
			controls[index] = location;
			if (parallel)
				parallelize(index);
		} else if (index % 3 == 2) {
			controls[index] = location;
			if (parallel)
				parallelize(index);
		}
	} else
		controls[index] = location;
}

void pg::Spline::parallelize(unsigned index)
{
	if (degree == 3) {
		if (index % 3 && index > 1) {
			Vec3 d = controls[index-1] - controls[index-2];
			float m = magnitude(d);
			if (m == 0.0f)
				m = 1.0f;
			d = controls[index-1] - controls[index];
			/* Arbitrary default value because zero vectors
			 * cannot be normalized. */
			if (isZero(d))
				d.z = 1.0f;
			d = m * normalize(d) + controls[index-1];
			controls[index-2] = d;
		} else if (index % 3 && index < controls.size() - 2) {
			Vec3 d = controls[index+2] - controls[index+1];
			float m = magnitude(d);
			if (m == 0.0f)
				m = 1.0f;
			d = controls[index+1] - controls[index];
			if (isZero(d))
				d.z = 1.0f;
			d = m * normalize(d) + controls[index+1];
			controls[index+2] = d;
		}
	}
}

void pg::Spline::clear()
{
	controls.clear();
}
