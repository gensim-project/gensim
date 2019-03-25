/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ISADescriptionParser.h
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#ifndef INC_ISA_ISADESCRIPTIONPARSER_H_
#define INC_ISA_ISADESCRIPTIONPARSER_H_

#include "isa/ISADescription.h"
#include "DiagnosticContext.h"

#include "flexbison_archc_ast.h"
#include "flexbison_archc.h"

namespace gensim
{
	namespace isa
	{

		class ISADescriptionParser
		{
		public:
			ISADescriptionParser(DiagnosticContext &diag, uint8_t isa_size);

			bool ParseFile(std::string filename);

			ISADescription *Get();
		private:
			DiagnosticContext &diag;

			ISADescription *isa;

			// load high-level ISA information from the given ARCH_CTOR node
			bool load_from_node(ArchC::AstNode &node, std::string filename);

			// load arch decode and disassembly info from given ISA_CTOR node
			bool load_isa_from_node(ArchC::AstNode & node, std::string filename);

			bool load_behaviours();
			bool load_behaviour_file(std::string filename);

			bool parse_struct(ArchC::AstNode &node);

			std::set<std::string> parsed_files;
			std::set<std::string> loaded_files;
		};

	}
}



#endif /* INC_ISA_ISADESCRIPTIONPARSER_H_ */
