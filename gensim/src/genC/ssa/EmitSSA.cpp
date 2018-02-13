#include <vector>
#include <typeinfo>
#include <list>
#include <string>

#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"

#define is(var, type) (dynamic_cast<type>(var) != NULL)

using namespace gensim::genc;
using namespace gensim::genc::ssa;

////////////////////////////////////////////////////////////////////////////////////////////////
// IRAction                                                                                   //
////////////////////////////////////////////////////////////////////////////////////////////////

void IRAction::EmitSSA(ssa::SSABuilder &bldr)
{
	IRBody *body_stmt = static_cast<IRBody *> (body);

	body_stmt->EmitSSAForm(bldr);
}

SSAFormAction *IRAction::GetSSAForm(SSAContext& context)
{
	if (emitted_ssa_ == nullptr) {
		SSABuilder sb(context, *this);
		EmitSSA(sb);
		emitted_ssa_ = sb.Target;
	}
	return emitted_ssa_;
}

#define OPTIMISED_TRIVIAL_AND_OR
		
SSAStatement *IRBinaryExpression::EmitSSAForm(SSABuilder &bldr) const
{
	using namespace BinaryOperator;

	BinaryOperator::EBinaryOperator type = Type;
	switch (type) {
		// With logical operations we have short circuiting so we need to do something different for these
		case Logical_Or: {
#ifdef OPTIMISED_TRIVIAL_AND_OR
			// do a quick check to see if we can trivially lower this to a bitwise-or-and-compare-not-equal-zero
			if (IsTrivial()) {
				SSAStatement *left = Left->EmitSSAForm(bldr);
				SSAStatement *right = Right->EmitSSAForm(bldr);

				SSAConstantStatement *left_constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(0), left->GetType());
				SSAConstantStatement *right_constant;
				if (left->GetType() == right->GetType()) {
					right_constant = left_constant;
				} else {
					right_constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(0), right->GetType());
				}

				left = new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, left_constant, BinaryOperator::Inequality);
				right = new SSABinaryArithmeticStatement(&bldr.GetBlock(), right, right_constant, BinaryOperator::Inequality);

				return new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, right, BinaryOperator::Bitwise_Or);
#else
					if(0) {
#endif
			} else {

				// first, evaluate the left hand side. If it is true, return the true value, otherwise
				// return the evaluation of the right hand side
				SSAStatement *left = Left->EmitSSAForm(bldr);

				SSASymbol *tempSymbol = bldr.GetTemporarySymbol(left->GetType());
				auto lvws = new SSAVariableWriteStatement(&bldr.GetBlock(), tempSymbol, left);
				lvws->SetDiag(Diag());

				SSABlock *after_block = new SSABlock(bldr);
				SSABlock *right_block = new SSABlock(bldr);

				auto ifs = new SSAIfStatement(&bldr.GetBlock(), left, *after_block, *right_block);
				ifs->SetDiag(Diag());

				bldr.ChangeBlock(*right_block, false);
				SSAStatement *right = Right->EmitSSAForm(bldr);

				auto rvws = new SSAVariableWriteStatement(&bldr.GetBlock(), tempSymbol, right);
				rvws->SetDiag(Diag());

				auto jmp = new SSAJumpStatement(&bldr.GetBlock(), *after_block);
				jmp->SetDiag(Diag());

				bldr.ChangeBlock(*after_block, false);
				SSAStatement *read = new SSAVariableReadStatement(&bldr.GetBlock(), tempSymbol);
				read->SetDiag(Diag());

				return read;
			}
		}
		case Logical_And: {
			// do a quick check to see if we can trivially lower this to a bitwise-and-and-compare-not-equal-zero
#ifdef OPTIMISED_TRIVIAL_AND_OR
			if (IsTrivial()) {
				SSAStatement *left = Left->EmitSSAForm(bldr);
				SSAStatement *right = Right->EmitSSAForm(bldr);

				SSAConstantStatement *left_constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(0), left->GetType());
				SSAConstantStatement *right_constant;
				if (left->GetType() == right->GetType()) {
					right_constant = left_constant;
				} else {
					right_constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(0), right->GetType());
				}

				left = new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, left_constant, BinaryOperator::Inequality);
				right = new SSABinaryArithmeticStatement(&bldr.GetBlock(), right, right_constant, BinaryOperator::Inequality);

				return new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, right, BinaryOperator::Bitwise_And);
