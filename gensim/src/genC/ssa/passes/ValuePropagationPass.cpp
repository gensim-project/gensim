/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <typeinfo>

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"

using namespace gensim::genc::ssa;

#define DPRINTF while(0) printf

class ValuePropagationPass : public SSAPass
{
public:
	virtual ~ValuePropagationPass()
	{

	}

	bool Run(SSAFormAction& action) const  override
	{
		bool changed = false;
		for(auto &block : action.GetBlocks()) {
			changed |= DoValuePropagation(block);
		}
		return changed;
	}

private:
	bool DoValuePropagation(SSABlock *block) const
	{
		bool changed = false;
		bool anychanged = false;

		std::map<SSASymbol*, SSAStatement*> live_values;


		DPRINTF("Value Prop %s: %s\n", block->Parent->GetPrototype().GetIRSignature().GetName().c_str(), block->GetName().c_str());
		do {
			live_values.clear();
			changed = false;
			for(auto stmt : block->GetStatements()) {
				// If this statement is a write to any variables, remove those variables
				// from the live value set
				if(auto kill = dynamic_cast<SSAVariableKillStatement*>(stmt)) {
					live_values.erase(kill->Target());
					DPRINTF("Killed %s\n", kill->Target()->GetPrettyName().c_str());
				}

				// also remove any variables which are made dynamic from the lv set
//				for(auto &killed : stmt->GetKilledVariables()) {
//					live_values.erase(killed);
//					DPRINTF("Killed %s\n", killed->GetPrettyName().c_str());
//				}

				// If this statement is precisely a variable write (and not any subclass)
				// Then store its expression as the live value for its target
				if(typeid(*stmt) == typeid(SSAVariableWriteStatement)) {
					auto write = (SSAVariableWriteStatement*) stmt;
					live_values[write->Target()] = write->Expr();
					DPRINTF("%s live with %s\n", write->Target()->GetPrettyName().c_str(), write->Expr()->GetName().c_str());
				}

				// Finally, if this statement reads any variables, if those variables have
				// live values, then replace uses of this variable read with uses of the live
				// statement for that variable
				if(auto read = dynamic_cast<SSAVariableReadStatement*>(stmt)) {
					if(live_values.count(read->Target())) {
						SSAStatement *live_value = live_values.at(read->Target());
						assert(live_value != read);

						DPRINTF("Replacing %s (read of %s) with %s\n", read->GetName().c_str(), read->Target()->GetPrettyName().c_str(), live_value->GetName().c_str());

						block->RemoveStatement(*read);
						while(read->GetUses().size()) {
							auto _use = read->GetUses().front();

							SSAStatement *user = dynamic_cast<SSAStatement*>(_use);
							if(user) {
								bool found = user->Replace(read, live_value);
								if(!found) {
									throw std::logic_error("");
								}
							} else {
								throw std::logic_error("Unknown use type");
							}
						}

						read->Dispose();
						delete read;
						changed = true;
						break;
					}
				}
			}
			anychanged |= changed;
		} while(changed);

		return anychanged;
	}
};

RegisterComponent0(SSAPass, ValuePropagationPass, "ValuePropagation", "Propagate values in registers instead of via loads/stores");
