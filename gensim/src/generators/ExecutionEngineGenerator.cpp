/*
 * ExecutionEngineGenerator.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: harry
 */

#include <ostream>
#include <sstream>
#include <vector>

#include "arch/ArchDescription.h"
#include "generators/ExecutionEngineGenerator.h"

DEFINE_DUMMY_COMPONENT(gensim::generator::ExecutionEngineGenerator, ee_gen);

namespace gensim
{
	namespace generator
	{
		ExecutionEngineGenerator::~ExecutionEngineGenerator() {}

		std::string ExecutionEngineGenerator::GetStateStruct() const
		{
			const arch::ArchDescription &arch = Manager.GetArch();
			std::ostringstream stream;

			// generate defines for tracing
			uint8_t index = 0;
			stream << "\n\n";
			for(const auto &reg : arch.GetRegFile().GetSlots()) {
				stream << "#define TRACE_ID_" << reg->GetID() << " " << (uint32_t)index << std::endl;
				index++;
			}
			index = 0;
			for(const auto &bank : arch.GetRegFile().GetBanks()) {
				stream << "#define TRACE_ID_" << bank->ID << " " << (uint32_t)index << std::endl;
				index++;
			}

			// create state struct containing the registers of this processor
			// todo: this is not correctly aligned for SSE instructions etc
			stream << "\n\nstruct gensim_state { \n";
			stream << "uint8_t register_file [ " << arch.GetRegFile().GetSize() << "];";
			stream << "};\n typedef struct gensim_state REGS_T;\n\n";

			stream << "#undef GENSIM_STATE_PTR\n#define GENSIM_STATE_PTR struct gensim_state *\n\n";

			return stream.str();
		}
	}
}
