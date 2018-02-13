/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/validation/SSAStatementValidationPass.h"
#include "genC/ssa/statement/SSAStatements.h"

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "arch/ArchDescription.h"

#include "ComponentManager.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

class OperandsInSameBlockValidationPass : public SSAStatementValidationPass {
public:
	void VisitPhiStatement(SSAPhiStatement& stmt) override {
		// nothing to do here since phi nodes are allowed to refer to statements in other blocks
	}
	
	void VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt) override {
		Assert(stmt.LHS()->Parent == stmt.Parent, "Binary arithmetic statement operands must be in same block", stmt.GetDiag());
		Assert(stmt.RHS()->Parent == stmt.Parent, "Binary arithmetic statement operands must be in same block", stmt.GetDiag());
		
		// Special handling for equality operations
		if (stmt.Type == gensim::genc::BinaryOperator::Equality) {
			auto lhs_type = stmt.LHS()->GetType();
			auto rhs_type = stmt.RHS()->GetType();
			
			if (lhs_type.VectorWidth > 1 && rhs_type.VectorWidth > 1) {
				// LHS and RHS are vectors... must be same type.
				Assert(stmt.LHS()->GetType() == stmt.RHS()->GetType(), "Binary arithmetic statement operands must be same type", stmt.GetDiag());
			} else if (lhs_type.VectorWidth > 1 && rhs_type.VectorWidth == 1) {
				// LHS is vector, RHS is scalar... LHS element type must be RHS type
				Assert(stmt.LHS()->GetType().GetElementType() == stmt.RHS()->GetType(), "Binary arithmetic statement operands must be same type", stmt.GetDiag());
			} else if (lhs_type.VectorWidth > 1 && rhs_type.VectorWidth == 1) {
				// LHS is scalar, RHS is vector... RHS element type must be LHS type
				Assert(stmt.LHS()->GetType() == stmt.RHS()->GetType().GetElementType(), "Binary arithmetic statement operands must be same type", stmt.GetDiag());
			} else {
				// LHS and RHS are scalars... must be same type.
				Assert(stmt.LHS()->GetType() == stmt.RHS()->GetType(), "Binary arithmetic statement operands must be same type", stmt.GetDiag());
			}
		} else {
			Assert(stmt.LHS()->GetType() == stmt.RHS()->GetType(), "Binary arithmetic statement operands must be same type", stmt.GetDiag());
		}
	}
	
	void VisitCastStatement(SSACastStatement& stmt) override {
		Assert(stmt.Expr()->Parent == stmt.Parent, "Cast statement expression must be in same block as cast statement", stmt.GetDiag());
	}
	
	void VisitIfStatement(SSAIfStatement& stmt) override {
		Assert(stmt.Expr()->Parent == stmt.Parent, "If statement expression must be in same block as if statement", stmt.GetDiag());
	}

	void VisitMemoryReadStatement(SSAMemoryReadStatement& stmt) override {
		Assert(stmt.Addr()->Parent == stmt.Parent, "Memory read address must be in same block as read statement", stmt.GetDiag());
		Assert(stmt.Addr()->HasValue(), "Memory read address must have a value", stmt.GetDiag());
		
		switch(stmt.Width) {
			case 1:
			case 2:
			case 4:
			case 8:
				break;
			default:
				Fail("Unsupported memory write width", stmt.GetDiag());
				break;
		}
	}

	void VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt) override {
		Assert(stmt.Addr()->Parent == stmt.Parent, "Memory write address must be in same block as read statement", stmt.GetDiag());
		Assert(stmt.Addr()->HasValue(), "Memory write address must have a value", stmt.GetDiag());
		Assert(stmt.Value()->Parent == stmt.Parent, "Memory write address must be in same block as read statement", stmt.GetDiag());
		Assert(stmt.Value()->HasValue(), "Memory write value must have a value", stmt.GetDiag());
		
		switch(stmt.Width) {
			case 1:
			case 2:
			case 4:
			case 8:
				break;
			default:
				Fail("Unsupported memory write width", stmt.GetDiag());
				break;
		}
	}
	
	void VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt) override {
		Assert(stmt.Parent->Parent->Symbols().count(stmt.Target()), "Internal error: Struct must read from a parameter in the current action", stmt.GetDiag());
	}

	void VisitRegisterStatement(SSARegisterStatement& stmt) override {
		auto &arch = stmt.Parent->Parent->GetContext().GetArchDescription();
		bool in_range = false;
		
		if(stmt.IsBanked) {
			Assert(stmt.RegNum()->Parent == stmt.Parent, "Register index must be in same block as register read", stmt.GetDiag());
			in_range = stmt.Bank < arch.GetRegFile().GetBanks().size();
			Assert(in_range, "Register bank index out of range", stmt.GetDiag());
		} else {
			in_range = stmt.Bank < arch.GetRegFile().GetSlots().size();
			Assert(in_range, "Register slot index out of range", stmt.GetDiag());
		}
		
		if(!stmt.IsRead) {
			Assert(stmt.Value()->Parent == stmt.Parent, "Register value must be in same block as register write", stmt.GetDiag());
			
			if(in_range) {
				gensim::genc::IRType regtype;
				if(stmt.IsBanked) {
					regtype = arch.GetRegFile().GetBankByIdx(stmt.Bank).GetRegisterIRType();
				} else {
					regtype = arch.GetRegFile().GetSlotByIdx(stmt.Bank).GetIRType();
				}

				Assert(stmt.Value()->GetType() == regtype, "Written value type must match register type (Expected " + regtype.GetUnifiedType() + ", got " + stmt.Value()->GetType().GetUnifiedType() + ")", stmt.GetDiag());
			}
		}
	}

	
	void VisitReturnStatement(SSAReturnStatement& stmt) override {
		if(stmt.Value() != nullptr) {
			Assert(stmt.Value()->Parent == stmt.Parent, "Return statement operand must be in same block", stmt.GetDiag());
		}
	}
	
	void VisitSelectStatement(SSASelectStatement& stmt) override {
		Assert(stmt.Cond()->Parent == stmt.Parent, "Select statement condition must be in same block", stmt.GetDiag());
		Assert(stmt.TrueVal()->Parent == stmt.Parent, "Select statement true value must be in same block", stmt.GetDiag());
		Assert(stmt.FalseVal()->Parent == stmt.Parent, "Select statement false value must be in same block", stmt.GetDiag());
	}
	
	void VisitSwitchStatement(SSASwitchStatement& stmt) override {
		Assert(stmt.Expr()->Parent == stmt.Parent, "Switch statement expression must be in same block", stmt.GetDiag());
		for(auto value : stmt.GetValues()) {
			Assert(value.first->Parent == stmt.Parent, "Switch statement value must be in same block", stmt.GetDiag());
			Assert(dynamic_cast<SSAConstantStatement*>(value.first) != nullptr, "Switch statement values must be constants", stmt.GetDiag());
		}
		
		const SSASwitchStatement &conststmt = stmt;
		for(unsigned i = 0; i < conststmt.GetOperands().size(); i += 2) {
			// even operands should be statements
			// odd operands should be blocks
//			Assert(dynamic_cast<SSAStatement*>(conststmt.GetOperands().at(i)) != nullptr, "Internal error: Even-numbered switch operands must be statements", stmt.GetDiag());
//			Assert(dynamic_cast<SSABlock*>(conststmt.GetOperands().at(i+1)) != nullptr, "Internal error: odd-numbered switch operands must be blocks", stmt.GetDiag());
		}
		
	}
	
	void VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt) override {
		Assert(stmt.Expr()->Parent == stmt.Parent, "Unary statement expression must be in same block", stmt.GetDiag());
	}

	void VisitVariableWriteStatement(SSAVariableWriteStatement& stmt) override {
		Assert(stmt.Expr()->Parent == stmt.Parent, "Variable write expression must be in same block", stmt.GetDiag());
		Assert(stmt.Expr()->GetType() == stmt.Target()->GetType(), "Variable write expression must match type of variable", stmt.GetDiag());
	}
};

