/*
 * ProcessorFeatures.h
 *
 *  Created on: 12 Jun 2016
 *      Author: harry
 */

#ifndef GENSIM_PROCESSORFEATURES_H_
#define GENSIM_PROCESSORFEATURES_H_

#include "util/PubSubSync.h"
#include "util/PubSubType.h"

#include <map>


namespace archsim
{

// A class used to pass around information about processor features.
	class ProcessorFeatureSet
	{
	public:
		virtual ~ProcessorFeatureSet() {}

		typedef std::map<uint32_t, uint32_t> feature_level_map_t;

		void AddFeature(uint32_t feature_id)
		{
			_feature_levels[feature_id] = 0;
		}
		uint32_t GetFeatureLevel(uint32_t id) const
		{
			return _feature_levels.at(id);
		}

		// This mask is made up of the first 7 features (we assume features are 8 bit for now)
		// It represents the set of features available on a CPU at a given time.
		// Each feature level is shifted into the appropriate position.
		// The top field should always be 0.
		uint64_t GetAvailableMask() const
		{
			uint64_t mask = 0;
			for(const auto &i : _feature_levels) {
				if(i.first >= 7) continue;
				mask |= i.second << (i.first*8);
			}
			assert((mask & 0xf000000000000000) == 0);
			return mask;
		}

		// This mask is made up of the first 7 features (we assume features are 8 bit for now)
		// It represents the set of features required by a translation, but not what their levels are.
		// For each required feature, its field is set to all 1s. Non required features are set to all 0s.
		// If a feature is required which does not fit, the top field is set to all 1s.
		uint64_t GetRequiredMask() const
		{
			uint64_t mask = 0;
			for(const auto &i : _feature_levels) {
				if(i.first >= 7) {
					mask |= 0xf000000000000000;
				}
				mask |= 0xff << (i.first*8);
			}

			return mask;
		}

		// This mask is made up of the first 7 features (we assume features are 8 bit for now)
		// It represents the levels of the features required by a translation.
		// For each required feature, its level is shifted into the appropriate position.
		// If a feature is required which does not fit, the top field is set to all 1s.
		uint64_t GetLevelMask() const
		{
			uint64_t mask = 0;
			for(const auto &i : _feature_levels) {
				if(i.first >= 7) {
					mask |= 0xf000000000000000;
				}
				mask |= i.second << (i.first*8);
			}

			return mask;
		}

		virtual void SetFeatureLevel(uint32_t id, uint32_t new_level)
		{
			_feature_levels[id] = new_level;
		}

		feature_level_map_t::const_iterator begin() const
		{
			return _feature_levels.begin();
		}
		feature_level_map_t::const_iterator end() const
		{
			return _feature_levels.end();
		}

	private:
		std::map<uint32_t, uint32_t> _feature_levels;
	};

// A class representing the set of features actually attached to a CPU
	class ProcessorFeatureInterface : public ProcessorFeatureSet
	{
	public:
		ProcessorFeatureInterface() = delete;
		ProcessorFeatureInterface(archsim::util::PubSubContext &pubsub) : _pubsub(pubsub) {}

		void AddNamedFeature(const std::string &name, uint32_t id)
		{
			_feature_names[name] = id;
			AddFeature(id);
		}

		void SetFeatureLevel(uint32_t id, uint32_t new_level) override
		{
			if(GetFeatureLevel(id) != new_level) {
				_pubsub.Publish(PubSubType::FeatureChange, 0);
			}

			ProcessorFeatureSet::SetFeatureLevel(id, new_level);
		}

		uint32_t GetFeatureLevelByName(const std::string &name) const
		{
			return GetFeatureLevel(GetFeatureId(name));
		}
		uint32_t GetFeatureId(const std::string &name) const
		{
			return _feature_names.at(name);
		}
		void SetFeatureLevel(const std::string &id, uint32_t new_level)
		{
			SetFeatureLevel(GetFeatureId(id), new_level);
		}


	private:
		std::map<std::string, uint32_t> _feature_names;
		util::PubSubscriber _pubsub;
	};

}



#endif /* GENSIM_PROCESSORFEATURES_H_ */
