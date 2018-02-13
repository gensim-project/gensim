/*
 * ArchFeatures.h
 *
 *  Created on: 12 Jun 2016
 *      Author: harry
 */

#ifndef INC_ARCH_ARCHFEATURES_H_
#define INC_ARCH_ARCHFEATURES_H_

#include <cstdint>

#include <string>
#include <vector>

namespace gensim
{

	namespace arch
	{

		class ArchFeature
		{
		public:
			ArchFeature(std::string name, uint32_t id);

			const std::string &GetName() const
			{
				return _name;
			}
			uint32_t GetId() const
			{
				return _id;
			}

			void SetDefaultLevel(uint32_t l)
			{
				_default_level = l;
			}
			uint32_t GetDefaultLevel() const
			{
				return _default_level;
			}

		private:
			std::string _name;
			uint32_t _id;
			uint32_t _default_level;
		};

		class ArchFeatureSet
		{
		public:
			ArchFeatureSet();
			typedef std::vector<ArchFeature> featureset_t;

			bool AddFeature(const std::string &);

			const ArchFeature *GetFeature(const std::string &) const;
			const ArchFeature *GetFeature(uint32_t) const;

			ArchFeature *GetFeature(const std::string &);
			ArchFeature *GetFeature(uint32_t);

			featureset_t::const_iterator begin() const
			{
				return _features.begin();
			}
			featureset_t::const_iterator end() const
			{
				return _features.end();
			}

		private:
			uint32_t _max_id;
			featureset_t _features;
		};

	}

}



#endif /* INC_ARCH_ARCHFEATURES_H_ */
