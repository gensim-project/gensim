/*
 * genC/ssa/statement/SSAJumpStatement.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ssa/statement/SSAControlFlowStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			/**
			 * A statement representing a constant unconditional jump in genC control flow.
			 */
			class SSAJumpStatement : public SSAControlFlowStatement
			{
			public:
				virtual bool IsFixed() const;
				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();

				virtual target_list_t GetTargets() override
				{
					return {Target()};
				}

				virtual target_const_list_t GetTargets() const override
				{
					return {Target()};
				}

				virtual bool ReplaceTarget(SSABlock *f, SSABlock *r)
				{
					if (Target() == f) {
						if (Target()) Target()->RemovePredecessor(*Parent);
						SetTarget(r);
						if (Target()) Target()->AddPredecessor(*Parent);
						return true;
					}
					return false;
				}

				SSABlock *Target()
				{
					return (SSABlock*) GetOperand(0);
				}

				const SSABlock *Target() const
				{
					return (const SSABlock*) GetOperand(0);
				}
				void SetTarget(SSABlock *newtarget);

				void Accept(SSAStatementVisitor& visitor) override;

				SSAJumpStatement(SSABlock *parent, SSABlock &target, SSAStatement *before = NULL) : SSAControlFlowStatement(1, parent, before)
				{
					SetOperand(0, &target);
				}
			};
		}
	}
}
