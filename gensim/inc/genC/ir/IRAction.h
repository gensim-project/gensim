/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/Enums.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IRScope.h"
#include "genC/ir/IRSignature.h"
#include "genC/DiagNode.h"
#include "genC/Intrinsics.h"
#include "IR.h"

#include <map>
#include <string>
#include <functional>

namespace gensim
{
	namespace genc
	{
		class GenCContext;
		class IRScope;
		class IRStatement;
		class IRCallExpression;

		namespace ssa
		{
			class SSABuilder;
			class SSAFormAction;
			class SSAContext;
			class SSAIntrinsicStatement;
		}

		class IRAction
		{
		public:
			typedef std::vector<IRSymbol *> ParamListType;
			typedef ParamListType::iterator ParamListIterator;
			typedef ParamListType::const_iterator ParamListConstIterator;

			IRAction(GenCContext &context);
			virtual ~IRAction() { }

			virtual IRSignature GetSignature(const IRCallExpression *callExpression = nullptr) const = 0;

			const DiagNode& Diag() const
			{
				return _diag_node;
			}

			void SetDiag(const DiagNode &node)
			{
				_diag_node = node;
			}

			IRStatement *body;
			GenCContext &Context;

			void PrettyPrint(std::ostringstream &out) const;
			virtual void PrettyPrintHeader(std::ostringstream &out) const = 0;

			IRScope *GetFunctionScope();

			ssa::SSAFormAction *GetSSAForm(ssa::SSAContext& context);
			void EmitSSA(ssa::SSABuilder &bldr);

		private:
			IRScope *_scope;
			DiagNode _diag_node;
			ssa::SSAFormAction *emitted_ssa_;
		};

		class IRCallableAction : public IRAction
		{
		public:
			IRCallableAction(GenCContext &context);
			virtual ~IRCallableAction() { }
		};

		class IRHelperAction : public IRCallableAction
		{
		public:
			IRHelperAction(const IRSignature &signature, HelperScope scope, GenCContext &context);
			virtual ~IRHelperAction() { }

			virtual void PrettyPrintHeader(std::ostringstream &out) const;

			IRSignature GetSignature(const IRCallExpression *callExpression = nullptr) const override
			{
				return signature_;
			}

		private:
			IRSignature signature_;
			uint32_t _statement_count;
			const HelperScope _scope;
		};

		class IRIntrinsicAction : public IRCallableAction
		{
		public:
			IRIntrinsicAction(IntrinsicDescriptor &descriptor, GenCContext &ctx);
			virtual ~IRIntrinsicAction() { }

			virtual void PrettyPrintHeader(std::ostringstream& out) const override;

			IRSignature GetSignature(const IRCallExpression *callExpression) const override
			{
				return descriptor_.GetSignature(callExpression);
			}

			const std::string& GetName() const
			{
				return descriptor_.GetName();
			}
			IntrinsicID GetID() const
			{
				return descriptor_.GetID();
			}

			bool ResolveFixedness(const genc::ssa::SSAIntrinsicStatement *intrinsicStmt) const
			{
				return descriptor_.GetFixedness(intrinsicStmt);
			}

			ssa::SSAStatement *Emit(const IRCallExpression *callExpression, ssa::SSABuilder& builder) const
			{
				return descriptor_.EmitSSA(callExpression, builder);
			}

		private:
			IntrinsicDescriptor &descriptor_;
		};

		class IRExecuteAction : public IRAction
		{
		public:
			IRExecuteAction(const std::string& name, GenCContext& context, const IRType instructionType);
			virtual ~IRExecuteAction() { }

			virtual void PrettyPrintHeader(std::ostringstream &out) const;
			virtual IRSignature GetSignature(const IRCallExpression *callExpression = nullptr) const override
			{
				return signature_;
			}

		private:
			IRSignature signature_;
		};
	}
}
