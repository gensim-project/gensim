/*
 * Constraint.h
 *
 *  Created on: 1 Nov 2013
 *      Author: harry
 */

#ifndef CONSTRAINT_H_
#define CONSTRAINT_H_

#include <string>
#include <stdint.h>

#include "genC/ssa/statement/SSAStatement.h"

#include "genC/Enums.h"
#include "genC/ir/IREnums.h"

namespace gensim
{

	namespace arch
	{
		class ArchDescription;
	}
	namespace isa
	{
		class InstructionDescription;
	}

	namespace generator
	{
		namespace testgeneration
		{

			class ConstraintBinaryExpression;
			class ConstraintExpression;
			class ConstraintSet;
			class ValueSet;

			typedef std::vector<genc::ssa::SSABlock *> BlockPath;

			class IConstraint
			{
			public:
				virtual ~IConstraint();
				virtual std::string prettyprint() const = 0;
				virtual uint64_t ScoreValueSet(const ValueSet &values) const = 0;

				std::vector<std::string> GetFields() const;
				virtual void GetFields(std::vector<std::string> &str) const = 0;

				virtual bool IsSolvable() const = 0;
				virtual bool ConflictsWith(const IConstraint &other) const = 0;
				virtual bool IsAtomic() const = 0;

				bool operator==(const IConstraint &other) const;
				virtual bool Equals(const IConstraint &other) const = 0;

				virtual IConstraint* Clone() const = 0;
			};

			class Constraint : public IConstraint
			{
			public:
				Constraint(const ConstraintBinaryExpression *const expr);
				Constraint(const ConstraintExpression *const LHS, const ConstraintExpression *const RHS, genc::BinaryOperator::EBinaryOperator op);
				Constraint(const genc::ssa::SSAStatement *const LHS, const genc::ssa::SSAStatement *const RHS, genc::BinaryOperator::EBinaryOperator op, const BlockPath &path);

				std::string prettyprint() const;

				void GetFields(std::vector<std::string> &) const;

				const ConstraintBinaryExpression *const InnerExpression;

				uint64_t ScoreValueSet(const ValueSet &values) const;
				bool IsAtomic() const;
				bool ConflictsWith(const IConstraint &other) const;
				bool IsSolvable() const;
				bool Equals(const IConstraint &other) const;

				IConstraint* Clone() const ;

			protected:
				FILE *_LogFile;

			private:
				Constraint();
				Constraint(const Constraint &other);
			};
		}
	}
}

namespace gensim
{
	namespace generator
	{
		namespace testgeneration
		{

			class ConstraintExpression
			{
			public:
				virtual ~ConstraintExpression();
				virtual ConstraintExpression *Clone() const = 0;

				virtual std::string prettyprint() const = 0;
				bool operator==(const ConstraintExpression &other) const;
				virtual void GetFields(std::vector<std::string> &fields) const = 0;
				virtual bool IsSigned() const = 0;
				virtual uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const = 0;

				virtual bool EqualTo(const ConstraintExpression &other) const = 0;

				static ConstraintExpression *Construct(const gensim::genc::ssa::SSAStatement *base_stmt, const BlockPath &block_list);

				virtual uint32_t EvaluateResult(const ValueSet &values) const = 0;
			};

			class ConstraintBinaryExpression : public ConstraintExpression
			{
			public:
				ConstraintBinaryExpression(const genc::ssa::SSABinaryArithmeticStatement *base_ssa, const BlockPath &path);
				ConstraintBinaryExpression(const ConstraintExpression *lhs, const ConstraintExpression *rhs, gensim::genc::BinaryOperator::EBinaryOperator op);

				ConstraintBinaryExpression(const ConstraintBinaryExpression& other);
				ConstraintExpression *Clone() const;

				std::string prettyprint() const;
				void GetFields(std::vector<std::string> &fields) const;
				bool IsSigned() const;
				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint64_t EvaluateConstraint(const ValueSet &values) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				const ConstraintExpression *const LHS, *const RHS;
				const gensim::genc::BinaryOperator::EBinaryOperator Operator;
			};

			class ConstraintTernaryExpression : public ConstraintExpression
			{
			public:
				ConstraintTernaryExpression(const genc::ssa::SSASelectStatement *base_ssa, const BlockPath &path);
				ConstraintTernaryExpression(const ConstraintTernaryExpression &other);
				ConstraintExpression *Clone() const;

