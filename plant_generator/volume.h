/* Copyright 2021 Floris Creyf
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

#ifndef PG_VOLUME_H
#define PG_VOLUME_H

#include "math/intersection.h"
#include "math/vec3.h"

namespace pg {
	class Volume {
	public:
		class Node {
			Node *nodes;
			Node *parent;
			int depth;
			Vec3 center;
			float size;

			float density;
			Vec3 direction;
			int quantity;

			Node();
			Node *getAdjacentNode(int, Vec3, bool);

		public:
			~Node();
			Node(Vec3 center, float size);
			Node *getParent();
			Node *getNode(int index);
			Node *getNextNode(int axis, Vec3 point);
			Node *getPreviousNode(int axis, Vec3 point);
			Node *getChildNode(Vec3 point);
			Node *getAdjacentNode(Ray ray);
			Vec3 getCenter() const;
			float getSize() const;
			int getDepth() const;
			void divide();
			void clear();

			void setDensity(float density);
			float getDensity() const;
			void setDirection(Vec3 direction);
			Vec3 getDirection() const;
			void setQuantity(int quantity);
			int getQuantity() const;
		};

		Volume(float size, int depth);
		void clear(float size, int depth);
		Node *addNode(Vec3 point);
		void addLine(Vec3 a, Vec3 b, float weight);
		Node *getNode(Vec3 point);
		Node *getRoot();

	private:
		float size;
		int depth;
		Node root;

		Node *getNode(Vec3 point, Node *node);
	};
}

#endif