#else
					if(0) {
#endif
			} else {

				// first, evaluate the left hand side. If it is true, evaluate the right side, otherwise
				// return false
				SSAStatement *left = Left->EmitSSAForm(bldr);
				SSASymbol *tempSymbol = bldr.GetTemporarySymbol(IRTypes::UInt8);

				auto left_constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(0), left->GetType());
				left_constant->SetDiag(Diag());

				left = new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, left_constant, BinaryOperator::Inequality);
				left->SetDiag(Diag());

				assert(left->GetType() == tempSymbol->GetType());

				auto lvws = new SSAVariableWriteStatement(&bldr.GetBlock(), tempSymbol, left);
				lvws->SetDiag(Diag());

				SSABlock *after_block = new SSABlock(bldr);
				SSABlock *right_block = new SSABlock(bldr);

				auto ifs = new SSAIfStatement(&bldr.GetBlock(), left, *right_block, *after_block);
				ifs->SetDiag(Diag());

				bldr.ChangeBlock(*right_block, false);
				SSAStatement *right = Right->EmitSSAForm(bldr);

				auto right_constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(0), right->GetType());
				right_constant->SetDiag(Diag());

				right = new SSABinaryArithmeticStatement(&bldr.GetBlock(), right, right_constant, BinaryOperator::Inequality);
				right->SetDiag(Diag());

				auto rvws = new SSAVariableWriteStatement(&bldr.GetBlock(), tempSymbol, right);
				rvws->SetDiag(Diag());

				auto jmp = new SSAJumpStatement(&bldr.GetBlock(), *after_block);
				jmp->SetDiag(Diag());

				bldr.ChangeBlock(*after_block, false);
				SSAStatement *read = new SSAVariableReadStatement(&bldr.GetBlock(), tempSymbol);
				read->SetDiag(Diag());

				return read;
			}

		}

		// These are handled slightly differently
		case ShiftLeft:
		case ShiftRight:
		case RotateLeft:
		case RotateRight: {
			SSAStatement *left = Left->EmitSSAForm(bldr);
			SSAStatement *right = Right->EmitSSAForm(bldr);

			bool signedshift = left->GetType().Signed;

			IRType maxType = IRType::Resolve(BinaryOperator::Add, left->GetType(), right->GetType());

			if (left->GetType() != maxType) {
				left = new SSACastStatement(&bldr.GetBlock(), maxType, left);
				left->SetDiag(Diag());
			}

			if (right->GetType() != maxType) {
				right = new SSACastStatement(&bldr.GetBlock(), maxType, right);
				right->SetDiag(Diag());
			}

			SSAStatement *shift_statement;
			if (type == ShiftRight && signedshift) {
				shift_statement = new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, right, SignedShiftRight);
			} else {
				shift_statement = new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, right, type);
			}

			shift_statement->SetDiag(Diag());
			return shift_statement;
		}

		case Bitwise_Or:
		case Bitwise_And:
		case Bitwise_XOR:
		case Equality:
		case Inequality:
		case LessThan:
		case GreaterThan:
		case LessThanEqual:
		case GreaterThanEqual:
		case Add:
		case Subtract:
		case Multiply:
		case Divide:
		case Modulo: {
Standard_Operator:
			SSAStatement *left = Left->EmitSSAForm(bldr);
			SSAStatement *right = Right->EmitSSAForm(bldr);

			// pretend we have just an addition since we want to promote the types, not find the resultant type
			IRType maxType = IRType::Resolve(BinaryOperator::Add, left->GetType(), right->GetType());

			// check that the types are begin correctly promoted (update: it's wrong to fail here since we might operate between short signed and long unsigned types.
			// assert((left->GetType() == maxType) || (right->GetType() == maxType));

			if (left->GetType() != maxType) {
				left = new SSACastStatement(&bldr.GetBlock(), maxType, left);
				left->SetDiag(Diag());
			}

			if (left->GetType().VectorWidth > 1 && right->GetType().VectorWidth == 1) {
				assert(maxType.VectorWidth > 1);

				// If this comparison is between a vector and a scalar, then the RHS should be cast to the
				// element type of the vector.
				if (right->GetType() != maxType.GetElementType()) {
					right = new SSACastStatement(&bldr.GetBlock(), maxType.GetElementType(), right);
					right->SetDiag(Diag());
				}
			} else {
				if (right->GetType() != maxType) {
					right = new SSACastStatement(&bldr.GetBlock(), maxType, right);
					right->SetDiag(Diag());
				}
			}

			SSAStatement *stmt = new SSABinaryArithmeticStatement(&bldr.GetBlock(), left, right, type);
			stmt->SetDiag(Diag());

			return stmt;
		}
		default: {
			// We have a set operation
			// Right now, do not support any pointer ops, just writes to variables

			// Do we have just a vanilla set operation?

			SSAStatement *value;
			if (Type != Set) {
				// We have an op-and-set operation
				// TODO: LEAK HERE
				IRBinaryExpression *innerExpr = new IRBinaryExpression(GetScope());
				innerExpr->Type = SetOpToNonSetOp(Type);
				innerExpr->Left = Left;
				innerExpr->Right = Right;

				value = innerExpr->EmitSSAForm(bldr);
			} else {
				value = Right->EmitSSAForm(bldr);
			}

			if (IRVariableExpression * var = dynamic_cast<IRVariableExpression *> (Left)) {
				// We're writing to a variable - emit a variable write statement
				SSASymbol *sym = bldr.GetSymbol(var->Symbol);
				// we might need to cast the variable first
				if (sym->GetType() != value->GetType()) {
					IRType targettype = sym->GetType();
					// do not try and cast to a reference
					targettype.Reference = false;

					value = new SSACastStatement(&bldr.GetBlock(), targettype, value);
					value->SetDiag(Diag());
				}

				auto stmt = new SSAVariableWriteStatement(&bldr.GetBlock(), sym, value);
				stmt->SetDiag(Diag());

				return stmt;
			} else if (IRDefineExpression * var = dynamic_cast<IRDefineExpression *> (Left)) {
				// We're also writing to a variable (but we get its name from somewhere else)
				// first we should emit the variable allocation
				SSASymbol *sym = bldr.GetSymbol(var->GetSymbol());
				//            SSAStatement *alloc = new SSAAllocRegisterStatement(&bldr.GetBlock(), this, sym);
				// we might need to cast the variable first
				if (sym->GetType() != value->GetType()) {
					value = new SSACastStatement(&bldr.GetBlock(), sym->GetType(), value);
					value->SetDiag(Diag());
				}

				auto stmt = new SSAVariableWriteStatement(&bldr.GetBlock(), sym, value);
				stmt->SetDiag(Diag());

				return stmt;
			} else if (IRUnaryExpression * var = dynamic_cast<IRUnaryExpression *> (Left)) {
				switch (var->Type) {
						using namespace IRUnaryOperator;
					case Member: {
						assert(false && "Unimplemented");
					}
					case Index: {
						IRVariableExpression *variable = dynamic_cast<IRVariableExpression*> (var->BaseExpression);
						assert(variable);

						SSAStatement *indexStatement = var->Arg->EmitSSAForm(bldr);
						SSAStatement *baseStatement = var->BaseExpression->EmitSSAForm(bldr);

						if (value->GetType() != variable->Symbol->Type.GetElementType()) {
							value = new SSACastStatement(&bldr.GetBlock(), variable->Symbol->Type.GetElementType(), value);
							value->SetDiag(Diag());
						}

						if (indexStatement->GetType() != IRTypes::Int32) {
							indexStatement = new SSACastStatement(&bldr.GetBlock(), IRTypes::Int32, indexStatement);
							indexStatement->SetDiag(Diag());
						}

						SSAStatement *new_vector = new SSAVectorInsertElementStatement(&bldr.GetBlock(), baseStatement, indexStatement, value);
						new_vector->SetDiag(Diag());

						auto stmt = new SSAVariableWriteStatement(&bldr.GetBlock(), bldr.GetSymbol(variable->Symbol), new_vector);
						stmt->SetDiag(Diag());

						return stmt;
					}
					case BitSequence: {
						IRVariableExpression *variable = dynamic_cast<IRVariableExpression*> (var->BaseExpression);
						assert(variable);

						SSAStatement *baseStatement = var->BaseExpression->EmitSSAForm(bldr);
						SSAStatement *faeStatement = var->Arg->EmitSSAForm(bldr);
						SSAStatement *taeStatement = var->Arg2->EmitSSAForm(bldr);

						SSAStatement *bit_deposit = new SSABitDepositStatement(&bldr.GetBlock(), baseStatement, faeStatement, taeStatement, value);
						bit_deposit->SetDiag(Diag());

						auto stmt = new SSAVariableWriteStatement(&bldr.GetBlock(), bldr.GetSymbol(variable->Symbol), bit_deposit);
						stmt->SetDiag(Diag());

						return stmt;
					}
					default:
						assert(false && "Unary operator not usable as lvalue");
				}
			} else {
				assert(false && "Unsupported l-value");
			}
		}
	}
	assert(false && "Unhandled binary operation");
	UNEXPECTED;
}