				std::string prettyprint() const;
				void GetFields(std::vector<std::string> &fields) const;
				bool IsSigned() const;
				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				const ConstraintExpression *Check, *IfTrue, *IfFalse;
			};

			class ConstraintCastExpression : public ConstraintExpression
			{
			public:
				ConstraintCastExpression(const genc::ssa::SSACastStatement *base_ssa, const BlockPath &path);
				ConstraintCastExpression(const ConstraintExpression *baseexpr, genc::IRType totype, genc::IRType fromtype);
				ConstraintCastExpression(const ConstraintCastExpression& other);

				ConstraintExpression *Clone() const;

				std::string prettyprint() const;
				void GetFields(std::vector<std::string> &fields) const;
				bool IsSigned() const;
				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				const gensim::genc::IRType ToType;
				const gensim::genc::IRType FromType;
				const ConstraintExpression *BaseExpression;
			};

			class ConstraintValue : public ConstraintExpression
			{
			public:

				std::string prettyprint() const = 0;
				virtual void GetFields(std::vector<std::string> &fields) const;
				bool IsSigned() const;
				virtual ConstraintExpression *Clone() const = 0;

			};

			class ConstraintConstantValue : public ConstraintValue
			{
			public:
				// ConstraintConstantValue(const genc::ssa::SSAConstantStatement *base_ssa, const BlockPath& path);
				ConstraintConstantValue(uint64_t value, uint32_t size_bits);
				ConstraintConstantValue(const ConstraintConstantValue &other);
				ConstraintExpression *Clone() const;

				std::string prettyprint() const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				const uint64_t Value;
				uint32_t size_bits;
			};

			class ConstraintRegisterValue : public ConstraintValue
			{
			public:
				ConstraintRegisterValue(const genc::ssa::SSARegisterStatement *base_ssa, const BlockPath &path);
				ConstraintRegisterValue(uint64_t regnum);
				ConstraintRegisterValue(uint64_t banknum, ConstraintExpression *regnum);
				ConstraintRegisterValue(const ConstraintRegisterValue &other);
				ConstraintExpression *Clone() const;

				std::string prettyprint() const;
				void GetFields(std::vector<std::string> &fields) const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				bool IsBanked;
				uint64_t BankNum;

				ConstraintExpression *RegNum;
			};

			class ConstraintInsnFieldValue : public ConstraintValue
			{
			public:
				ConstraintInsnFieldValue(const genc::ssa::SSAReadStructMemberStatement *base_ssa, const BlockPath &path);
				ConstraintInsnFieldValue(const std::string &fieldname);
				ConstraintInsnFieldValue(const ConstraintInsnFieldValue &other);
				ConstraintExpression *Clone() const;

				std::string prettyprint() const;
				void GetFields(std::vector<std::string> &fields) const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				const std::string FieldName;
			};

			class ConstraintMemoryValue : public ConstraintValue
			{
			public:
				ConstraintMemoryValue(const genc::ssa::SSAMemoryReadStatement *mem_stmt, const BlockPath &block_list);
				ConstraintMemoryValue(const ConstraintMemoryValue& other);
				ConstraintExpression *Clone() const;

				std::string prettyprint() const;
				void GetFields(std::vector<std::string> &fields) const;

				bool EqualTo(const ConstraintExpression &other) const;
				uint32_t EvaluateResult(const ValueSet &values) const;

				uint32_t GetSize(const arch::ArchDescription *arch_ctx, const isa::InstructionDescription *insn_ctx) const;

				uint8_t Width;
				ConstraintExpression *AddrExpr;
			};
		}
	}
}

namespace std
{
	template <>
	struct hash<gensim::generator::testgeneration::Constraint> {
		uint32_t operator()(const gensim::generator::testgeneration::Constraint &o) const
		{
			return (uint32_t)o.InnerExpression->Operator;
		}
	};

	template <>
	struct equal_to<gensim::generator::testgeneration::Constraint> {
		bool operator()(const gensim::generator::testgeneration::Constraint &a, const gensim::generator::testgeneration::Constraint &b) const
		{
			return *a.InnerExpression == *b.InnerExpression;
		}
	};
}

#endif /* CONSTRAINT_H_ */
