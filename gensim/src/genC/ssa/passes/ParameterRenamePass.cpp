/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <typeinfo>

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/statement/SSAReturnStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"


using namespace gensim::genc::ssa;

class ParameterRenamingPass : public SSAPass
{
public:
	SSABlock *GetPrologue(SSAFormAction &action) const
	{
		// for now, just create a new block and insert it at the start of
		// the action. In future, only do this if the entry block isn't already
		// the shape of a 'prologue' (only parameter reads and variable writes)

		SSABlock *prologue = new SSABlock(action.GetContext(), action);

		new SSAJumpStatement(prologue, *action.EntryBlock);
		action.EntryBlock = prologue;

		return prologue;
	}

	SSABlock *GetEpilogue(SSAFormAction &action) const
	{
		// for now, just create a new block and point all returns at it.
		// In future, only do this if there isn't already an epilogue block.

		SSABlock *epilogue = new SSABlock(action.GetContext(), action);

		SSASymbol *return_symbol = nullptr;
		if(action.GetPrototype().GetIRSignature().HasReturnValue()) {
			return_symbol = new SSASymbol(action.GetContext(), "return_symbol", action.GetPrototype().ReturnType(), gensim::genc::Symbol_Local);
			action.AddSymbol(return_symbol);

			auto vread = new SSAVariableReadStatement(epilogue, return_symbol);
			new SSAReturnStatement(epilogue, vread);
		} else {
			new SSAReturnStatement(epilogue, nullptr);
		}

		for(auto block : action.Blocks) {
			if(block == epilogue) {
				continue;
			}
			// replace each return statement with a variable write and a
			// jump to the epilogue
			if(SSAReturnStatement *rtn = dynamic_cast<SSAReturnStatement*>(block->GetControlFlow())) {
				if(return_symbol != nullptr) {
					new SSAVariableWriteStatement(block, return_symbol, rtn->Value(), rtn);
				}
				new SSAJumpStatement(block, *epilogue, rtn);

				block->RemoveStatement(*rtn);
				rtn->Unlink();
				rtn->Dispose();
				delete rtn;
			}
		}

		return epilogue;
	}

	std::list<SSASymbol*> GetParameterSymbols(SSAFormAction &action) const
	{
		std::list<SSASymbol*> symbols;
		for(auto sym : action.Symbols()) {
			if(sym->GetType().IsStruct()) {
				// don't do anything with the instruction struct (for now))
				continue;
			}
			if(sym->SType == gensim::genc::Symbol_Parameter) {
				symbols.push_back(sym);
			} else if(sym->IsReference()) {
				symbols.push_back(sym);
			}
		}
		return symbols;
	}

	void RenameParameter(SSAFormAction &action, SSASymbol *parameter, SSABlock *prologue, SSABlock *epilogue) const
	{
		// create a new symbol for the replaced parameter
		SSAType sym_type = parameter->GetType();
		sym_type.Reference = false;
		SSASymbol *renamed_param = new SSASymbol(action.GetContext(), "replaced_" + parameter->GetPrettyName(), sym_type, gensim::genc::Symbol_Local);
		action.AddSymbol(renamed_param);

		// replace all uses of the parameter with the renamed value
		auto uses = ((SSAValue*)parameter)->GetUses();
		for(SSAValue *use : uses) {
			if(SSAStatement *stmt = dynamic_cast<SSAStatement*>(use)) {
				stmt->Replace(parameter, renamed_param);
			}
		}

		// read/write the parameter in the prologue
		auto incoming_value = new SSAVariableReadStatement(prologue, parameter, prologue->GetControlFlow());
		new SSAVariableWriteStatement(prologue, renamed_param, incoming_value, prologue->GetControlFlow());

		// if the parameter is a reference, write it back in the epilogue
		if(parameter->GetType().Reference) {
			auto vread = new SSAVariableReadStatement(epilogue, renamed_param, epilogue->GetControlFlow());
			new SSAVariableWriteStatement(epilogue, parameter, vread, epilogue->GetControlFlow());
		}
	}

	bool Run(SSAFormAction& action) const override
	{
		// get a list of symbols to replace (parameters which are accessed
		// outside of the prologue and epilogue)
		auto params = GetParameterSymbols(action);

		if(params.empty()) {
			return false;
		}

		// look for (or create) prologue and epilogue blocks
		SSABlock *prologue_block = GetPrologue(action);
		SSABlock *epilogue_block = GetEpilogue(action);


		for(auto param : params) {
			RenameParameter(action, param, prologue_block, epilogue_block);
		}

		return false;
	}
};

RegisterComponent0(SSAPass, ParameterRenamingPass, "ParameterRename", "Perform parameter renaming to prepare for phi analysis");
