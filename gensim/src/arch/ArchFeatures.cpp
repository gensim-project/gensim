/*
 * ArchFeatures.cpp
 *
 *  Created on: 12 Jun 2016
 *      Author: harry
 */

#include "arch/ArchFeatures.h"

using namespace gensim::arch;

ArchFeature::ArchFeature(std::string name, uint32_t id) : _name(name), _id(id), _default_level(0) {}

ArchFeatureSet::ArchFeatureSet() : _max_id(0) {}

bool ArchFeatureSet::AddFeature(const std::string &str)
{
	if(GetFeature(str)) return false;
	_features.push_back(ArchFeature(str, _max_id++));

	return true;
}

const ArchFeature *ArchFeatureSet::GetFeature(const std::string &str) const
{
	for(const auto &i : _features) if(i.GetName() == str) return &i;
	return nullptr;
}

const ArchFeature *ArchFeatureSet::GetFeature(uint32_t id) const
{
	for(const auto &i : _features) if(i.GetId() == id) return &i;
	return nullptr;
}


ArchFeature *ArchFeatureSet::GetFeature(const std::string &str)
{
	for(auto &i : _features) if(i.GetName() == str) return &i;
	return nullptr;
}

ArchFeature *ArchFeatureSet::GetFeature(uint32_t id)
{
	for(auto &i : _features) if(i.GetId() == id) return &i;
	return nullptr;
}

