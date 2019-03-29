/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   AssemblyReader.h
 * Author: harry
 *
 * Created on 20 July 2017, 10:17
 */

#ifndef ASSEMBLYREADER_H
#define ASSEMBLYREADER_H

#include "DiagnosticContext.h"
#include "flexbison_harness.h"
#include "flexbison_gencssa_ast.h"
#include "flexbison_gencssa.h"

#include <string>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			namespace io
			{

				// Wrap up the antlr tree in case we want to use something
				// different in future
				class AssemblyFileContext
				{
				public:
					AssemblyFileContext(const GenCSSA::AstNode &tree) : tree_(tree) {}
					const GenCSSA::AstNode &GetTree() const
					{
						return tree_;
					}
				private:
					GenCSSA::AstNode tree_;
				};

				class AssemblyReader
				{
				public:
					bool Parse(const std::string &filename, gensim::DiagnosticContext &diag, AssemblyFileContext *& target) const;
					bool ParseText(const std::string &text, gensim::DiagnosticContext &diag, AssemblyFileContext *& target) const;
				};
			}
		}
	}
}

#endif /* ASSEMBLYREADER_H */

