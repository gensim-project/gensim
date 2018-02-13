/*
 * genC/ssa/SSABuilder.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/Enums.h"
#include "genC/ssa/SSAFormAction.h"

#include <stack>
#include <unordered_map>

namespace gensim
{
	namespace genc
	{
		class IRAction;
		class IRSymbol;

		namespace ssa
		{
			class SSABlock;
			class SSAFormAction;
			class SSAStatement;
			class SSASymbol;
			class SSAContext;

			class SSABuilder
			{
			public:
				SSAContext& Context;
				SSAFormAction *Target;
				std::stack<SSABlock *> ReturnBlockStack;

				SSABuilder(SSAContext& context, const IRAction& action);

				bool HasSymbol(std::string name);
				SSASymbol *GetSymbol(const IRSymbol *sym);
				SSASymbol *InsertSymbol(const IRSymbol *sym, SSASymbol *referenceTo = NULL);
				SSASymbol *InsertNewSymbol(const std::string name, const IRType &type, const SymbolType stype, SSASymbol * const referenceTo = NULL);
				SSASymbol *GetTemporarySymbol(IRType type);

				void AddInstruction(SSAStatement &insn);
				void AddBlock(SSABlock &block);
				void AddBlockAndChange(SSABlock &block);
				void ChangeBlock(SSABlock &block, bool EmitBranch);
				void EmitBranch(SSABlock &target, const IRStatement &stmt);

				SSABlock &GetBlock();

				void PushBreak(SSABlock &block);
				SSABlock &PeekBreak();
				SSABlock &PopBreak();

				void PushCont(SSABlock &block);
				SSABlock &PeekCont();
				SSABlock &PopCont();

			private:
				SSABlock *curr_block;

				std::unordered_map<std::string, SSASymbol *> _param_symbol_map;
				std::unordered_map<const IRSymbol *, SSASymbol *> _symbol_map;

				std::stack<SSABlock *> BreakStack;
				std::stack<SSABlock *> ContStack;
			};
		}
	}
}