SSAStatement *IRUnaryExpression::EmitSSAForm(SSABuilder &bldr) const
{

	switch (Type) {
			using namespace IRUnaryOperator;
		case Positive: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			return stmt;
		}
		case Negative: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			auto uas = new SSAUnaryArithmeticStatement(&bldr.GetBlock(), stmt, SSAUnaryOperator::OP_NEGATIVE);
			uas->SetDiag(Diag());

			return uas;
		}
		case Negate: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			auto uas = new SSAUnaryArithmeticStatement(&bldr.GetBlock(), stmt, SSAUnaryOperator::OP_NEGATE);
			uas->SetDiag(Diag());

			return uas;
		}
		case Complement: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			auto uas = new SSAUnaryArithmeticStatement(&bldr.GetBlock(), stmt, SSAUnaryOperator::OP_COMPLEMENT);
			uas->SetDiag(Diag());

			return uas;
		}
		case Preincrement: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			SSAVariableReadStatement *load = static_cast<SSAVariableReadStatement *> (stmt);

			auto constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(1), stmt->GetType());
			constant->SetDiag(Diag());

			auto inc = new SSABinaryArithmeticStatement(&bldr.GetBlock(), load, constant, BinaryOperator::Add);
			inc->SetDiag(Diag());

			auto write = new SSAVariableWriteStatement(&bldr.GetBlock(), load->Target(), inc);
			write->SetDiag(Diag());

			return inc;
		}
		case Predecrement: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			SSAVariableReadStatement *load = static_cast<SSAVariableReadStatement *> (stmt);

			auto constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(1), stmt->GetType());
			constant->SetDiag(Diag());

			auto dec = new SSABinaryArithmeticStatement(&bldr.GetBlock(), load, constant, BinaryOperator::Subtract);
			dec->SetDiag(Diag());

			auto write = new SSAVariableWriteStatement(&bldr.GetBlock(), load->Target(), dec);
			write->SetDiag(Diag());

			return dec;
		}
		case Postincrement: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			SSAVariableReadStatement *load = static_cast<SSAVariableReadStatement *> (stmt);

			auto constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(1), stmt->GetType());
			constant->SetDiag(Diag());

			auto inc = new SSABinaryArithmeticStatement(&bldr.GetBlock(), load, constant, BinaryOperator::Add);
			inc->SetDiag(Diag());

			auto write = new SSAVariableWriteStatement(&bldr.GetBlock(), load->Target(), inc);
			write->SetDiag(Diag());

			return load;
		}
		case Postdecrement: {
			SSAStatement *stmt = BaseExpression->EmitSSAForm(bldr);
			SSAVariableReadStatement *load = static_cast<SSAVariableReadStatement *> (stmt);

			auto constant = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(1), stmt->GetType());
			constant->SetDiag(Diag());

			auto dec = new SSABinaryArithmeticStatement(&bldr.GetBlock(), load, constant, BinaryOperator::Subtract);
			dec->SetDiag(Diag());

			auto write = new SSAVariableWriteStatement(&bldr.GetBlock(), load->Target(), dec);
			write->SetDiag(Diag());

			return load;
		}
		case Member: {
			/*
			 //Structs are passed around by reference, so we can load a member of a struct by:
			 // 1. Getting the offset of the member we are trying to load
			 SSAConstantStatement *offset = new SSAConstantStatement(&bldr.GetBlock(), this, BaseExpression->EvaluateType().BaseType.StructType->GetOffset(MemberStr), IRTypes::UInt32);
			 // 2. Adding the offset to the struct pointer
			 SSABinaryArithmeticStatement *addr = new SSABinaryArithmeticStatement(&bldr.GetBlock(), this, *offset, *stmt, BinaryOperator::Add);

			 IRType memberType = BaseExpression->EvaluateType().BaseType.StructType->GetMemberType(MemberStr);
			 // 3. Cast the pointer to the correct type
			 SSACastStatement *cast = new SSACastStatement(&bldr.GetBlock(), this, IRType::Ptr(memberType), *addr);
			 // 4. Load the pointer value
			 SSAHostMemoryStatement *load = new SSAHostMemoryStatement(&bldr.GetBlock(), this, *cast, SSAHostMemoryStatement::MemoryRead, memberType);

			 if (BaseExpression->EvaluateType().Const) load->IsConstifiable = true;

			 return load;
			 */
			IRVariableExpression *var = dynamic_cast<IRVariableExpression *> (BaseExpression);
			assert(var);

			SSASymbol *structSym = bldr.GetSymbol(var->Symbol);

			auto stmt = new SSAReadStructMemberStatement(&bldr.GetBlock(), structSym, MemberStr, -1);
			stmt->SetDiag(Diag());

			return stmt;
		}

		case Index: {
			IRVariableExpression *variable = dynamic_cast<IRVariableExpression*> (BaseExpression);
			assert(variable);

			SSAVariableReadStatement *ssa_var = (SSAVariableReadStatement*) variable->EmitSSAForm(bldr);
			SSAStatement *index = Arg->EmitSSAForm(bldr);

			auto stmt = new SSAVectorExtractElementStatement(&bldr.GetBlock(), ssa_var, index);
			stmt->SetDiag(Diag());

			return stmt;
		}

		case BitSequence: {
			IRVariableExpression *variable = dynamic_cast<IRVariableExpression*> (BaseExpression);
			assert(variable);

			SSAStatement *baseStatement = BaseExpression->EmitSSAForm(bldr);
			SSAStatement *faeStatement = Arg->EmitSSAForm(bldr);
			SSAStatement *taeStatement = Arg2->EmitSSAForm(bldr);

			SSAStatement *bit_extract = new SSABitExtractStatement(&bldr.GetBlock(), baseStatement, faeStatement, taeStatement);
			bit_extract->SetDiag(Diag());

			return bit_extract;
		}
		default: {
			assert(false && "Unary operator unimplemented");
			UNEXPECTED;
		}
	}
}

