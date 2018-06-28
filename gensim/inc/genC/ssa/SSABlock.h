/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/SSAValue.h"
#include "util/MaybeVector.h"

#include <list>
#include <vector>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace gensim
{
	class DiagnosticContext;

	namespace genc
	{
		class IRStatement;

		namespace ssa
		{
			class SSABuilder;
			class SSAFormAction;
			class SSAStatement;
			class SSAVariableWriteStatement;
			class SSAControlFlowStatement;
			class SSASymbol;
			class SSABlock;

			/**
			 * If a block has only const predecessors, it is always const. If it has only non-const
			 * predecessors, it is never const. If it has a mix, it is sometimes const.
			 */
			enum SSABlockConstness {
				BLOCK_INVALID,
				BLOCK_NEVER_CONST,
				BLOCK_SOMETIMES_CONST,
				BLOCK_ALWAYS_CONST
			};

			class SSABlock : public SSAValue
			{
			public:
				typedef std::vector<SSAStatement *> StatementList;
				typedef StatementList::const_iterator StatementListConstIterator;
				typedef StatementList::iterator StatementListIterator;
				typedef StatementList::reverse_iterator StatementListReverseIterator;
				typedef StatementList::const_reverse_iterator StatementListConstReverseIterator;

				typedef util::MaybeVector<SSABlock*, 2> BlockList;
				typedef util::MaybeVector<const SSABlock*, 2> BlockConstList;

				SSABlock(SSAContext& context, SSAFormAction &parent);
				SSABlock(SSABuilder &bldr);

				~SSABlock();

				SSAFormAction *Parent;

				bool Resolve(DiagnosticContext &ctx);
				void Cleanup();

				void Unlink() override;
				void Destroy();

				bool ContainsStatement(SSAStatement* stmt) const;

				SSAControlFlowStatement *GetControlFlow();
				const SSAControlFlowStatement *GetControlFlow() const;

				void AddStatement(SSAStatement &stmt);
				void AddStatement(SSAStatement &stmt, SSAStatement &before);

				void RemoveStatement(SSAStatement &stmt);

				void AddPredecessor(SSABlock &stmt);

				void RemovePredecessor(SSABlock &stmt);

				const BlockList GetPredecessors() const;

				BlockList GetSuccessors();
				BlockConstList GetSuccessors() const;

				const StatementList &GetStatements() const;
				inline const IRStatement &GetSourceStatement() const
				{
					throw std::logic_error("");
				}

				inline uint32_t GetStartLine() const
				{
					return 0;
				}

				inline uint32_t GetEndLine() const
				{
					return 0;
				}

				const SSAVariableWriteStatement *GetLastWriteTo(const SSASymbol *symbol, StatementListConstReverseIterator startpos) const;
				const SSAVariableWriteStatement *GetLastWriteTo(const SSASymbol *symbol, const SSAStatement *startpos) const;
				const SSAVariableWriteStatement *GetLastWriteTo(const SSASymbol *symbol) const;

				uint32_t GetID() const;

				std::string GetName() const override;

				/**
				 * Returns true if all control flow paths to this block are determinable at JIT-time
				 * @return
				 */
				SSABlockConstness IsFixed() const;

				/**
				 * Initiate fixedness analysis starting from this block. This is most useful to the user
				 * when called on an action entry block.
				 * @return
				 */
				bool InitiateFixednessAnalysis();

				/**
				 * Clear fixedness analysis information
				 */
				void ClearFixedness();

				/**
				 * Initiate value propagation for this block. Values passed around in variables are instead
				 * passed around directly where possible
				 */
				bool DoValuePropagation();

				/**
				 * Insert temporary variable reads and writes for statements which use the values of
				 * statements in other basic blocks. This is essentially a fix-up pass for after code
				 * generation
				 */
				void DoInsertTemporaries();

				/**
				 * Get all ancestors of this block
				 * @return
				 */
				const std::set<SSABlock *> GetAllPredecessors() const;

				std::set<SSASymbol *> DynamicIn;
				std::set<SSASymbol *> DynamicOut;

				SSABlockConstness _constness;

				const SSAType GetType() const override
				{
					return _type;
				}

				std::string ToString() const override;

			private:
				StatementList Statements;
				SSAType _type;

				SSABlock() = delete;
			};
		}
	}
}
