/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAType.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAValue.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/metadata/SSAMetadata.h"
#include "util/Visitable.h"
#include "Util.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <map>
#include <string>
#include <set>
#include <sstream>
#include <vector>
#include <iostream>

// First, static cast the value to an SSAValue*. If we don't, then the dynamic
// cast is eliminated (because it's casting something to the same type as it
// already is).
#define OPERAND_SETTER(name, type, index) \
	void Set##name(type *_value) { \
		if((_value != nullptr) && (dynamic_cast<type*>((SSAValue*)_value) == nullptr)) { \
			throw std::logic_error("Operand type mismatch"); \
		} \
		SetOperand(index, _value); \
	}


#define OPERAND_GETTER(name, type, index) type *name() { return (type*)GetOperand(index); } const type *name() const { return (const type*)GetOperand(index); }
#define OPERAND(name, type, index) OPERAND_GETTER(name, type, index) OPERAND_SETTER(name, type, index)
#define STATEMENT_OPERAND(name, index) OPERAND(name, SSAStatement, index)
#define BLOCK_OPERAND(name, index) OPERAND(name, SSABlock, index)
#define SYMBOL_OPERAND(name, index) OPERAND(name, SSASymbol, index)
#define ACTION_OPERAND(name ,index) OPERAND(name,SSAActionBase,index)

namespace gensim
{
	class DiagnosticContext;

	namespace genc
	{
		class IRHelperAction;
		class IRStatement;

		namespace ssa
		{
			class SSASymbol;
			class SSAStatementVisitor;
			class SSAFormAction;

			/**
			 * Class representing an SSA statement. An IRStatement may consist of multiple SSAStatements
			 * e.g. a += b; consists of reads of a and b, a binary arithmetic operation, and a write to a.
			 */
			class SSAStatement : public SSAValue, public util::Visitable<SSAStatementVisitor>
			{
			public:
				enum StatementClass {
					Class_Unknown,
					Class_Arithmetic,
					Class_Call,
					Class_Controlflow
				};

				StatementClass GetClass() const
				{
					return class_;
				}

				typedef std::vector<SSAValue*> operand_list_t;

				bool Resolved;

				SSABlock *Parent;

				/**
				 * Returns true if this SSA statement produces a value. Writes to variables, for example,
				 * do not produce values
				 * @return
				 */
				bool HasValue() const;

				/**
				 * Return true if this statement should not be considered dead, even if it
				 * has no uses
				 * @return
				 */
				virtual bool HasSideEffects() const;

				/**
				 * Returns true if this SSA statement should be computed at JIT time rather than emitted
				 * as llvm instructions
				 * @return
				 */
				virtual bool IsFixed() const = 0;

				void Unlink() override;

				/**
				 * Returns a list of variables killed by this statement.
				 * A variable is killed if it is assigned a value which might
				 * not be const
				 */
				virtual std::set<SSASymbol *> GetKilledVariables() = 0;

				virtual void PrettyPrint(std::ostringstream &) const;
				std::string ToString() const override;

				bool Replace(const SSAValue *find, SSAValue *replace);

				virtual bool Resolve(DiagnosticContext &ctx);

				const std::string &GetISA() const;

				std::string GetName() const;

				int GetIndex() const;

				static void CheckConsistency();

				virtual void Accept(SSAStatementVisitor& visitor) override;

				virtual ~SSAStatement();

				const std::set<SSAStatement *> GetUsedStatements();

				const operand_list_t &GetOperands() const
				{
					return operands_;
				}
			protected:
#ifdef DEBUG_STATEMENTS
				static std::unordered_set<SSAStatement*> _all_statements;
#endif

				SSAStatement(StatementClass clazz, int operand_count, SSABlock *parent, SSAStatement *before = NULL);

				SSAValue *GetOperand(int idx)
				{
					if(operands_.at(idx) != nullptr) {
						operands_.at(idx)->CheckDisposal();
					}
					return operands_.at(idx);
				}

				const SSAValue *GetOperand(int idx) const
				{
					return operands_.at(idx);
				}

				void SetOperand(int idx, SSAValue *new_value);
				void SetOperandCount(int count);
				void AddOperand(SSAValue *new_operand);

				unsigned OperandCount() const
				{
					return operands_.size();
				}

				operand_list_t &GetOperands()
				{
					return operands_;
				}
			private:
				const StatementClass class_;
				operand_list_t operands_;

				SSAStatement(const SSAStatement&) = delete;
				SSAStatement(SSAStatement&&) = delete;
			};
		}
	}
}