SSAStatement *IRVariableExpression::EmitSSAForm(SSABuilder &bldr) const
{
	if (Symbol->SType == Symbol_Constant) {
		// we have a global constant read, so emit a constant
		std::pair<IRSymbol *, uint32_t> constantValue = GetScope().GetContainingAction().Context.GetConstant(name_);

		SSAStatement *stmt = new SSAConstantStatement(&bldr.GetBlock(), IRConstant::Integer(constantValue.second), constantValue.first->Type);
		stmt->SetDiag(Diag());

		return stmt;
	} else {
		SSASymbol *sym = nullptr;
		sym = bldr.GetSymbol(Symbol);

		// In this context, we are reading the given variable

		while (sym->IsReference()) sym = sym->GetReferencee();
		assert(sym);

		SSAStatement *stmt = new SSAVariableReadStatement(&bldr.GetBlock(), sym);
		stmt->SetDiag(Diag());

		return stmt;

	}
}

#define INLINE_ALL_CALLS

SSAStatement *IRCallExpression::EmitSSAForm(SSABuilder &bldr) const
{
	// First we need to determine if we are using an intrinsic l(i.e. memory or register operation)

	if (dynamic_cast<IRExternalAction *> (Target) != nullptr) {
		return EmitExternalCall(bldr, GetTarget().Context.Arch);
	} else if (dynamic_cast<IRIntrinsicAction *> (Target) != nullptr) {
		return EmitIntrinsicCall(bldr, GetTarget().Context.Arch);
	} else if (dynamic_cast<IRHelperAction *> (Target) != nullptr) {
		return EmitHelperCall(bldr, GetTarget().Context.Arch);
	} else {
		throw std::logic_error("Cannot emit SSA form for unknown IRCallableAction type");
	}
}

