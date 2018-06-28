/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IREnums.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSABinaryArithmeticStatement.h"
#include "genC/ssa/statement/SSACastStatement.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAStatement.h"

using namespace gensim::genc::ssa;


SSAConstantStatement *ConstantFoldBinaryOp(SSABinaryArithmeticStatement *binary_stmt)
{
	using namespace gensim::genc;

	SSAConstantStatement *lhs = dynamic_cast<SSAConstantStatement *>(binary_stmt->LHS());
	SSAConstantStatement *rhs = dynamic_cast<SSAConstantStatement *>(binary_stmt->RHS());

	assert(lhs && rhs);

	if(lhs->GetType().IsFloating() || rhs->GetType().IsFloating()) {
		UNIMPLEMENTED;
	}

	uint64_t val = 0;
	switch (binary_stmt->Type) {
		case BinaryOperator::Add:
			val = lhs->Constant.Int() + rhs->Constant.Int();
			break;
		case BinaryOperator::Subtract:
			val = lhs->Constant.Int() - rhs->Constant.Int();
			break;
		case BinaryOperator::Multiply:
			val = lhs->Constant.Int() * rhs->Constant.Int();
			break;
		case BinaryOperator::Divide:
			if(rhs->Constant.Int() == 0) {
				val = 0;
			} else {
				val = lhs->Constant.Int() / rhs->Constant.Int();
			}
			break;
		case BinaryOperator::Modulo:
			if(rhs->Constant.Int() == 0) {
				val = 0;
			} else {
				val = lhs->Constant.Int() % rhs->Constant.Int();
			}
			break;
		case BinaryOperator::ShiftLeft:
			val = lhs->Constant.Int() << rhs->Constant.Int();
			break;
		case BinaryOperator::ShiftRight:
			val = lhs->Constant.Int() >> rhs->Constant.Int();
			break;
		case BinaryOperator::SignedShiftRight:
			val = ((int64_t)lhs->Constant.Int()) >> rhs->Constant.Int();
			break;

		case BinaryOperator::RotateLeft:
			val = IRConstant::ROL(lhs->Constant, rhs->Constant, binary_stmt->GetType().SizeInBytes()*8).Int();
			break;

		case BinaryOperator::RotateRight:
			val = IRConstant::ROR(lhs->Constant, rhs->Constant, binary_stmt->GetType().SizeInBytes()*8).Int();
			break;

		case BinaryOperator::Bitwise_And:
			val = lhs->Constant.Int() & rhs->Constant.Int();
			break;
		case BinaryOperator::Bitwise_Or:
			val = lhs->Constant.Int() | rhs->Constant.Int();
			break;
		case BinaryOperator::Bitwise_XOR:
			val = lhs->Constant.Int() ^ rhs->Constant.Int();
			break;

		case BinaryOperator::Logical_Or:
			val = lhs->Constant.Int() || rhs->Constant.Int();
			break;
		case BinaryOperator::Logical_And:
			val = lhs->Constant.Int() && rhs->Constant.Int();
			break;
		case BinaryOperator::Equality:
			val = lhs->Constant.Int() == rhs->Constant.Int();
			break;
		case BinaryOperator::Inequality:
			val = lhs->Constant.Int() != rhs->Constant.Int();
			break;
		case BinaryOperator::LessThan:
			val = lhs->Constant.Int() < rhs->Constant.Int();
			break;
		case BinaryOperator::LessThanEqual:
			val = lhs->Constant.Int() <= rhs->Constant.Int();
			break;
		case BinaryOperator::GreaterThan:
			val = lhs->Constant.Int() > rhs->Constant.Int();
			break;
		case BinaryOperator::GreaterThanEqual:
			val = lhs->Constant.Int() >= rhs->Constant.Int();
			break;


		default:
			assert(false && "Unimplemented");
			break;
	}

	SSAConstantStatement *newstmt = new SSAConstantStatement(binary_stmt->Parent, IRType::Cast(IRConstant::Integer(val), binary_stmt->GetType(), binary_stmt->GetType()), binary_stmt->GetType(), binary_stmt);
	newstmt->SetDiag(binary_stmt->GetDiag());

	std::set<SSAStatement*> users;
	for(auto i : binary_stmt->GetUses()) {
		SSAStatement *use = dynamic_cast<SSAStatement*>(i);
		if(use) {
			users.insert(use);
		}
	}

	while(users.size()) {
		auto use = *users.begin();
		users.erase(use);
		use->Replace(binary_stmt, newstmt);
	}

	binary_stmt->Parent->RemoveStatement(*binary_stmt);
	binary_stmt->Dispose();
	delete binary_stmt;

	return newstmt;
}

