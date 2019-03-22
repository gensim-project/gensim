/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "arch/ArchDescription.h"
#include "DiagnosticContext.h"

#include "flexbison_archc_ast.h"
#include "flexbison_archc.h"

#include <string>

namespace gensim
{
	namespace arch
	{
		class ArchDescriptionParser
		{
		public:
			ArchDescriptionParser(DiagnosticContext &diag_ctx);

			bool ParseFile(std::string filename);

			ArchDescription *Get();
		private:
			DiagnosticContext& diag_ctx;

			bool load_from_arch_node(ArchC::AstNode &node);
			bool load_regspace(ArchC::AstNode & node);
			bool load_arch_ctor(ArchC::AstNode & ctorNode);
			bool load_feature_set(ArchC::AstNode & ctorNode);
			bool load_mem(ArchC::AstNode & ctorNode);

			ArchDescription *arch;
		};
	}
}