ssa::SSAStatement* IRCallExpression::EmitHelperCall(ssa::SSABuilder& bldr, const gensim::arch::ArchDescription&) const
{
	std::vector<SSAValue *> args;
	int i = 0;
	for (std::vector<IRExpression *>::const_iterator ci = Args.begin(); ci != Args.end(); ++ci) {
		if (Target->GetSignature().GetParams().at(i).GetType().Reference) {
			IRVariableExpression *read = dynamic_cast<IRVariableExpression*> (*ci);
			if (!read) {
				throw std::logic_error("");
			}

			args.push_back(bldr.GetSymbol(read->Symbol));
		} else {
			SSAStatement *stmt = (*ci)->EmitSSAForm(bldr);
			IRType paramType = Target->GetSignature().GetParams().at(i).GetType();
			if (stmt->GetType() != paramType) {
				assert(!paramType.Reference);
				stmt = new SSACastStatement(&bldr.GetBlock(), paramType, stmt);
				stmt->SetDiag(Diag());
			}
			args.push_back(stmt);
		}
		i++;
	}

	SSACallStatement *call = new SSACallStatement(&bldr.GetBlock(), Target->GetSSAForm(bldr.Context), args);
	call->SetDiag(Diag());

	return call;
}

SSAStatement *IRCastExpression::EmitSSAForm(SSABuilder &bldr) const
{
	// if we're casting a const, just emit a const of the cast type
	IRConstExpression *const_stmt = dynamic_cast<IRConstExpression*> (Expr);
	if (const_stmt != nullptr) {
		if (!const_stmt->Type.IsFloating() && !ToType.IsFloating()) {
			SSAConstantStatement *stmt = new SSAConstantStatement(&bldr.GetBlock(), IRType::Cast(const_stmt->Value, const_stmt->Type, ToType), ToType);
			stmt->SetDiag(Diag());
			return stmt;
		}
	}

	SSAStatement *inner_stmt = Expr->EmitSSAForm(bldr);
	IRType::PromoteResult res = inner_stmt->GetType().AutoPromote(ToType);
	SSACastStatement::CastType cast_type;
			SSACastStatement::CastOption cast_option = SSACastStatement::Option_None;

			
	switch (res) {
		case IRType::PROMOTE_TRUNCATE:
			cast_type = SSACastStatement::Cast_Truncate;
			break;
		case IRType::PROMOTE_CONVERT:
			cast_type = SSACastStatement::Cast_Convert;
			cast_option = SSACastStatement::Option_RoundDefault;
			break;
		case IRType::PROMOTE_OK:
			cast_type = SSACastStatement::Cast_ZeroExtend;
			break;
		case IRType::PROMOTE_SIGN_CHANGE:
			cast_type = SSACastStatement::Cast_ZeroExtend;
			break;
		case IRType::PROMOTE_OK_SIGNED:
			cast_type = SSACastStatement::Cast_SignExtend;
			break;
		default:
			assert(false && "Attempting to generate SSA code for invalid cast type");
			UNEXPECTED;
	}

	auto stmt = new SSACastStatement(&bldr.GetBlock(), ToType, (inner_stmt), cast_type);
			stmt->SetOption(cast_option);
	stmt->SetDiag(Diag());

	return stmt;
}

