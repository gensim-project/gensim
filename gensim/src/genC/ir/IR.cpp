/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <ios>
#include <assert.h>

#include "arch/ArchComponents.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IRAction.h"
#include "genC/Parser.h"

namespace gensim
{
	namespace genc
	{

/////////////////////////////////////////////////////////////
// Constructors
/////////////////////////////////////////////////////////////

		IRBody::IRBody(IRScope &containing_scope) : IRStatement(containing_scope), Scope(*(new IRScope(containing_scope, IRScope::SCOPE_BODY))) {}

		IRBody::IRBody(IRScope &containing_scope, IRScope &scope) : IRStatement(containing_scope), Scope(scope) {}

		IRStatement::IRStatement(IRScope &scope) : Resolved(false), Scope(scope)
		{
		}

		IRDefineExpression::IRDefineExpression(IRScope &scope, const IRType type, std::string name) : IRExpression(scope), type(type), name(name) {}

		IRSelectionStatement::IRSelectionStatement(IRScope &scope, SelectionType type, IRExpression &expr, IRStatement &body, IRStatement *elsebody) : IRStatement(scope), Type(type), Expr(&expr), Body(&body), ElseBody(elsebody) {}

		IRExpressionStatement::IRExpressionStatement(IRScope &scope, IRExpression &expr) : IRStatement(scope)
		{
			Expr = &expr;
		}

/////////////////////////////////////////////////////////////
// Accessors
/////////////////////////////////////////////////////////////

		const std::string IRDefineExpression::GetName()
		{
			return symbol->GetLocalName();
		}

		const IRType &IRDefineExpression::GetType()
		{
			return type;
		}

/////////////////////////////////////////////////////////////
// Resolve Pass
/////////////////////////////////////////////////////////////

		bool IRSelectionStatement::Resolve(GenCContext &Context)
		{
			bool success = true;
			success &= Expr->Resolve(Context);
			success &= Body->Resolve(Context);
			if (ElseBody != NULL) success &= ElseBody->Resolve(Context);
			Resolved = success;
			return success;
		}

		bool IRExpressionStatement::Resolve(GenCContext &Context)
		{
			Resolved = Expr->Resolve(Context);
			return Resolved;
		}

		bool EmptyExpression::Resolve(GenCContext &Context)
		{
			return true;
		}

		bool IRTernaryExpression::Resolve(GenCContext &Context)
		{
			bool success = true;
			success &= Cond->Resolve(Context);
			success &= Left->Resolve(Context);
			success &= Right->Resolve(Context);
			if (!success) {
				Context.Diag().Error("Error in ternary expression", Diag());

				return false;
			}
			if (Left->EvaluateType() != Right->EvaluateType()) {
				Context.Diag().Error( "Type mismatch between ternary subexpressions", Diag());
				return false;
			}
			Resolved = true;
			return true;
		}

