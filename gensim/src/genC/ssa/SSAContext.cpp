/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/passes/SSAPass.h"

#include "genC/ssa/testing/SSAInterpreter.h"
#include "genC/ssa/validation/SSAActionValidator.h"

#include "genC/ssa/printers/SSAActionPrinter.h"
#include "genC/ir/IRAttributes.h"

#include "util/WorkQueue.h"
#include "util/ParallelWorker.h"

#include <stdio.h>

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSAContext::SSAContext(const gensim::isa::ISADescription& isa, const gensim::arch::ArchDescription& arch) : SSAContext(isa, arch, std::shared_ptr<ssa::SSATypeManager>(new ssa::SSATypeManager()))
{

}


/**
 * Constructs a new SSAContext, and associates it with the given architecture description.
 * @param arch The architecture description to associate the SSA context with.
 */
SSAContext::SSAContext(const gensim::isa::ISADescription& isa, const gensim::arch::ArchDescription& arch, std::shared_ptr<SSATypeManager> type_manager) : arch_(arch), isa_(isa), parallel_optimise_(false), test_optimise_(false), type_manager_(type_manager)
{
#ifdef MULTITHREAD
	SetParallelOptimise(true);
#endif
}

/**
 * Finalises an SSAContext object.
 */
SSAContext::~SSAContext()
{
	for(auto i : actions_) {
		i.second->Unlink();
	}
	for(auto i : actions_) {
		i.second->Destroy();
	}
	for(auto i : actions_) {
		delete i.second;
	}
}

static bool OptimiseAction(SSAFormAction *action, SSAPass *pass)
{
	// run the pass until it reaches a fixed point
	bool anychange = false;
	bool changed = false;
	do {
		changed = pass->Run(*action);
		anychange |= changed;
	} while(changed);
	return anychange;
}

typedef  gensim::genc::ssa::testing::MachineState<gensim::genc::ssa::testing::BasicRegisterFileState, gensim::genc::ssa::testing::MemoryState> machine_state_t;

static machine_state_t InterpretAction(gensim::genc::ssa::SSAFormAction *action)
{
	machine_state_t machine_state;
	machine_state.RegisterFile().SetSize(action->Arch->GetRegFile().GetSize());
	machine_state.RegisterFile().SetWrap(true);

	gensim::genc::ssa::testing::SSAInterpreter interp (action->Arch, machine_state);
	interp.SetTracing(false);

	std::vector<IRConstant> values;

	for(unsigned int i = 0; i < action->ParamSymbols.size(); ++i) {
		values.push_back(IRConstant::Integer(0));
	}

	auto result = interp.ExecuteAction(action, values);

	switch(result.Result) {
			using namespace gensim::genc::ssa::testing;
		case Interpret_Error:
			throw std::logic_error("Error in interpretation");
			break;
	}

	return machine_state;
}

static bool OptimiseAndTestAction(SSAFormAction *action, SSAPass *pass)
{
	auto pre_state = InterpretAction(action);

	bool changed = OptimiseAction(action, pass);

	auto post_state = InterpretAction(action);

	if(!pre_state.Equals(post_state)) {
		fprintf(stderr, "ERROR DETECTED!\n");
		abort();
	}

	return changed;
}

void SSAContext::Optimise()
{
	for(auto action : Actions()) {
		if(dynamic_cast<SSAFormAction*>(action.second)) {
			Optimise((SSAFormAction*)action.second);
		}
	}
}


/**
 * Optimises the given SSAFormAction.
 * @param action The SSAFormAction to optimise.
 */
void SSAContext::Optimise(SSAFormAction *action)
{
	SSAPassDB::Get("O4")->Run(*action);
}

/**
 * Resolves the SSAContext.
 * @param ctx The diagnostic context to use for message reporting.
 * @return Returns TRUE if the SSAContext was successfully resolved, FALSE otherwise.
 */
bool SSAContext::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	for (const auto& action_item : actions_) {
		if (!action_item.second->HasAttribute(ActionAttribute::Helper)) continue;

		SSAFormAction *action = dynamic_cast<SSAFormAction *>(action_item.second);

		success &= action->Resolve(ctx);
		action->DoFixednessAnalysis();
	}
	if(!success) {
		return false;
	}

	for (const auto& action_item : actions_) {
		if (action_item.second->HasAttribute(ActionAttribute::Helper)) continue;

		SSAFormAction *action = dynamic_cast<SSAFormAction *>(action_item.second);
		success &= action->Resolve(ctx);
		action->DoFixednessAnalysis();
	}

	if(!success) {
		return false;
	}

	return success;
}

bool SSAContext::Validate(DiagnosticContext& ctx)
{
	using namespace validation;
	SSAActionValidator man;

	bool success = true;
	for(auto action : Actions()) {
		if(dynamic_cast<SSAFormAction*>(action.second)) {
			success &= man.Run((SSAFormAction*)action.second, ctx);
		}
	}

	return success;
}