SSAStatement *IRConstExpression::EmitSSAForm(SSABuilder &bldr) const
{
	auto stmt = new SSAConstantStatement(&bldr.GetBlock(), Value, Type);
	stmt->SetDiag(Diag());

	return stmt;
}

SSAStatement *EmptyExpression::EmitSSAForm(SSABuilder &bldr) const
{
	return NULL;
}

SSAStatement *IRTernaryExpression::EmitSSAForm(ssa::SSABuilder &bldr) const
{
	// If this ternary expression is sufficiently simple, we can emit it as
	// a select statement rather than emit messy control flow
	if (Cond->IsTrivial() && Left->IsTrivial() && Right->IsTrivial()) {
		SSAStatement *condition = Cond->EmitSSAForm(bldr);
		SSAStatement *left = Left->EmitSSAForm(bldr);
		SSAStatement *right = Right->EmitSSAForm(bldr);

		auto stmt = new SSASelectStatement(&bldr.GetBlock(), condition, left, right);
		stmt->SetDiag(Diag());

		return stmt;
	} else {
		// first, emit condition
		SSAStatement *condition = Cond->EmitSSAForm(bldr);

		// Emit blocks for left and right
		SSABlock *left = new SSABlock(bldr);
		SSABlock *right = new SSABlock(bldr);
		SSABlock *after = new SSABlock(bldr);

		SSASymbol *temp_reg = bldr.GetTemporarySymbol(Left->EvaluateType());

		auto ifs = new SSAIfStatement(&bldr.GetBlock(), condition, *left, *right);
		ifs->SetDiag(Diag());

		bldr.ChangeBlock(*left, false);
		SSAStatement *leftExpr = Left->EmitSSAForm(bldr);
		auto lvws = new SSAVariableWriteStatement(&bldr.GetBlock(), temp_reg, leftExpr);
		lvws->SetDiag(Diag());

		auto ljmp = new SSAJumpStatement(&bldr.GetBlock(), *after);
		ljmp->SetDiag(Diag());

		bldr.ChangeBlock(*right, false);
		SSAStatement *rightExpr = Right->EmitSSAForm(bldr);
		auto rvws = new SSAVariableWriteStatement(&bldr.GetBlock(), temp_reg, rightExpr);
		rvws->SetDiag(Diag());

		auto rjmp = new SSAJumpStatement(&bldr.GetBlock(), *after);
		rjmp->SetDiag(Diag());

		bldr.ChangeBlock(*after, false);

		SSAVariableReadStatement *stmt = new SSAVariableReadStatement(&bldr.GetBlock(), temp_reg);
		stmt->SetDiag(Diag());

		return stmt;
	}
}

SSAStatement *IRDefineExpression::EmitSSAForm(SSABuilder &bldr) const
{
	assert(Resolved);
	assert(symbol);
	// Symbol should already be in the symbol table.

	return NULL;
}

