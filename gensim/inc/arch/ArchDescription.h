/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
#include "MemoryInterfaceDescription.h"

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

			typedef std::list<isa::ISADescription *> ISAListType;

			ISAListType ISAs;

			ArchDescription();
			virtual ~ArchDescription();

			const isa::ISADescription *GetIsaByName(const std::string isaname) const;
			isa::ISADescription *GetIsaByName(const std::string isaname);

			RegisterFile &GetRegFile()
			{
				return register_file_;
			}
			const RegisterFile &GetRegFile() const
			{
				return register_file_;
			}

			ArchFeatureSet &GetFeatures()
			{
				return features_;
			}
			const ArchFeatureSet &GetFeatures() const
			{
				return features_;
			}

			MemoryInterfacesDescription &GetMemoryInterfaces()
			{
				return memory_interfaces_;
			}
			const MemoryInterfacesDescription &GetMemoryInterfaces() const
			{
				return memory_interfaces_;
			}

			bool PrettyPrint(std::ostream &) const;

			uint32_t GetMaxInstructionSize() const;

			typedef std::map<std::string, std::string> TypenameMap;

			// should refactor type manager to belong to arch in order to avoid
			// having to pass around strings here
			typedef std::map<std::string, std::pair<std::string, std::string>> ConstantMap;
			TypenameMap &GetTypenames()
			{
				return type_names_;
			}
			const TypenameMap &GetTypenames() const
			{
				return type_names_;
			}

			ConstantMap &GetConstants()
			{
				return constants_;
			}
			const ConstantMap &GetConstants() const
			{
				return constants_;
			}

		private:
			// disallow copy
			ArchDescription(const ArchDescription &orig);
			ArchDescription &operator=(const ArchDescription &orig);

			RegisterFile register_file_;
			ArchFeatureSet features_;

			MemoryInterfacesDescription memory_interfaces_;

			TypenameMap type_names_;
			ConstantMap constants_;
		};
	}  // namespace arch
}  // namespace gensim

#endif /* _ARCHDESCRIPTION_H */