		bool IRCallExpression::ResolveParameter(GenCContext &Context, IRType paramType, IRExpression *givenParameter, int idx)
		{
			bool success = true;
			IRType givenType = givenParameter->EvaluateType();

			if (givenType.Reference && givenType != paramType) {
				Context.Diag().Error(Format("In call to %s, parameter %u: Reference parameters cannot be implicitly cast (expecting %s, got %s)", TargetName.c_str(), idx, paramType.PrettyPrint().c_str(), givenType.PrettyPrint().c_str()), Diag());
				success = false;
			}

			if(paramType.Reference) {
				// parameter must be a variable expression
				if(dynamic_cast<IRVariableExpression*>(givenParameter) == nullptr) {
					std::string errstring = Format("In call to %s, parameter %u: Reference parameter must be a simple variable expression", TargetName.c_str(), idx, paramType.PrettyPrint().c_str(), givenType.PrettyPrint().c_str());
					Context.Diag().Error(errstring, Diag());
					success = false;
				}

				// given type must be paramtype or dereferenced paramtype
//				if(paramType.Dereference() != givenType.Dereference()) {
//					std::string errstring = Format("In call to %s, parameter %u: Reference parameters cannot be implicitly cast (expecting %s, got %s)", TargetName.c_str(), idx, paramType.PrettyPrint().c_str(), givenType.PrettyPrint().c_str());
//					Context.Diag().Error(errstring, Diag());
//					success = false;
//				}
			}

			// If the user gives a parameter which is floating, but an integer is expected, give an error
			if(givenType.IsFloating() && !paramType.IsFloating()) {
				std::string errstring = Format("In call to %s, parameter %u: A floating point parameter was given but an integer was expected", TargetName.c_str(), idx);
				Context.Diag().Error(errstring, Diag());
				success = false;
			}

			// Cannot pass structs
			if(givenType.IsStruct()) {
				std::string errstring = Format("In call to %s, parameter %u: Cannot pass struct to helper", TargetName.c_str(), idx);
				Context.Diag().Error(errstring, Diag());
				success = false;
			}

			// If the user gives a parameter which is an integer, but a float is expected, give an error
			if(!givenType.IsFloating() && paramType.IsFloating()) {
				std::string errstring = Format("In call to %s, parameter %u: An integer parameter was given but a floating point value was expected", TargetName.c_str(), idx);
				Context.Diag().Error(errstring, Diag());
				success = false;
			}

			// If the user gives a parameter which is a constant and is too large to fit within the parameter, signal an error
			if(IRConstExpression * constant = dynamic_cast<IRConstExpression*>(givenParameter)) {
				if(constant->Value.Int() > paramType.GetMaxValue()) {
					std::string errstring = Format("In call to %s, parameter %u: Constant value is to large to fit in parameter type %s", TargetName.c_str(), idx, givenType.PrettyPrint().c_str());
					Context.Diag().Error(errstring, Diag());

					success = false;
				}
			}

			// If the user gives a parameter which is a variable and is too large to fit within the parameter, signal a warning
			if(IRExpression *variable = dynamic_cast<IRExpression*>(givenParameter)) {
				IRType::PromoteResult res = givenType.AutoPromote(paramType);
				if(res == IRType::PROMOTE_TRUNCATE) {
					std::string errstring = Format("In call to %s, parameter %u: Value of type %s is to large to fit in parameter type %s", TargetName.c_str(), idx, givenType.PrettyPrint().c_str(), paramType.PrettyPrint().c_str());
					Context.Diag().Warning(errstring, Diag());
				}
				if(res == IRType::PROMOTE_SIGN_CHANGE) {
					std::string errstring = Format("In call to %s, parameter %u: Parameter sign changed", TargetName.c_str(), idx, givenType.PrettyPrint().c_str(), paramType.PrettyPrint().c_str());
					Context.Diag().Warning(errstring, Diag());
				}
			}

			return success;
		}

		bool IRCallExpression::Resolve(GenCContext &Context)
		{
			Target = Context.GetCallable(TargetName);

			if (Target == nullptr) {
				std::string errstring = Format("Undefined function: %s", TargetName.c_str());
				Context.Diag().Error(errstring, Diag());

				return false;
			}

			// If the parameter list is the wrong size, signal an error
			if (Args.size() != Target->GetSignature().GetParams().size()) {
				std::string errstring = Format("In call to %s, expected %lu parameters, %lu supplied", TargetName.c_str(), Target->GetSignature().GetParams().size(), Args.size());
				Context.Diag().Error(errstring, Diag());

				return false;
			}

			auto params = Target->GetSignature().GetParams();
			assert(Args.size() == params.size());

			bool success = true;
			int idx = 1;

			// Loop through the args list and param types and check each one
			for(unsigned int i = 0; i < params.size(); ++i) {
				auto &param = params.at(i);
				auto &arg = Args.at(i);

				if (!arg->Resolve(Context)) {
					success = false;
					Context.Diag().Error("Error in argument " + std::to_string(i), Diag());
					continue;
				}

				IRExpression *givenParameter = arg;

				IRType givenType = arg->EvaluateType();
				IRType paramType = param.GetType();

				success &= ResolveParameter(Context, paramType, givenParameter, idx);
			}
			Resolved = success;
			return success;
		}