SSAStatement *IRFlowStatement::EmitSSAForm(SSABuilder &bldr) const
{
	switch (Type) {
		case FLOW_RETURN_VALUE: {
			// first, calculate the return value
			SSAStatement *rval = (Expr->EmitSSAForm(bldr));
			if (rval->GetType() != bldr.Target->GetPrototype().ReturnType()) {
				rval = new SSACastStatement(&bldr.GetBlock(), bldr.Target->GetPrototype().ReturnType(), rval);
				rval->SetDiag(Diag());
			}

			auto stmt = new SSAReturnStatement(&bldr.GetBlock(), rval);
			stmt->SetDiag(Diag());

			return stmt;
		}
		case FLOW_RETURN_VOID: {
			auto stmt = new SSAReturnStatement(&bldr.GetBlock(), NULL);
			stmt->SetDiag(Diag());

			return stmt;
		}
		case FLOW_RAISE: {
			auto stmt = new SSARaiseStatement(&bldr.GetBlock(), NULL);
			stmt->SetDiag(Diag());

			return stmt;
		}
		case FLOW_BREAK: {
			// Jump to the top of the break stack
			auto stmt = new SSAJumpStatement(&bldr.GetBlock(), bldr.PeekBreak());
			stmt->SetDiag(Diag());

			return stmt;
		}
		case FLOW_CONTINUE: {
			auto stmt = new SSAJumpStatement(&bldr.GetBlock(), bldr.PeekCont());
			stmt->SetDiag(Diag());

			return stmt;
		}
		default: {
			assert(false && "Unrecognized flow statement");
			UNEXPECTED;
		}
	}
}

SSAStatement *IRSelectionStatement::EmitSSAForm(SSABuilder &bldr) const
{
	if (Type == IRSelectionStatement::SELECT_IF) {
		SSAStatement *ssa_expr = Expr->EmitSSAForm(bldr);

		SSABlock *true_block = new SSABlock(bldr);
		SSABlock *end_block = new SSABlock(bldr);
		SSABlock *false_block = ElseBody == NULL ? end_block : new SSABlock(bldr);

		auto ifs = new SSAIfStatement(&bldr.GetBlock(), ssa_expr, *true_block, *false_block);
		ifs->SetDiag(Diag());

		bldr.ChangeBlock(*true_block, false);
		Body->EmitSSAForm(bldr);
		if (bldr.GetBlock().GetControlFlow() == nullptr) {
			bldr.EmitBranch(*end_block, *Body);
		}

		if (ElseBody != NULL) {
			// First, emit the else block
			bldr.ChangeBlock(*false_block, false);
			ElseBody->EmitSSAForm(bldr);
			if (bldr.GetBlock().GetControlFlow() == nullptr) {
				bldr.EmitBranch(*end_block, *ElseBody);
			}
		}
		bldr.ChangeBlock(*end_block, true);
		return NULL;
	} else if (Type == IRSelectionStatement::SELECT_SWITCH) {
		SSAStatement *ssa_expr = Expr->EmitSSAForm(bldr);

		// Emit the after block
		SSABlock *after_block = new SSABlock(bldr);
		SSABlock *def_block = after_block;

		bldr.PushBreak(*after_block);

		// Get a reference to the current block so that we can emit the switch statement expressions
		SSABlock &curr_block = bldr.GetBlock();

		IRBody *switchBody = (IRBody *) Body;

		std::map<SSAStatement *, SSABlock *> cases;

		//        fprintf(stderr, "For switch statement %p\n", this);
		// Get a map of switch cases and emit blocks for them
		for (std::vector<IRStatement *>::const_iterator ci = switchBody->Statements.begin(); ci != switchBody->Statements.end(); ++ci) {
			assert(is((*ci), IRFlowStatement *));
			IRFlowStatement *caseStmt = (IRFlowStatement *) (*ci);
			if (caseStmt->Type == IRFlowStatement::FLOW_CASE) {
				bldr.ChangeBlock(curr_block, false);
				SSAStatement *caseExpr = caseStmt->Expr->EmitSSAForm(bldr);

				SSABlock *caseBlock = new SSABlock(bldr);
				bldr.ChangeBlock(*caseBlock, false);
				caseStmt->Body->EmitSSAForm(bldr);

				cases[caseExpr] = caseBlock;
				//            fprintf(stderr, "Emitting case statement %p -> %p\n", caseExpr, caseBlock);
			} else if (caseStmt->Type == IRFlowStatement::FLOW_DEFAULT) {
				def_block = new SSABlock(bldr);
				bldr.ChangeBlock(*def_block, false);
				caseStmt->Body->EmitSSAForm(bldr);
				if (!def_block->GetControlFlow()) {
					auto jmp = new SSAJumpStatement(&bldr.GetBlock(), *after_block);
					jmp->SetDiag(Diag());
				}

				//            fprintf(stderr, "Emitting default block %p\n", def_block);
			}
		}

		// Emit the switch statement itself
		SSASwitchStatement *switchStmt = new SSASwitchStatement(&curr_block, ssa_expr, def_block);
		switchStmt->SetDiag(Diag());

		// Add values to the switch statement itself
		for (std::map<SSAStatement *, SSABlock *>::const_iterator ci = cases.begin(); ci != cases.end(); ++ci) {
			switchStmt->AddValue((ci->first), (ci->second));
		}

		bldr.PopBreak();

		// Switch to the post-switch block
		bldr.ChangeBlock(*after_block, false);

		return switchStmt;
	}
	assert(false && "Unhandled select operation");
	UNEXPECTED;
}

