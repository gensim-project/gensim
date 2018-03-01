/*
 * genC/ir/IRAction.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/Enums.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IRScope.h"
#include "genC/ir/IRSignature.h"
#include "genC/DiagNode.h"

#include <map>
#include <string>

namespace gensim
{
	namespace genc
	{
		class GenCContext;
		class IRScope;
		class IRStatement;

		namespace ssa
		{
			class SSABuilder;
			class SSAFormAction;
			class SSAContext;
		}

		class IRAction
		{
		public:
			typedef std::vector<IRSymbol *> ParamListType;
			typedef ParamListType::iterator ParamListIterator;
			typedef ParamListType::const_iterator ParamListConstIterator;

			IRAction(const IRSignature &signature, GenCContext &context);

			IRSignature& GetSignature()
			{
				return _signature;
			}
			const IRSignature& GetSignature() const
			{
				return _signature;
			}

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

		protected:
			ParamListType _params;

		private:
			IRScope *_scope;
			IRSignature _signature;
			DiagNode _diag_node;
			ssa::SSAFormAction *emitted_ssa_;
		};

		class IRCallableAction : public IRAction
		{
		public:
			IRCallableAction(const IRSignature &signature, GenCContext &context);
		};

		class IRHelperAction : public IRCallableAction
		{
		public:
			virtual void PrettyPrintHeader(std::ostringstream &out) const;

			IRHelperAction(const IRSignature &signature, HelperScope scope, GenCContext &context);

		private:
			uint32_t _statement_count;
			const HelperScope _scope;
		};

		class IRExternalAction : public IRCallableAction
		{
		public:
			IRExternalAction(const IRSignature &signature, GenCContext& context);

			void PrettyPrintHeader(std::ostringstream& out) const override;
		};

		class IRIntrinsicAction : public IRCallableAction
		{
		public:
			void PrettyPrintHeader(std::ostringstream& out) const override;

			IRIntrinsicAction(const IRSignature &signature, GenCContext& context);
		};

		class IRExecuteAction : public IRAction
		{
		public:
			IRSymbol *InstructionSymbol;

			virtual void PrettyPrintHeader(std::ostringstream &out) const;

			IRExecuteAction(const std::string& name, GenCContext& context, const IRType instructionType);

		private:
			IRExecuteAction();
		};
	}
}
