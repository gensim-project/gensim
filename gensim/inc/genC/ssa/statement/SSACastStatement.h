/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{

			/**
			 * A statement representing a cast of a genC register. This may truncate or sign extend the value.
			 */
			class SSACastStatement : public SSAStatement
			{
			public:

				enum CastType {
					// This cast is unknown. Unknown casts should not exist: they should be flagged during validation
					Cast_Unknown,

					// Consider the 'bits' of the source value to be of the new type. The bit widths of the types must match.
					Cast_Reinterpret,

					// Remove higher order bits of the source value until it is the same width as the new type
					Cast_Truncate,

					// Sign extend the source value until it is the same width as the new type
					Cast_SignExtend,

					// Zero extend the new value until it is the same width as the new type
					Cast_ZeroExtend,

					// Convert the 'bits' of the source value to the data format for the new type. This cast type has an option.
					Cast_Convert,
					
					// Splat a scalar into a vector type
					Cast_VectorSplat
				};

				enum CastOption {
					// An unknown option. This should be flagged at validation.
					Option_Unknown,

					// The default option is no option. This is for casts which do not have options.
					Option_None,

					// These options are targeted at floating point conversions:
					// RoundDefault refers to whatever rounding mode is currently selected
					Option_RoundDefault,
					// These four match the IEEE standard rounding modes.
					Option_RoundTowardNearest,
					Option_RoundTowardPInfinity,
					Option_RoundTowardNInfinity,
					Option_RoundTowardZero
				};

				virtual bool IsFixed() const;
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();
				bool HasSideEffects() const override;

				virtual bool Resolve(DiagnosticContext &ctx);

				STATEMENT_OPERAND(Expr, 0);

				void Accept(SSAStatementVisitor& visitor) override;

				SSACastStatement(SSABlock *parent, const IRType &targetType, SSAStatement *expr, SSAStatement *before = NULL);

				SSACastStatement(SSABlock *parent, const IRType &targetType, SSAStatement *expr, CastType kind, SSAStatement *before = NULL) : SSAStatement(Class_Unknown, 1, parent, before), option_(Option_None), target_type_(targetType), cast_type_(kind)
				{
					assert(expr);
					SetExpr(expr);
				}

				CastOption GetOption() const
				{
					return option_;
				}

				void SetOption(CastOption option)
				{
					option_ = option;
				}

				~SSACastStatement();

				CastType GetCastType() const;

				const SSAType GetType() const override
				{
					return target_type_;
				}

			private:
				SSACastStatement();
				mutable CastType cast_type_; // this might be lazily evaluated. TODO: make a transform pass which does this 'eagerly'.
				mutable CastOption option_;
				const SSAType target_type_;
			};
		}
	}
}