class BinaryArithmeticStatementValidationPass : public SSAStatementValidationPass {
public:
	void VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt) override {
		if(!stmt.LHS()->HasValue()) {
			Fail("Binary arithmetic statement operands must have values", stmt.GetDiag());
		}
		if(!stmt.RHS()->HasValue()) {
			Fail("Binary arithmetic statement operands must have values", stmt.GetDiag());
		}
	}
};

class CastStatementValidationPass : public SSAStatementValidationPass {
	void VisitCastStatement(SSACastStatement& stmt) override {
		if(!stmt.Expr()->HasValue()) {
			Fail("Cast statement operand must have a value", stmt.GetDiag());
		}
	}

};

class IfStatementValidationPass : public SSAStatementValidationPass {
	void VisitIfStatement(SSAIfStatement& stmt) override {
		if(!stmt.Expr()->HasValue()) {
			Fail("If statement expression must have a value", stmt.GetDiag());
		}
		
		Assert(stmt.Expr()->GetType().VectorWidth == 1, "If statement expression cannot be a full vector", stmt.GetDiag());
	}

};

class SelectStatementValidationPass : public SSAStatementValidationPass {
	void VisitSelectStatement(SSASelectStatement& stmt) override {
		if(stmt.Cond() != nullptr) {
			Assert(stmt.Cond()->HasValue(), "Select statement condition must have a value", stmt.GetDiag());
		} else {
			Fail("Select statement must have a condition", stmt.GetDiag());
		}
		
		if(stmt.TrueVal() != nullptr) {
			Assert(stmt.TrueVal()->HasValue(), "Select statement 'true' value must have a value", stmt.GetDiag());
		} else {
			Fail("Select statement must have a 'true' value", stmt.GetDiag());
		}
		
		if(stmt.FalseVal() != nullptr) {
			Assert(stmt.FalseVal()->HasValue(), "Select statement 'false' value must have a value", stmt.GetDiag());
		} else {
			Fail("Select statement must have a 'false' value", stmt.GetDiag());
		}
	}

};

