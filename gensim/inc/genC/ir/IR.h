/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   GenCIR.h
 * Author: Harry
 *
 * Created on 17 May 2012, 16:05
 */

#ifndef GENCIR_H
#define GENCIR_H

#include <map>
#include <stdint.h>
#include <sstream>
#include <vector>

#include <antlr3.h>

#include "arch/ArchDescription.h"
#include "IREnums.h"
#include "IRType.h"
#include "genC/DiagNode.h"
#include "genC/ir/IRConstant.h"

namespace gensim
{
	namespace isa
	{
		class ISADescription;
	}

	namespace genc
	{

		namespace ssa
		{
			class SSABuilder;
			class SSABlock;
			class SSAStatement;
			class SSAFormAction;
		}

		class IRAction;
		class IRCallableAction;
		class GenCContext;

		class IRType;
		class IRStructType;

		class IRScope;
		class IRSymbol;

		class IRStatement
		{
		public:
			virtual void PrettyPrint(std::ostringstream &out) const;
			virtual void PrettyPrint() const;
			std::string Dump() const;
			virtual bool Resolve(GenCContext &Context) = 0;

			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const = 0;

			IRStatement(IRScope &scope);
			void SetDiag(const DiagNode &diag)
			{
				diag_node_ = diag;
			}
			const DiagNode &Diag() const
			{
				return diag_node_;
			}

			inline IRScope &GetScope() const
			{
				return Scope;
			}
			bool Resolved;

		protected:
		private:
			IRScope &Scope;
			DiagNode diag_node_;

			IRStatement();
			IRStatement(const pANTLR3_BASE_TREE &);
		};

		class IRExpression : public IRStatement
		{
		public:
			virtual const IRType EvaluateType() = 0;

			// Do any error checking or symbol resolution requires. Returns 1 if this was successful, otherwise 0
			virtual bool Resolve(GenCContext &Context) = 0;
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const = 0;
			virtual bool IsTrivial() const = 0;

			virtual ~IRExpression() {}

			IRExpression(IRScope &scope) : IRStatement(scope) {}
		};

		class EmptyExpression : public IRExpression
		{
		public:
			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual void PrettyPrint(std::ostringstream &out) const;
			virtual bool IsTrivial() const
			{
				return true;
			}

			EmptyExpression(IRScope &scope) : IRExpression(scope) {}
		};

		class IRTernaryExpression : public IRExpression
		{
		public:
			virtual const IRType EvaluateType();

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual void PrettyPrint(std::ostringstream &out) const;

			IRTernaryExpression(IRScope &scope, IRExpression *cond, IRExpression *left, IRExpression *right) : IRExpression(scope), Cond(cond), Left(left), Right(right) {}

			virtual bool IsTrivial() const
			{
				return Cond->IsTrivial() && Left->IsTrivial() && Right->IsTrivial();
			}

		private:
			IRExpression *Cond, *Left, *Right;
		};

		class IRUnaryExpression : public IRExpression
		{
		public:
			IRUnaryOperator::EIRUnaryOperator Type;

			IRExpression *BaseExpression;
			IRExpression *Arg;      // Index argument
			IRExpression *Arg2;      // Index 2nd argument
			std::string MemberStr;  // Member/Ptr argument

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual void PrettyPrint(std::ostringstream &out) const;

			virtual bool IsTrivial() const
			{
				switch (Type) {
						using namespace IRUnaryOperator;
					case Positive:
					case Negative:
					case Complement:
					case Negate:
					case Member:
						return true;
					default:
						return false;
				}
			}

			IRUnaryExpression(IRScope &scope) : IRExpression(scope) {}
		};

		class IRCallExpression : public IRExpression
		{
		public:
			std::string TargetName;
			std::vector<IRExpression *> Args;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual void PrettyPrint(std::ostringstream &out) const;

			virtual bool IsTrivial() const
			{
				return false;
			}

			const IRCallableAction &GetTarget() const
			{
				return *Target;
			}

			IRCallExpression(IRScope &scope) : IRExpression(scope), Target(NULL) {}

		protected:
			bool ResolveParameter(GenCContext &Context, IRType paramType, IRExpression *givenParameter, int idx);

		private:
			ssa::SSAStatement *EmitIntrinsicCall(ssa::SSABuilder &bldr, const gensim::arch::ArchDescription &) const;
			ssa::SSAStatement *EmitExternalCall(ssa::SSABuilder &bldr, const gensim::arch::ArchDescription &) const;
			ssa::SSAStatement *EmitHelperCall(ssa::SSABuilder &bldr, const gensim::arch::ArchDescription &) const;

			IRCallableAction *Target;
		};

		class IRBinaryExpression : public IRExpression
		{
		public:
			BinaryOperator::EBinaryOperator Type;

			IRExpression *Left, *Right;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual void PrettyPrint(std::ostringstream &out) const;

			virtual bool IsTrivial() const
			{
				return Left->IsTrivial() && Right->IsTrivial();
			}

			IRBinaryExpression(IRScope &scope) : IRExpression(scope) {}
		};

		class IRCastExpression : public IRExpression
		{
		public:
			enum IRCastType {
				Transform,
				Bitcast
			};

			const IRType ToType;
			IRExpression *Expr;
			IRCastType CastType;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual void PrettyPrint(std::ostringstream &out) const;

			virtual bool IsTrivial() const
			{
				return Expr->IsTrivial();
			}

