/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "uarch/uArch.h"
#include "uarch/cache/CacheGeometry.h"

using namespace archsim::uarch;

bool uArch::Initialise()
{
	cache_geometry = new cache::CacheGeometry();
	return true;
}

void uArch::Destroy()
{
	delete cache_geometry;
}

void uArch::PrintStatistics(std::ostream& stream)
{
	stream << "uArch Statistics" << std::endl;

	cache_geometry->PrintStatistics(stream);
}
