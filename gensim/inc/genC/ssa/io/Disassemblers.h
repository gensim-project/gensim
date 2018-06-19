/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   DisassemblyPrinter.h
 * Author: harry
 *
 * Created on 13 July 2017, 15:34
 */

#ifndef DISASSEMBLYPRINTER_H
#define DISASSEMBLYPRINTER_H

#include <ostream>
#include "genC/ssa/SSAType.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
			class SSAActionBase;
			class SSABlock;
			class SSAStatement;

			namespace io
			{
				class ContextDisassembler
				{
				public:
					void Disassemble(const SSAContext *context, std::ostream &str);
				};

				class ActionDisassembler
				{
				public:
					void Disassemble(SSAActionBase *action, std::ostream &str);
				};

				class BlockDisassembler
				{
				public:
					void Disassemble(SSABlock *block, std::ostream &str);
				};

				class TypeDisassembler
				{
				public:
					void Disassemble(const SSAType &type, std::ostream &str);
					void DisassemblePOD(const SSAType &type, std::ostream &str);
					void DisassembleStruct(const SSAType &type, std::ostream &str);
				};

				class StatementDisassembler
				{
				public:
					void Disassemble(SSAStatement *stmt, std::ostream &str);
				};
			}
		}
	}
}

#endif /* DISASSEMBLYPRINTER_H */