		bool IRCastExpression::Resolve(GenCContext &Context)
		{
			if (Resolved) return true;
			if (!Expr->Resolve(Context)) {
				std::string errstring = Format("Error in cast expression");
				Context.Diag().Error(errstring, Diag());

				return false;
			}
			const IRType &ExprType = Expr->EvaluateType();
			if (ExprType.DataType != ToType.DataType) {
				std::string errstring = Format("Cannot cast between structs and POD");
				Context.Diag().Error(errstring, Diag());

				return false;
			}
			if ((ExprType.DataType == IRType::Struct) && (ExprType.BaseType.StructType != ToType.BaseType.StructType)) {
				std::string errstring = Format("Cannot cast between struct types");
				Context.Diag().Error(errstring, Diag());

				return false;
			}
			Resolved = true;
			return true;
		}

		bool IRDefineExpression::Resolve(GenCContext &Context)
		{
			if (Resolved) return true;
			IRType temp;
			if (GetScope().GetSymbolType(name, temp)) {
				std::string errstring = Format("Variable redefined: %s", name.c_str());
				Context.Diag().Error(errstring, Diag());

				return false;
			}
			if (type.Reference) {
				std::string errstring = Format("References types can only be used for parameters");
				Context.Diag().Error(errstring, Diag());

				return false;
			}
			symbol = GetScope().InsertSymbol(name, type, Symbol_Local);
			Resolved = true;
			return true;
		}

		bool IRVariableExpression::Resolve(GenCContext &Context)
		{
			if (!(Symbol = Scope.GetSymbol(name_))) {
				std::string errstring = Format("Undefined Variable: %s", name_.c_str());
				Context.Diag().Error(errstring, Diag());

				return false;
			}
			assert(Symbol);
			Resolved = true;
			return true;
		}

		bool IRUnaryExpression::Resolve(GenCContext &Context)
		{
			if (Resolved) return true;
			bool success = BaseExpression->Resolve(Context);
			if (success) {
				switch (Type) {
						using namespace IRUnaryOperator;
					case Member: {
						const IRType &BaseType = BaseExpression->EvaluateType();
						if (BaseType.DataType != IRType::Struct) {
							std::string errstring = Format("Only struct types have members");
							Context.Diag().Error(errstring, Diag());

							return false;
						}
						if (!BaseType.BaseType.StructType->HasMember(MemberStr)) {
							std::string errstring = Format("Type %s does not have a member named %s", BaseType.PrettyPrint().c_str(), MemberStr.c_str());
							Context.Diag().Error(errstring, Diag());

							return false;
						}
						break;
					}
					case Ptr: {
						const IRType &BaseType = BaseExpression->EvaluateType();
						if (BaseType.DataType != IRType::Struct) {
							std::string errstring = Format("Only struct types have members");
							Context.Diag().Error(errstring, Diag());

							return false;
						}
						break;
					}
					case Index:
						if(!Arg->Resolve(Context)) return false;
						break;
					case BitSequence:
						if(!Arg->Resolve(Context)) return false;
						if(!Arg2->Resolve(Context)) return false;
						break;
					default: {

						break;
					}
				}
			}
			Resolved = success;
			return success;
		}

