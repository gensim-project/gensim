/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SSAInterpreterStatementVisitor.h
 * Author: harry
 *
 * Created on 22 April 2017, 11:51
 */

#ifndef SSAINTERPRETERSTATEMENTVISITOR_H
#define SSAINTERPRETERSTATEMENTVISITOR_H

#include "genC/ssa/SSAStatementVisitor.h"
#include "SSAInterpreter.h"

namespace gensim
{

	namespace arch
	{
		class ArchDescription;
	}

	namespace genc
	{
		namespace ssa
		{
			namespace testing
			{
				class SSAInterpreterStatementVisitor : public SSAStatementVisitor
				{
				public:
					SSAInterpreterStatementVisitor(VMActionState &vmstate, MachineStateInterface &machstate, const arch::ArchDescription *arch);
					virtual ~SSAInterpreterStatementVisitor();

					void VisitStatement(SSAStatement& stmt) override;
					void VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt) override;
					void VisitBitDepositStatement(SSABitDepositStatement& stmt) override;
					void VisitBitExtractStatement(SSABitExtractStatement& stmt) override;
					void VisitCallStatement(SSACallStatement& stmt) override;
					void VisitCastStatement(SSACastStatement& stmt) override;
					void VisitConstantStatement(SSAConstantStatement& stmt) override;
					void VisitControlFlowStatement(SSAControlFlowStatement& stmt) override;
					void VisitDeviceReadStatement(SSADeviceReadStatement& stmt) override;
					void VisitIfStatement(SSAIfStatement& stmt) override;
					void VisitIntrinsicStatement(SSAIntrinsicStatement& stmt) override;
					void VisitJumpStatement(SSAJumpStatement& stmt) override;
					void VisitMemoryReadStatement(SSAMemoryReadStatement& stmt) override;
					void VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt) override;
					void VisitPhiStatement(SSAPhiStatement& stmt) override;
					void VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt) override;
					void VisitRegisterStatement(SSARegisterStatement& stmt) override;
					void VisitRaiseStatement(SSARaiseStatement& stmt) override;
					void VisitReturnStatement(SSAReturnStatement& stmt) override;
					void VisitSelectStatement(SSASelectStatement& stmt) override;
					void VisitSwitchStatement(SSASwitchStatement& stmt) override;
					void VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt) override;
					void VisitVariableKillStatement(SSAVariableKillStatement& stmt) override;
					void VisitVariableReadStatement(SSAVariableReadStatement& stmt) override;
					void VisitVariableWriteStatement(SSAVariableWriteStatement& stmt) override;
					void VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt) override;
					void VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt) override;
					void VisitVectorShuffleStatement(SSAVectorShuffleStatement& stmt) override;


				private:
					const arch::ArchDescription *_arch;
					VMActionState &_vmstate;
					MachineStateInterface &_machine_state;
				};
			}
		}
	}
}

#endif /* SSAINTERPRETERSTATEMENTVISITOR_H */

