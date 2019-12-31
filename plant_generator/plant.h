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

#ifndef PG_PLANT_H
#define PG_PLANT_H

#include "stem.h"
#include "material.h"
#include "geometry.h"
#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace pg {
	class Plant {
		friend class boost::serialization::access;

		Stem *root;
		std::map<long, Material> materials;
		std::map<long, Geometry> leafMeshes;

		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			(void)version;
			ar & root;
			ar & materials;
			ar & leafMeshes;
		}

		void removeMaterial(Stem *, long);

	public:
		Plant();
		~Plant();

		/** Add a new stem to the plant at a parent stem. */
		Stem *addStem(Stem *parent);
		/** Insert a stem into the plant at parent. */
		void insertStem(Stem *stem, Stem *parent);
		/** Remove all stems from the plant and create a new root. */
		Stem *createRoot();
		/** Remove a stem from the plant. */
		Stem *extractStem(Stem *);
		/** Return the root or trunk of the plant. */
		Stem *getRoot();
		/** Remove all stems in the plant. */
		void removeRoot();
		/** Remove a stem and its descendants from the plant. */
		void removeStem(Stem *stem);

		void addMaterial(Material material);
		void removeMaterial(long id);
		Material getMaterial(long id);
		std::map<long, Material> getMaterials();

		void addLeafMesh(Geometry mesh);
		void removeLeafMesh(long id);
		void removeLeafMeshes();
		Geometry getLeafMesh(long id);
		std::map<long, Geometry> getLeafMeshes();
	};
}

#endif /* PG_PLANT_H */