		bool IRBinaryExpression::Resolve(GenCContext &Context)
		{
			bool success = true;
			bool lsuccess = Left->Resolve(Context);
			assert(Left->Resolved == lsuccess);

			bool rsuccess = Right->Resolve(Context);
			assert(Right->Resolved == rsuccess);
			success = lsuccess & rsuccess;
			if (!success) {
				std::string errstring = Format("Error in binary expression");
				Context.Diag().Error(errstring, Diag());

				return false;
			}

			// need to cast vectors if we have a splat operation
			bool left_is_vector = Left->EvaluateType().VectorWidth > 1;
			bool right_is_vector = Right->EvaluateType().VectorWidth > 1;
			
			if(left_is_vector != right_is_vector) {
				if(!left_is_vector) {
					auto cast = new IRCastExpression(GetScope(), Right->EvaluateType());
					cast->Expr = Left;
					Left = cast;
					cast->Resolve(Context);
				}
				if(!right_is_vector) {
					auto cast = new IRCastExpression(GetScope(), Left->EvaluateType());
					cast->Expr = Right;
					Right = cast;
					cast->Resolve(Context);
				}
			}
			
			// If we are using a set operator, we need to check that Left is large enough to contain Right
			IRType LType = Left->EvaluateType();
			IRType RType = Right->EvaluateType();
			if (BinaryOperator::IsAssignment(Type)) {
				if (LType.DataType != RType.DataType) {
					std::string errstring = Format("Cannot assign value of type %s to variable of type %s", RType.PrettyPrint().c_str(), LType.PrettyPrint().c_str());
					Context.Diag().Error(errstring, Diag());

					success = false;
				} else if ((LType.DataType == IRType::Struct) && (LType.BaseType.StructType != RType.BaseType.StructType)) {
					std::string errstring = Format("Cannot assign struct of type %s to variable of type %s", RType.PrettyPrint().c_str(), LType.PrettyPrint().c_str());
					Context.Diag().Error(errstring, Diag());

					success = false;
				}
			}


			if(LType.DataType == IRType::Struct || RType.DataType == IRType::Struct) {
				Context.Diag().Error("Cannot operate on struct types", Diag());
				success = false;
			}

			Resolved = success;
			return success;
		}

		bool IRConstExpression::Resolve(GenCContext &Context)
		{
			Resolved = true;
			return true;
		}

		bool IRIterationStatement::Resolve(GenCContext &Context)
		{
			bool success = true;
			switch (Type) {
				case ITERATE_FOR:
					success &= For_Expr_Start->Resolve(Context) && For_Expr_Check->Resolve(Context) && Expr->Resolve(Context);
					break;
				case ITERATE_WHILE:
				case ITERATE_DO_WHILE:
					success &= Expr->Resolve(Context);
					break;
			}
			success &= Body->Resolve(Context);
			Resolved = success;
			return success;
		}

		bool IRBody::Resolve(GenCContext &Context)
		{
			bool success = true;
			for (std::vector<IRStatement *>::iterator i = Statements.begin(); i != Statements.end(); ++i) {
				success &= (*i)->Resolve(Context);
			}

			// check to make sure we have only one control flow statement
			int ctrlflow_count = 0;
			IRStatement *last_ctrlflow = nullptr;
			for (std::vector<IRStatement *>::iterator i = Statements.begin(); i != Statements.end(); ++i) {
				IRFlowStatement *flow = dynamic_cast<IRFlowStatement*>(*i);
				if(flow && flow->Type != IRFlowStatement::FLOW_CASE && flow->Type != IRFlowStatement::FLOW_DEFAULT) {
					ctrlflow_count++;
					last_ctrlflow = *i;
				}
			}
			if(ctrlflow_count > 1) {
				Context.Diag().Error("Block must have exactly one control flow statement", last_ctrlflow->Diag());
				success = false;
			}

			Resolved = success;
			return success;
		}

