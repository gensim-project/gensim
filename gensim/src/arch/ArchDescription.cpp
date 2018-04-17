/*
 * File:   ArchDescription.cpp
 * Author: s0803652
 *
 * Created on 27 September 2011, 12:03
 */

#include <string.h>


#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "UArchDescription.h"

#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ssa/SSAContext.h"

#include <string>
#include <vector>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <fstream>



namespace gensim
{

	namespace arch
	{

		ArchDescription::ArchDescription() : big_endian(false)
		{

		}

		ArchDescription::~ArchDescription()
		{
			for (std::list<isa::ISADescription *>::const_iterator II = ISAs.begin(), IE = ISAs.end(); II != IE; ++II) {
				delete *II;
			}
		}

		const isa::ISADescription *ArchDescription::GetIsaByName(const std::string isaname) const
		{
			for (auto isa : ISAs)
				if (isa->ISAName == isaname) return isa;
			return NULL;
		}
		isa::ISADescription *ArchDescription::GetIsaByName(const std::string isaname)
		{
			for (auto isa : ISAs)
				if (isa->ISAName == isaname) return isa;
			return NULL;
		}

		bool ArchDescription::PrettyPrint(std::ostream &str) const
		{
			str << "Arch " << Name << "{" << std::endl;
			GetRegFile().PrettyPrint(str);
			str << "}" << std::endl;

			return true;
		}



	}  // namespace arch
}  // namespace gensim
