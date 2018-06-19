/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <map>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSABlock;
			class SSAFormAction;
			class SSAStatement;
			class SSASymbol;
			class SSAValue;

			class SSACloneContext
			{
				friend class SSACloner;
			public:
				SSACloneContext(SSAFormAction *target_action);

				typedef std::map<const SSASymbol *, SSASymbol *> cloned_symbol_map_t;
				typedef std::map<const SSABlock *, SSABlock *> cloned_block_map_t;
				typedef std::map<const SSAStatement *, SSAStatement *> cloned_statement_map_t;

				cloned_symbol_map_t &Symbols()
				{
					return _cloned_symbols;
				}
				cloned_block_map_t &Blocks()
				{
					return _cloned_blocks;
				}
				cloned_statement_map_t &Statements()
				{
					return _cloned_statements;
				}

				void add(const SSAStatement *source, SSAStatement *cloned);
				SSAStatement *get(const SSAStatement *source);

				void add(const SSABlock *source, SSABlock *cloned);
				SSABlock *get(const SSABlock *source);

				SSASymbol *get(const SSASymbol *source);

				SSAValue *getvalue(const SSAValue *source);

			private:
				void CloneMetadata(const SSAValue *source, SSAValue *cloned);

				cloned_symbol_map_t _cloned_symbols;
				cloned_block_map_t _cloned_blocks;
				cloned_statement_map_t _cloned_statements;

				SSAFormAction *_target_action;
			};

			class SSACloner
			{
			public:
				SSAFormAction *Clone(const SSAFormAction *source);
				SSABlock *Clone(SSABlock* block, SSAFormAction *new_parent, SSACloneContext &ctx);
			};
		}
	}
}
