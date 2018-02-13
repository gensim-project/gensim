/*
 * File:   uArch.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 14:45
 */

#ifndef UARCH_H
#define	UARCH_H

#include <ostream>

namespace archsim
{
	namespace uarch
	{
		namespace cache
		{
			class CacheGeometry;
		}

		class uArch
		{
		public:
			bool Initialise();
			void Destroy();

			void PrintStatistics(std::ostream& stream);

			inline cache::CacheGeometry& GetCacheGeometry() const
			{
				return *cache_geometry;
			}

		private:
			cache::CacheGeometry *cache_geometry;
		};
	}
}

#endif	/* UARCH_H */