SSAStatement *IRIterationStatement::EmitSSAForm(SSABuilder &bldr) const
{
	switch (Type) {
		case ITERATE_FOR: {
			// for(begin; check; end)
			//    body
			// after

			// Create the blocks
			SSABlock *check_block = new SSABlock(bldr);
			SSABlock *body_block = new SSABlock(bldr);
			SSABlock *after_block = new SSABlock(bldr);

			// Emit the begin statement
			For_Expr_Start->EmitSSAForm(bldr);

			// Emit the contents of the check block
			bldr.EmitBranch(*check_block, *For_Expr_Check);
			bldr.ChangeBlock(*check_block, false);
			SSAStatement *check_expr = For_Expr_Check->EmitSSAForm(bldr);
			auto ifs = new SSAIfStatement(&bldr.GetBlock(), check_expr, *body_block, *after_block);
			ifs->SetDiag(Diag());

			// Emit the body
			bldr.PushBreak(*after_block);
			bldr.PushCont(*check_block);
			bldr.ChangeBlock(*body_block, false);
			Body->EmitSSAForm(bldr);

			// Emit the end statement
			Expr->EmitSSAForm(bldr);

			// Emit the jump back to the check block
			auto jmp = new SSAJumpStatement(&bldr.GetBlock(), *check_block);
			jmp->SetDiag(Diag());

			// Change to the after block and return.
			bldr.ChangeBlock(*after_block, false);
			bldr.PopBreak();
			bldr.PopCont();

			break;
		}
		case ITERATE_WHILE: {
			// while(statement)
			//    body
			// after

			// Create the blocks
			SSABlock *check_block = new SSABlock(bldr);
			SSABlock *body_block = new SSABlock(bldr);
			SSABlock *after_block = new SSABlock(bldr);

			// Emit the check
			bldr.EmitBranch(*check_block, *Expr);
			bldr.ChangeBlock(*check_block, false);
			SSAStatement *check_expr = Expr->EmitSSAForm(bldr);
			auto ifs = new SSAIfStatement(&bldr.GetBlock(), check_expr, *body_block, *after_block);
			ifs->SetDiag(Diag());

			// Emit the body
			bldr.PushBreak(*after_block);
			bldr.PushCont(*check_block);
			bldr.ChangeBlock(*body_block, false);

			Body->EmitSSAForm(bldr);

			// Emit the jump back to the check block
			auto jmp = new SSAJumpStatement(&bldr.GetBlock(), *check_block);
			jmp->SetDiag(Diag());

			bldr.ChangeBlock(*after_block, false);
			bldr.PopBreak();
			bldr.PopCont();
			break;
		}
		case ITERATE_DO_WHILE: {
			// do
			//  body
			// while(stmt)
			// after

			// Create the blocks
			SSABlock *body_block = new SSABlock(bldr);
			SSABlock *after_block = new SSABlock(bldr);

			// Emit the body
			bldr.EmitBranch(*body_block, *Body);
			bldr.ChangeBlock(*body_block, false);
			bldr.PushBreak(*after_block);
			bldr.PushCont(*body_block);
			Body->EmitSSAForm(bldr);

			// Emit the check
			SSAStatement *check_stmt = Expr->EmitSSAForm(bldr);
			auto ifs = new SSAIfStatement(&bldr.GetBlock(), check_stmt, *body_block, *after_block);
			ifs->SetDiag(Diag());

			// Change to the after block
			bldr.ChangeBlock(*after_block, false);
			bldr.PopBreak();
			bldr.PopCont();
			break;
		}
		default:
			assert(false && "Unrecognized iteration type");
	}
	return NULL;
}

SSAStatement *IRExpressionStatement::EmitSSAForm(SSABuilder &bldr) const
{
	return Expr->EmitSSAForm(bldr);
}

SSAStatement *IRBody::EmitSSAForm(SSABuilder &bldr) const
{
	for (std::vector<IRStatement *>::const_iterator ci = Statements.begin(); ci != Statements.end(); ++ci) {
		assert((*ci)->Resolved);
		(*ci)->EmitSSAForm(bldr);
	}

	return NULL;
}
