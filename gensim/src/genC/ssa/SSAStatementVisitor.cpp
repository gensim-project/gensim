/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSAStatementVisitor.h"
#include "genC/ssa/statement/SSAStatements.h"

using namespace gensim::genc::ssa;

SSAStatementVisitor::~SSAStatementVisitor()
{

}

void HierarchicalSSAStatementVisitor::VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt)
{
	stmt.LHS()->Accept(*this);
	stmt.RHS()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitBitDepositStatement(SSABitDepositStatement& stmt)
{
	stmt.BitFrom()->Accept(*this);
	stmt.BitTo()->Accept(*this);
	stmt.Value()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitBitExtractStatement(SSABitExtractStatement& stmt)
{
	stmt.BitFrom()->Accept(*this);
	stmt.BitTo()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitCallStatement(SSACallStatement& call_stmt)
{
	for (unsigned int i = 0; i < call_stmt.ArgCount(); i++) {
		if (auto arg_stmt = dynamic_cast<SSAStatement *>(call_stmt.Arg(i))) {
			arg_stmt->Accept(*this);
		}
	}
}

void HierarchicalSSAStatementVisitor::VisitCastStatement(SSACastStatement& stmt)
{
	stmt.Expr()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitConstantStatement(SSAConstantStatement& stmt)
{

}

void HierarchicalSSAStatementVisitor::VisitControlFlowStatement(SSAControlFlowStatement& stmt)
{

}

void HierarchicalSSAStatementVisitor::VisitDeviceReadStatement(SSADeviceReadStatement& stmt)
{
	stmt.Device()->Accept(*this);
	stmt.Address()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitIfStatement(SSAIfStatement& stmt)
{
	stmt.Expr()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitIntrinsicStatement(SSAIntrinsicStatement& stmt)
{
	for (unsigned int i = 0; i < stmt.ArgCount(); i++) {
		if (auto arg_stmt = dynamic_cast<SSAStatement *>(stmt.Args(i))) {
			arg_stmt->Accept(*this);
		}
	}
}

void HierarchicalSSAStatementVisitor::VisitJumpStatement(SSAJumpStatement& stmt)
{

}

void HierarchicalSSAStatementVisitor::VisitMemoryReadStatement(SSAMemoryReadStatement& stmt)
{
	stmt.Addr()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt)
{
	stmt.Addr()->Accept(*this);
	stmt.Value()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitPhiStatement(SSAPhiStatement& stmt)
{
	assert(false);
}

void HierarchicalSSAStatementVisitor::VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt)
{
}

void HierarchicalSSAStatementVisitor::VisitRegisterStatement(SSARegisterStatement& stmt)
{
	stmt.RegNum()->Accept(*this);
	if (!stmt.IsRead) stmt.Value()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitRaiseStatement(SSARaiseStatement& stmt)
{
}

void HierarchicalSSAStatementVisitor::VisitReturnStatement(SSAReturnStatement& stmt)
{
	if (stmt.Value() != nullptr) {
		stmt.Value()->Accept(*this);
	}
}

void HierarchicalSSAStatementVisitor::VisitSelectStatement(SSASelectStatement& stmt)
{
	stmt.Cond()->Accept(*this);
	stmt.TrueVal()->Accept(*this);
	stmt.FalseVal()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitStatement(SSAStatement& stmt)
{
	//for (unsigned int i = 0; i < stmt.op)
}

void HierarchicalSSAStatementVisitor::VisitSwitchStatement(SSASwitchStatement& stmt)
{
	stmt.Expr()->Accept(*this);

	for (const auto& value : stmt.GetValues()) {
		value.first->Accept(*this);
	}
}

void HierarchicalSSAStatementVisitor::VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt)
{
	stmt.Expr()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitVariableKillStatement(SSAVariableKillStatement& stmt)
{
}

void HierarchicalSSAStatementVisitor::VisitVariableReadStatement(SSAVariableReadStatement& stmt)
{
}

void HierarchicalSSAStatementVisitor::VisitVariableWriteStatement(SSAVariableWriteStatement& stmt)
{
	stmt.Expr()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt)
{
	stmt.Base()->Accept(*this);
	stmt.Index()->Accept(*this);
}

void HierarchicalSSAStatementVisitor::VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt)
{
	stmt.Base()->Accept(*this);
	stmt.Index()->Accept(*this);
	stmt.Value()->Accept(*this);
}

void EmptySSAStatementVisitor::VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitBitDepositStatement(SSABitDepositStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitBitExtractStatement(SSABitExtractStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitCallStatement(SSACallStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitCastStatement(SSACastStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitConstantStatement(SSAConstantStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitControlFlowStatement(SSAControlFlowStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitDeviceReadStatement(SSADeviceReadStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitIfStatement(SSAIfStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitIntrinsicStatement(SSAIntrinsicStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitJumpStatement(SSAJumpStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitMemoryReadStatement(SSAMemoryReadStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitPhiStatement(SSAPhiStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitRegisterStatement(SSARegisterStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitRaiseStatement(SSARaiseStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitReturnStatement(SSAReturnStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitSelectStatement(SSASelectStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitStatement(SSAStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitSwitchStatement(SSASwitchStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitVariableKillStatement(SSAVariableKillStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitVariableReadStatement(SSAVariableReadStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitVariableWriteStatement(SSAVariableWriteStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt)
{

}

void EmptySSAStatementVisitor::VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt)
{

}

