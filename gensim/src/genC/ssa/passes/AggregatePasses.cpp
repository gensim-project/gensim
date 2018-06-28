/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/passes/SSAPass.h"

#include "genC/ssa/statement/SSAControlFlowStatement.h"

#include <list>

using namespace gensim::genc::ssa;

class PhiOptimisationPass : public SSAPass
{
public:
	bool Run(SSAFormAction &action) const override
	{
		SSAPassManager manager;
		manager.SetMultirunEach(true);
		manager.SetMultirunAll(false);
		manager.AddPass(SSAPassDB::Get("PhiAnalysis"));
//		manager.AddPass(SSAPassDB::Get("PhiPropagation"));

		return manager.Run(action);
	}
};

class CheckEmptyBlocksPass : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		for(auto i : action.Blocks) {
			GASSERT(!i->GetStatements().empty());
		}
		return false;
	}

};

class O1Pass : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		SSAPassManager manager;

		manager.AddPass(SSAPassDB::Get("DeadCodeElimination"));
		manager.AddPass(SSAPassDB::Get("UnreachableBlockElimination"));
		manager.AddPass(SSAPassDB::Get("ControlFlowSimplification"));
		manager.AddPass(SSAPassDB::Get("BlockMerging"));
		manager.AddPass(SSAPassDB::Get("Inlining"));
		manager.AddPass(SSAPassDB::Get("DeadSymbolElimination"));

		return manager.Run(action);
	}
};

class O3Pass : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		SSAPassManager manager;

		manager.AddPass(SSAPassDB::Get("DeadCodeElimination"));
		manager.AddPass(SSAPassDB::Get("UnreachableBlockElimination"));
		manager.AddPass(SSAPassDB::Get("ControlFlowSimplification"));
		manager.AddPass(SSAPassDB::Get("BlockMerging"));
		manager.AddPass(SSAPassDB::Get("ConstantFolding"));
		manager.AddPass(SSAPassDB::Get("ConstantPropagation"));
		manager.AddPass(SSAPassDB::Get("ValuePropagation"));
		manager.AddPass(SSAPassDB::Get("LoadStoreElimination"));
		manager.AddPass(SSAPassDB::Get("DeadWriteElimination"));
		manager.AddPass(SSAPassDB::Get("Inlining"));
		manager.AddPass(SSAPassDB::Get("DeadSymbolElimination"));
		manager.AddPass(SSAPassDB::Get("JumpThreading"));

		return manager.Run(action);
	}
};

class O4Pass : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		// O3, then phi analyse, then O3, then phi eliminate, then O3 again
		SSAPassManager manager;
		manager.SetMultirunAll(false);
		manager.SetMultirunEach(true);

		manager.AddPass(SSAPassDB::Get("O3"));
		manager.AddPass(SSAPassDB::Get("PhiAnalysis"));
		manager.AddPass(SSAPassDB::Get("DeadPhiElimination"));
		manager.AddPass(SSAPassDB::Get("O3"));
		manager.AddPass(SSAPassDB::Get("PhiSetElimination"));
		manager.AddPass(SSAPassDB::Get("O3"));

		return manager.Run(action);
	}

};

RegisterComponent0(SSAPass, O1Pass, "O1", "Use a few optimisations")
RegisterComponent0(SSAPass, O3Pass, "O3", "Use a lot of optimisations")
RegisterComponent0(SSAPass, O4Pass, "O4", "Use even more optimisations")
