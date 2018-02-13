/*
 * File:   ArchDescription.h
 * Author: s0803652
 *
 * Created on 27 September 2011, 12:03
 */

#ifndef _ARCHDESCRIPTION_H
#define _ARCHDESCRIPTION_H

#include <ostream>
#include <vector>
#include <map>
#include <list>
#include <set>

#include "clean_defines.h"

#include "ArchComponents.h"
#include "DiagnosticContext.h"
#include "RegisterFile.h"
#include "ArchFeatures.h"

namespace gensim
{

	namespace isa
	{
		class ISADescription;
	}

	namespace uarch
	{
		class UArchDescription;
	}

	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
		}
	}

	namespace arch
	{

		class ArchDescription
		{
		public:
			std::string Name;

			bool big_endian;
			uint32_t wordsize;

			std::vector<std::string> IncludeFiles;
			std::vector<std::string> IncludeDirs;

			typedef std::list<isa::ISADescription *> ISAListType;

			ISAListType ISAs;
			gensim::uarch::UArchDescription *Uarch;

			std::string GetIncludes() const;

			ArchDescription();
			virtual ~ArchDescription();

			const isa::ISADescription *GetIsaByName(const std::string isaname) const;
			isa::ISADescription *GetIsaByName(const std::string isaname);

			RegisterFile &GetRegFile()
			{
				return register_file;
			}
			const RegisterFile &GetRegFile() const
			{
				return register_file;
			}

			ArchFeatureSet &GetFeatures()
			{
				return _features;
			}
			const ArchFeatureSet &GetFeatures() const
			{
				return _features;
			}

			bool PrettyPrint(std::ostream &) const;

		private:
			// disallow copy
			ArchDescription(const ArchDescription &orig);
			ArchDescription &operator=(const ArchDescription &orig);

			RegisterFile register_file;
			ArchFeatureSet _features;
		};
	}  // namespace arch
}  // namespace gensim

#endif /* _ARCHDESCRIPTION_H */