SSAConstantStatement *ConstantFoldCastOp(SSACastStatement *cast_stmt)
{
	using namespace gensim::genc;

	SSAConstantStatement *const_stmt = static_cast<SSAConstantStatement *>(cast_stmt->Expr());
	assert(const_stmt);

	IRConstant end_value = IRType::Cast(const_stmt->Constant, const_stmt->GetType(), cast_stmt->GetType());

	SSAConstantStatement *new_stmt = new SSAConstantStatement(cast_stmt->Parent, end_value, cast_stmt->GetType(), cast_stmt);
	new_stmt->SetDiag(cast_stmt->GetDiag());

	std::set<SSAStatement*> users;
	for(auto i : cast_stmt->GetUses()) {
		SSAStatement *use = dynamic_cast<SSAStatement*>(i);
		if(use) {
			users.insert(use);
		}
	}

	while(users.size()) {
		auto use = *users.begin();
		users.erase(use);
		use->Replace(cast_stmt, new_stmt);
	}

	cast_stmt->Parent->RemoveStatement(*cast_stmt);
	assert(cast_stmt->GetUses().empty());

	cast_stmt->Dispose();
	delete cast_stmt;

	return new_stmt;
}


class ConstantFoldingPass : public SSAPass
{
public:
	virtual ~ConstantFoldingPass()
	{

	}

	bool Run(SSAFormAction& action) const override
	{
		bool change_made = false;

		for(auto *block : action.GetBlocks()) {

			std::list<SSABinaryArithmeticStatement *> bin_candidates;
			std::list<SSACastStatement *> cast_candidates;

			// loop through each statement in the block.
			for(auto &stmt : block->GetStatements()) {

				// If it is a binary statement and its arguments are constant, replace it with a constant
				if (SSABinaryArithmeticStatement *binary_stmt = dynamic_cast<SSABinaryArithmeticStatement *>(stmt)) {
					SSAConstantStatement *lhs = dynamic_cast<SSAConstantStatement *>(binary_stmt->LHS());
					SSAConstantStatement *rhs = dynamic_cast<SSAConstantStatement *>(binary_stmt->RHS());

					if (lhs && rhs) {
						bin_candidates.push_back(binary_stmt);
					}
				}

				// If it is a cast and its argument is a constant, constant fold it
				if (SSACastStatement *cast_stmt = dynamic_cast<SSACastStatement *>(stmt)) {
					if (dynamic_cast<SSAConstantStatement *>(cast_stmt->Expr())) {
						cast_candidates.push_back(cast_stmt);
					}
				}
			}

			// if(!bin_candidates.empty())fprintf(stderr, "[GenC-Opt] Constant folding %u binary statements\n", bin_candidates.size());
			// if(!cast_candidates.empty())fprintf(stderr, "[GenC-Opt] Constant folding %u cast statements\n", cast_candidates.size());

			for (std::list<SSABinaryArithmeticStatement *>::iterator i = bin_candidates.begin(); i != bin_candidates.end(); ++i) {
				SSABinaryArithmeticStatement *binary_stmt = *i;
				ConstantFoldBinaryOp(binary_stmt);
				change_made = true;
			}

			for (std::list<SSACastStatement *>::iterator i = cast_candidates.begin(); i != cast_candidates.end(); ++i) {
				SSACastStatement *cast_stmt = *i;
				if(cast_stmt->GetCastType() != SSACastStatement::Cast_Reinterpret) {
					ConstantFoldCastOp(cast_stmt);
					change_made = true;
				}
			}
		}
		return change_made;
	}

};


RegisterComponent0(SSAPass, ConstantFoldingPass, "ConstantFolding", "Fold constant values and expressions")