		bool IRFlowStatement::Resolve(GenCContext &Context)
		{
			bool success = true;
			switch (Type) {
				case FLOW_CASE: {
					success &= Expr->Resolve(Context);
					if (!success) return false;
					if (!Expr->EvaluateType().Const) {
						std::string errstring = Format("Case expression must be constant");
						Context.Diag().Error(errstring, Diag());

						success = false;
					}
					if (GetScope().Type != IRScope::SCOPE_SWITCH) {
						Context.Diag().Error("Case expression must be in switch statement", Diag());

						success = false;
					}
					success &= Body->Resolve(Context);
					break;
				}
				case FLOW_DEFAULT: {
					if (GetScope().Type != IRScope::SCOPE_SWITCH) {
						Context.Diag().Error("Case expression must be in switch statement", Diag());

						success = false;
					}
					success &= Body->Resolve(Context);
					break;
				}
				case FLOW_BREAK: {
					if (!GetScope().ScopeBreakable()) {
						Context.Diag().Error("Break expression must be in case statement or loop", Diag());

						success = false;
					}
					break;
				}
				case FLOW_CONTINUE: {
					if (!GetScope().ScopeContinuable()) {
						Context.Diag().Error("Continue expression must be in loop", Diag());

						success = false;
					}
					break;
				}
				case FLOW_RETURN_VOID: {
					if (GetScope().GetContainingAction().GetSignature().GetType() != IRTypes::Void) {
						Context.Diag().Error("Attempting to return void in non-void action", Diag());

						success = false;
					}
					break;
				}
				case FLOW_RETURN_VALUE: {
					success &= Expr->Resolve(Context);
					if (!success) break;

					IRType expr_type = Expr->EvaluateType();
					IRType return_type = GetScope().GetContainingAction().GetSignature().GetType();

					if(expr_type.IsFloating() != return_type.IsFloating()) {
						Context.Diag().Error("Cannot implicitly cast between integer/float types in return", Diag());
						success = false;
						break;
					}

					IRType::PromoteResult promoteResult = expr_type.AutoPromote(return_type);
					switch (promoteResult) {
						case IRType::PROMOTE_TO_STRUCT:
						case IRType::PROMOTE_FROM_STRUCT:
							Context.Diag().Error("Cannot cast between struct and POD types", Diag());

							break;
						case IRType::PROMOTE_SIGN_CHANGE:
							Context.Diag().Warning("Implicit cast causes signedness change", Diag());

							break;
						case IRType::PROMOTE_TRUNCATE:
							Context.Diag().Warning("Implicit cast causes truncation", Diag());

							break;
						default:
							;  // Promotion successful
					}
					break;
				}
			}
			Resolved = success;
			return success;
		}

/////////////////////////////////////////////////////////////
// Type Evaluation
/////////////////////////////////////////////////////////////

		const IRType IRCallExpression::EvaluateType()
		{
			return Target->GetSignature().GetType();
		}

		const IRType IRBinaryExpression::EvaluateType()
		{
			assert(Resolved);
			return IRType::Resolve(Type, Left->EvaluateType(), Right->EvaluateType());
		}

		const IRType IRCastExpression::EvaluateType()
		{
			return ToType;
		}

		const IRType IRConstExpression::EvaluateType()
		{
			return Type;
		}

		const IRType IRDefineExpression::EvaluateType()
		{
			return type;
		}

		const IRType IRVariableExpression::EvaluateType()
		{
			assert(Resolved);
			assert(Symbol);
			return Symbol->Type;
		}

		const IRType EmptyExpression::EvaluateType()
		{
			return IRTypes::Void;
		}

		const IRType IRTernaryExpression::EvaluateType()
		{
			return Left->EvaluateType();
		}

		const IRType IRUnaryExpression::EvaluateType()
		{
			switch (Type) {
					using namespace IRUnaryOperator;
				case Dereference:
					assert(false);
					return IRTypes::Void;
				case Index:	{
					IRType type = BaseExpression->EvaluateType();
					assert(type.VectorWidth != 1);
					type.VectorWidth = 1;
					return type;
				}
				case BitSequence:	{
					IRType type = BaseExpression->EvaluateType();
					type.VectorWidth = 1;
					return type;
				}
				case Member:
				case Ptr: {
					const IRType &BaseType = BaseExpression->EvaluateType();
					assert(BaseType.DataType == IRType::Struct);
					assert(BaseType.BaseType.StructType != NULL);
					return BaseType.BaseType.StructType->GetMemberType(MemberStr);
				}
				default:
					return BaseExpression->EvaluateType();
			}
		}
	}
}
