#pragma once

#include <map>
#include <set>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAStatement;
			class SSAFormAction;
			class SSABlock;

			class RegisterAllocationAnalysis;
			class RegisterAllocation
			{
				typedef int RegisterID;
				friend class RegisterAllocationAnalysis;

			public:
				bool IsStatementAllocated(const SSAStatement *statement) const
				{
					return _allocation.count(statement) != 0;
				}

				RegisterID GetRegisterForStatement(const SSAStatement *statement) const
				{
					return _allocation.at(statement);
				}

			private:
				std::map<const SSAStatement *, RegisterID> _allocation;

				void AllocateStatement(const SSAStatement *statement, RegisterID id)
				{
					_allocation[statement] = id;
				}
			};

			class RegisterAllocationAnalysis
			{
			public:
				const RegisterAllocation Allocate(const SSAFormAction *action, int max_regs);

			private:
				const std::set<SSABlock *> GetExitBlocks(const SSAFormAction& action) const;
				void ProcessBlock(RegisterAllocation& allocation, SSABlock *block) const;
			};
		}
	}
}