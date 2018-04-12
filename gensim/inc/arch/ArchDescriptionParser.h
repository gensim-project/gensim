#pragma once

#include "arch/ArchDescription.h"
#include "DiagnosticContext.h"

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

			bool load_from_arch_node(pANTLR3_BASE_TREE node);
			bool load_regspace(pANTLR3_BASE_TREE node);
			bool load_arch_ctor(pANTLR3_BASE_TREE ctorNode);
			bool load_feature_set(pANTLR3_BASE_TREE ctorNode);
			bool load_mem(pANTLR3_BASE_TREE ctorNode);

			ArchDescription *arch;
		};
	}
}
