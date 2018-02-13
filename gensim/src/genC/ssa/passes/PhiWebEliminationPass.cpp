/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

class PhiWeb
{
public:
	PhiWeb(const std::list<SSAStatement*> &nodes) : nodes_(nodes)
	{
		for(auto i : nodes) {
			if(dynamic_cast<SSAPhiStatement*>(i)) {
				type_ = i->GetType();
			}
		}
	}
	
	SSAType GetType() {
		return type_;
	}
	
	std::list<SSAStatement*> GetNodes() {
		return nodes_;
	}
private:
	std::list<SSAStatement*> nodes_;
	SSAType type_;
};

class PhiWebEliminationPass : public SSAPass
{
public:		
	std::list<PhiWeb> GetPhiWebs(SSAFormAction &action) const {
		auto stmts = action.GetStatements([](SSAStatement *stmt){ return dynamic_cast<SSAPhiStatement*>(stmt) != nullptr; });
		std::set<SSAStatement *> nodes;
		std::set<std::pair<SSAStatement*, SSAStatement*>> edges;
		
		for(auto stmt : stmts) {
			nodes.insert(stmt);
			if(SSAPhiStatement *phi_stmt = dynamic_cast<SSAPhiStatement*>(stmt)) {
				for(auto i : phi_stmt->Get()) {
					edges.insert({phi_stmt, (SSAStatement*)i});
					edges.insert({(SSAStatement*)i, phi_stmt});
					nodes.insert((SSAStatement*)i);
				}
			}
		}
		
		gensim::util::Tarjan<SSAStatement*> tarjan (nodes.begin(), nodes.end(), edges.begin(), edges.end());
		auto sccs = tarjan.Compute();
		
		std::list<PhiWeb> phiwebs;
		
		for(auto &scc : sccs) {
			phiwebs.push_back(PhiWeb(std::list<SSAStatement*>(scc.begin(), scc.end())));
		}
		
		return phiwebs;
	}
	
	void ReplacePhiNode(SSAPhiStatement *phi_stmt, SSASymbol *symbol) const {		
		SSAVariableReadStatement *phi_read = new SSAVariableReadStatement(phi_stmt->Parent, symbol, phi_stmt);
		auto uses = phi_stmt->GetUses();
		for(auto i : uses) {
			if(SSAStatement *stmt = dynamic_cast<SSAStatement*>(i)) {
				stmt->Replace(phi_stmt, phi_read);
			}
		}
		
		phi_stmt->Parent->RemoveStatement(*phi_stmt);
		phi_stmt->Unlink();
		phi_stmt->Dispose();
		delete phi_stmt;
	}
	
	void ReplaceNonPhiNode(SSAStatement *stmt, SSASymbol *symbol) const {
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
		auto phiwebs = GetPhiWebs(action);
		
		// for each phi web
		for(auto &web : phiwebs) {
			//  create a symbol to represent the phi web
			SSASymbol *web_symbol = new SSASymbol(action.GetContext(), "phinode", web.GetType(), gensim::genc::Symbol_Local);
			action.AddSymbol(web_symbol);
			
			// For each entry in the phi web:
			// - if it's a phi node, replace it with a variable read
			// - otherwise, place a write to the phi variable immediately afterwards
			for(auto stmt : web.GetNodes()) {
				if(SSAPhiStatement *phi_stmt = dynamic_cast<SSAPhiStatement*>(stmt)) {
					ReplacePhiNode(phi_stmt, web_symbol);
				} else {
					ReplaceNonPhiNode(stmt, web_symbol);
				}
			}
		}
		
		return false;
	}
};


RegisterComponent0(SSAPass, PhiWebEliminationPass, "PhiWebElimination", "Perform phi elimination using phi webs");