			IRCastExpression(IRScope &scope, const IRType toType) : IRExpression(scope), ToType(toType), CastType(Transform) {}
			IRCastExpression(IRScope &scope, const IRType toType, IRCastType ctype) : IRExpression(scope), ToType(toType), CastType(ctype) {}
		};

		class IRVariableExpression : public IRExpression
		{
		public:
			const IRSymbol *Symbol;
			IRScope &Scope;

			IRVariableExpression(std::string name, IRScope &scope) : IRExpression(scope), Symbol(NULL), Scope(scope), name_(name) {}

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual bool IsTrivial() const
			{
				return true;
			}

			virtual void PrettyPrint(std::ostringstream &out) const;

		private:
			std::string name_;
		};

		class IRConstExpression : public IRExpression
		{
		public:
			const IRType Type;

			IRConstExpression(IRScope &scope, const IRType &type, IRConstant value) : IRExpression(scope), Type(type), value_()
			{
				if(IRType::Cast(value, type, type) != value) {
					throw std::logic_error("Given type cannot store given value");
				}
				value_ = value;
			}

			const IRConstant &GetValue() const
			{
				return value_;
			}

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual void PrettyPrint(std::ostringstream &out) const;
			virtual const IRType EvaluateType();
			virtual bool IsTrivial() const
			{
				return true;
			}

		private:
			IRConstant value_;

		};

		class IRVectorExpression : public IRExpression
		{
		public:
			IRVectorExpression(IRScope &scope, std::vector<IRExpression*> elements) : IRExpression(scope), elements_(elements) {}
			~IRVectorExpression();

			bool Resolve(GenCContext& Context) override;
			ssa::SSAStatement* EmitSSAForm(ssa::SSABuilder& bldr) const override;
			void PrettyPrint(std::ostringstream& out) const override;

			const IRType EvaluateType() override;

			bool IsTrivial() const override
			{
				return false;
			}
			std::vector<IRExpression*> &GetElements()
			{
				return elements_;
			}
			const std::vector<IRExpression*> &GetElements() const
			{
				return elements_;
			}

		private:
			std::vector<IRExpression*> elements_;
		};

		class IRDefineExpression : public IRExpression
		{
		public:
			const std::string GetName();
			const IRType &GetType();

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual const IRType EvaluateType();
			virtual void PrettyPrint(std::ostringstream &out) const;

			virtual bool IsTrivial() const
			{
				return false;
			}

			const IRSymbol *GetSymbol() const
			{
				return symbol;
			}

			IRDefineExpression(IRScope &_scope, const IRType _type, std::string _name);

		private:
			const IRType type;
			std::string name;
			IRSymbol *symbol;
		};

		class IRExpressionStatement : public IRStatement
		{
		public:
			IRExpression *Expr;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual void PrettyPrint(std::ostringstream &out) const;

			IRExpressionStatement(IRScope &scope, IRExpression &expr);
		};

		class IRSelectionStatement : public IRStatement
		{
		public:
			enum SelectionType {
				SELECT_SWITCH,
				SELECT_IF
			};
			SelectionType Type;
			IRExpression *Expr;
			IRStatement *Body;
			IRStatement *ElseBody;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual void PrettyPrint(std::ostringstream &out) const;

			IRSelectionStatement(IRScope &scope, SelectionType type, IRExpression &expr, IRStatement &body, IRStatement *elsebody = 0);
		};

		class IRIterationStatement : public IRStatement
		{
		public:
			enum IterationType {
				ITERATE_FOR,
				ITERATE_DO_WHILE,
				ITERATE_WHILE
			};

			IterationType Type;
			IRExpression *For_Expr_Start;
			IRExpression *For_Expr_Check;
			IRExpression *Expr;
			IRStatement *Body;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual void PrettyPrint(std::ostringstream &out) const;

			static IRIterationStatement *CreateFor(IRScope &scope, IRExpression &Start, IRExpression &Check, IRExpression &End, IRStatement &Body);
			static IRIterationStatement *CreateWhile(IRScope &scope, IRExpression &Expr, IRStatement &Body);
			static IRIterationStatement *CreateDoWhile(IRScope &scope, IRExpression &Expr, IRStatement &Body);

		private:
			IRIterationStatement(IRScope &scope) : IRStatement(scope) {}
		};

		class IRFlowStatement : public IRStatement
		{
		public:
			enum FlowType {
				FLOW_CASE,
				FLOW_DEFAULT,
				FLOW_BREAK,
				FLOW_CONTINUE,
				FLOW_RETURN_VOID,
				FLOW_RETURN_VALUE,
				FLOW_RAISE,

				FLOW_INVALID
			};

			FlowType Type;
			IRExpression *Expr;
			IRStatement *Body;

			virtual bool Resolve(GenCContext &Context);
			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

			virtual void PrettyPrint(std::ostringstream &out) const;

			IRFlowStatement(IRScope &scope) : IRStatement(scope), Expr(NULL), Body(NULL), Type(FLOW_INVALID) {}
		};

		class IRBody : public IRStatement
		{
		public:
			static IRBody *CreateBodyWithScope(IRScope &scope);

			std::vector<IRStatement *> Statements;
			IRScope &Scope;

			IRBody(IRScope &containing_scope);

			virtual bool Resolve(GenCContext &Context);

			virtual void PrettyPrint(std::ostringstream &out) const;

			virtual ssa::SSAStatement *EmitSSAForm(ssa::SSABuilder &bldr) const;

		private:
			IRBody(IRScope &containing_scope, IRScope &scope);
		};
	}
}

#endif /* GENCIR_H */
