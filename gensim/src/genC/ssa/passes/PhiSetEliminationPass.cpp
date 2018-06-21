/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSAPhiStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"
#include "genC/ssa/statement/SSAControlFlowStatement.h"
#include "genC/ssa/analysis/VariableUseDefAnalysis.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "genC/ssa/analysis/SSADominance.h"

#include "util/Tarjan.h"

using namespace gensim::genc::ssa;

#define DEBUG_PASS
#ifdef DEBUG_PASS
#define DEBUG(...) do { fprintf(stderr, "DEBUG: " __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

template <typename T> std::string FormatSet(const std::set<T> &set);

template <> std::string FormatSet<SSAStatement*>(const std::set<SSAStatement*> &set)
{
	std::stringstream str;
	str << "{";

	bool first = true;
	for(auto i : set) {
		if(!first) {
			str << ", " << i->GetName();
		} else {
			str << i->GetName();
			first = false;
		}
	}

	str << "}";
	return str.str();
}

class PhiSetInfo
{
public:
	typedef std::set<SSAStatement*> phi_set_t;
	std::map<phi_set_t, std::list<SSAPhiStatement*>> PhiSets;
};

class PhiSetEliminationPass : public SSAPass
{
public:
	PhiSetInfo GetPhiSets(SSAFormAction &action) const
	{
		PhiSetInfo psi;

		auto stmts = action.GetStatements([](SSAStatement *stmt) {
			return dynamic_cast<SSAPhiStatement*>(stmt) != nullptr;
		});

		for(SSAStatement *stmt : stmts) {
			SSAPhiStatement *phi_stmt = (SSAPhiStatement*)stmt;
			std::set<SSAStatement*> phi_set;
			for(auto i : phi_stmt->Get()) {
				phi_set.insert((SSAStatement*)i);
			}
			psi.PhiSets[phi_set].push_back(phi_stmt);
		}

		return psi;
	}

	void ReplacePhiNode(SSAPhiStatement *phi_stmt, SSASymbol *symbol) const
	{
		SSAVariableReadStatement *phi_read = new SSAVariableReadStatement(phi_stmt->Parent, symbol, phi_stmt);
		auto uses = phi_stmt->GetUses();
		for(auto i : uses) {
			if(SSAStatement *stmt = dynamic_cast<SSAStatement*>(i)) {
				stmt->Replace(phi_stmt, phi_read);
			}
		}
	}

	void ReplaceNonPhiNode(SSAStatement *stmt, SSASymbol *symbol) const
	{
		// after the statement, insert a write to the phi symbol
		auto stmt_it = std::find(stmt->Parent->GetStatements().begin(), stmt->Parent->GetStatements().end(), stmt);
		stmt_it++;

		new SSAVariableWriteStatement(stmt->Parent, symbol, stmt, *stmt_it);
	}

	bool Run(SSAFormAction& action) const override
	{
		SSAPass *phi_cleanup_pass = GetComponent<SSAPass>("PhiCleanup");
		phi_cleanup_pass->Run(action);
		delete phi_cleanup_pass;

		// create phi webs
		auto phisets = GetPhiSets(action);

		std::map<PhiSetInfo::phi_set_t, SSASymbol*> phi_symbols;
		for(auto &phiset : phisets.PhiSets) {
			SSAType symbol_type = phiset.second.front()->GetType();
			SSASymbol *phi_symbol = new SSASymbol(action.GetContext(), "phiset", symbol_type, gensim::genc::Symbol_Local);
			action.AddSymbol(phi_symbol);
			phi_symbols[phiset.first] = phi_symbol;
		}

		for(auto &phiset : phisets.PhiSets) {
			auto phi_symbol = phi_symbols[phiset.first];
			for(auto i : phiset.first) {
				ReplaceNonPhiNode((SSAStatement*)i, phi_symbol);
			}
		}

		for(auto &phiset : phisets.PhiSets) {
			auto phi_symbol = phi_symbols[phiset.first];
			for(auto i : phiset.second) {
				ReplacePhiNode((SSAPhiStatement*)i, phi_symbol);
			}
		}


		return false;
	}
};


RegisterComponent0(SSAPass, PhiSetEliminationPass, "PhiSetElimination", "Perform phi elimination using phi sets");