class ReturnStatementValidationPass : public SSAStatementValidationPass {
	void VisitReturnStatement(SSAReturnStatement& stmt) override {
		bool action_returns_a_value = stmt.Parent->Parent->GetPrototype().GetIRSignature().HasReturnValue();
		if(action_returns_a_value) {
			if(stmt.Value() == nullptr) {
				Fail("Must return a value in an action which returns non-void", stmt.GetDiag());
			} else {
				Assert(stmt.Value()->GetType() == stmt.Parent->Parent->GetPrototype().ReturnType(), "Returned value must match type of action", stmt.GetDiag());
			}
		} else {
			Assert(stmt.Value() == nullptr, "Cannot return a value in an action which returns void", stmt.GetDiag());
		}
	}

};



RegisterComponent0(SSAStatementValidationPass, OperandsInSameBlockValidationPass, "OperandsInSameBlockValidationPass", "Check that statement operands are in the same block where necessary")
RegisterComponent0(SSAStatementValidationPass, BinaryArithmeticStatementValidationPass, "BinaryArithmeticStatementValidationPass", "Check that binary arithmetic statements have valid operands")
RegisterComponent0(SSAStatementValidationPass, CastStatementValidationPass, "CastStatementValidationPass", "Basic validation for SSACastStatement")
RegisterComponent0(SSAStatementValidationPass, IfStatementValidationPass, "IfStatementValidationPass", "Basic validation for SSAIfStatement")
RegisterComponent0(SSAStatementValidationPass, ReturnStatementValidationPass, "ReturnStatementValidationPass", "Basic validation for SSAReturnStatement")
RegisterComponent0(SSAStatementValidationPass, SelectStatementValidationPass, "SelectStatementValidationPass", "Basic validation for SSASelectStatement")