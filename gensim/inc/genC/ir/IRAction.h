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
			using SignatureFactory = std::function<IRSignature(const IRIntrinsicAction *, const IRCallExpression *)>;
			using SSAEmitter = std::function<genc::ssa::SSAStatement *(const IRIntrinsicAction *, const IRCallExpression *, ssa::SSABuilder &)>;
			using FixednessResolver = std::function<bool(const genc::ssa::SSAIntrinsicStatement *, const IRIntrinsicAction *)>;

			IRIntrinsicAction(const std::string& name, IntrinsicID id, SignatureFactory factory, SSAEmitter ssaEmitter, FixednessResolver fixednessResolver, GenCContext& context);
			virtual ~IRIntrinsicAction() { }

			virtual void PrettyPrintHeader(std::ostringstream& out) const override;

			IRSignature GetSignature(const IRCallExpression *callExpression) const override
			{
				return factory_(this, callExpression);
			}

			const std::string& GetName() const
			{
				return name_;
			}
			IntrinsicID GetID() const
			{
				return id_;
			}

			bool ResolveFixedness(const genc::ssa::SSAIntrinsicStatement *intrinsicStmt) const
			{
				return resolver_(intrinsicStmt, this);
			}

			ssa::SSAStatement *Emit(const IRCallExpression *callExpression, ssa::SSABuilder& builder) const
			{
				return emitter_(this, callExpression, builder);
			}

		private:
			std::string name_;
			IntrinsicID id_;
			SignatureFactory factory_;
			SSAEmitter emitter_;
			FixednessResolver resolver_;
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
