/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "util/Visitor.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAStatement;
			class SSABinaryArithmeticStatement;
			class SSABitDepositStatement;
			class SSABitExtractStatement;
			class SSACallStatement;
			class SSACastStatement;
			class SSAConstantStatement;
			class SSAControlFlowStatement;
			class SSADeviceReadStatement;
			class SSAIfStatement;
			class SSAIntrinsicStatement;
			class SSAJumpStatement;
			class SSAMemoryReadStatement;
			class SSAMemoryWriteStatement;
			class SSAPhiStatement;
			class SSARaiseStatement;
			class SSAReadStructMemberStatement;
			class SSARegisterStatement;
			class SSAReturnStatement;
			class SSASelectStatement;
			class SSASwitchStatement;
			class SSAUnaryArithmeticStatement;
			class SSAVariableKillStatement;
			class SSAVariableReadStatement;
			class SSAVariableWriteStatement;
			class SSAVectorExtractElementStatement;
			class SSAVectorInsertElementStatement;

			class SSAStatementVisitor : public util::Visitor
			{
			public:
				virtual ~SSAStatementVisitor();

				virtual void VisitStatement(SSAStatement& stmt) = 0;
				virtual void VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt) = 0;
				virtual void VisitBitDepositStatement(SSABitDepositStatement& stmt) = 0;
				virtual void VisitBitExtractStatement(SSABitExtractStatement& stmt) = 0;
				virtual void VisitCallStatement(SSACallStatement& stmt) = 0;
				virtual void VisitCastStatement(SSACastStatement& stmt) = 0;
				virtual void VisitConstantStatement(SSAConstantStatement& stmt) = 0;
				virtual void VisitControlFlowStatement(SSAControlFlowStatement& stmt) = 0;
				virtual void VisitDeviceReadStatement(SSADeviceReadStatement& stmt) = 0;
				virtual void VisitIfStatement(SSAIfStatement& stmt) = 0;
				virtual void VisitIntrinsicStatement(SSAIntrinsicStatement& stmt) = 0;
				virtual void VisitJumpStatement(SSAJumpStatement& stmt) = 0;
				virtual void VisitMemoryReadStatement(SSAMemoryReadStatement& stmt) = 0;
				virtual void VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt) = 0;
				virtual void VisitPhiStatement(SSAPhiStatement& stmt) = 0;
				virtual void VisitRaiseStatement(SSARaiseStatement& stmt) = 0;
				virtual void VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt) = 0;
				virtual void VisitRegisterStatement(SSARegisterStatement& stmt) = 0;
				virtual void VisitReturnStatement(SSAReturnStatement& stmt) = 0;
				virtual void VisitSelectStatement(SSASelectStatement& stmt) = 0;
				virtual void VisitSwitchStatement(SSASwitchStatement& stmt) = 0;
				virtual void VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt) = 0;
				virtual void VisitVariableKillStatement(SSAVariableKillStatement& stmt) = 0;
				virtual void VisitVariableReadStatement(SSAVariableReadStatement& stmt) = 0;
				virtual void VisitVariableWriteStatement(SSAVariableWriteStatement& stmt) = 0;
				virtual void VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt) = 0;
				virtual void VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt) = 0;
			};

			class HierarchicalSSAStatementVisitor : public SSAStatementVisitor
			{
			public:
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
			};

			class EmptySSAStatementVisitor : public SSAStatementVisitor
			{
			public:
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
				void VisitRaiseStatement(SSARaiseStatement& stmt) override;
				void VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt) override;
				void VisitRegisterStatement(SSARegisterStatement& stmt) override;
				void VisitReturnStatement(SSAReturnStatement& stmt) override;
				void VisitSelectStatement(SSASelectStatement& stmt) override;
				void VisitSwitchStatement(SSASwitchStatement& stmt) override;
				void VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt) override;
				void VisitVariableKillStatement(SSAVariableKillStatement& stmt) override;
				void VisitVariableReadStatement(SSAVariableReadStatement& stmt) override;
				void VisitVariableWriteStatement(SSAVariableWriteStatement& stmt) override;
				void VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt) override;
				void VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt) override;
			};
		}
	}
}
