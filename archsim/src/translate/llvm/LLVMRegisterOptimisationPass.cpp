/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include "translate/llvm/LLVMRegisterOptimisationPass.h"
#include <llvm/Analysis/RegionInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/PassSupport.h>

#include <sstream>

using namespace archsim::translate::translate_llvm;

char LLVMRegisterOptimisationPass::ID = 0;

//#define DEBUGLOG(...) fprintf(stderr, __VA_ARGS__)
//#define DEBUGLOG2(...) fprintf(stderr, __VA_ARGS__)

#ifndef DEBUGLOG
#define DEBUGLOG(...)
#endif
#ifndef DEBUGLOG2
#define DEBUGLOG2(...)
#endif

//static llvm::RegisterPass<LLVMRegisterOptimisationPass> X("register-opt", "Optimise register accesses", false, false);

static std::string FormatLLVMInst(llvm::Value *inst)
{
	std::string st;
	llvm::raw_string_ostream str(st);
	str << *inst;
	return str.str();
}

static void *initializeLLVMRegisterOptimisationPass(llvm::PassRegistry &Registry)
{
	llvm::PassInfo *pi = new llvm::PassInfo("Optimise Register Accesses", "register-opt", &LLVMRegisterOptimisationPass::ID, llvm::PassInfo::NormalCtor_t(llvm::callDefaultCtor<LLVMRegisterOptimisationPass>), false, false);
	Registry.registerPass(*pi, true);
	return pi;
}

static void initPass(llvm::PassRegistry &r)
{
	llvm::initializeRegionInfoPassPass(r);
	initializeLLVMRegisterOptimisationPass(r);
}

static llvm::once_flag init_flag;
class OptRegisterer
{
public:
	OptRegisterer()
	{
		llvm::call_once(init_flag, initPass, *llvm::PassRegistry::getPassRegistry());
	}
};

OptRegisterer opt;


LLVMRegisterOptimisationPass::LLVMRegisterOptimisationPass() : llvm::FunctionPass(ID)
{

}

static bool isRegisterStore(llvm::Instruction *inst)
{
	if(llvm::isa<llvm::StoreInst>(inst)) {
		llvm::StoreInst *store = (llvm::StoreInst*)inst;

		PointerInformationProvider pip(inst->getFunction());
		std::vector<int> info;
		pip.GetPointerInfo(store->getPointerOperand(), info);

		if(info.size()) {
			return info.at(0) == 0;
		}
	}
	return false;
}

static bool isRegisterLoad(llvm::Instruction *inst)
{
	if(llvm::isa<llvm::LoadInst>(inst)) {
		llvm::LoadInst *load = (llvm::LoadInst*)inst;

		PointerInformationProvider pip(inst->getFunction());
		std::vector<int> info;
		pip.GetPointerInfo(load->getPointerOperand(), info);

		if(info.size()) {
			return info.at(0) == 0;
		}
	}
	return false;
}

static bool isBreaker(llvm::Instruction *inst)
{
	if(llvm::isa<llvm::CallInst>(inst)) {
		llvm::CallInst* call = (llvm::CallInst*)inst;

		if(call->getCalledFunction() == nullptr) {
			return true;
		}

		std::string fnname = call->getCalledFunction()->getName().str();
		if(fnname.find("Trace") != std::string::npos) {
			return false;
		}
		if(fnname.find("llvm.assume") == 0) {
			return false;
		}

		if(!call->getCalledFunction()->doesNotAccessMemory()) {
			return true;
		}
	}
	return false;
}

class RegisterDefinitions
{
public:
	using Interval = std::pair<int, int>;
	using DefinitionList = std::unordered_set<llvm::StoreInst*>;

	static RegisterDefinitions MergeDefinitions(const std::vector<RegisterDefinitions> &defs)
	{
		RegisterDefinitions output_defs;

		for(auto &i : defs) {
			for(int byte = 0; byte < i.interval_definition_.size(); ++byte) {
				output_defs.AddByteDefinition(byte, i.interval_definition_.at(byte));
			}
		}

		return output_defs;
	}
	static RegisterDefinitions MergeDefinitions(const std::vector<RegisterDefinitions*> &defs)
	{
		RegisterDefinitions output_defs;

		for(auto &i : defs) {
			for(int byte = 0; byte < i->interval_definition_.size(); ++byte) {
				output_defs.AddByteDefinition(byte, i->interval_definition_.at(byte));
			}
		}

		return output_defs;
	}

	void SetDefinition(const Interval &interval, llvm::StoreInst *inst)
	{
		for(int i = interval.first; i <= interval.second; ++i) {
			SetByteDefinition(i, inst);
		}
	}

	void AddDefinition(const Interval &interval, llvm::StoreInst *inst)
	{
		for(int i = interval.first; i <= interval.second; ++i) {
			AddByteDefinition(i, inst);
		}
	}

	std::set<llvm::StoreInst*> GetDefinitions() const
	{
		std::set<llvm::StoreInst*> defs;
		for(auto i : interval_definition_) {
			for(auto j : i) {
				defs.insert(j);
			}
		}
		return defs;
	}

	std::set<llvm::StoreInst*> GetDefinitionsForByte(int b) const
	{
		if(interval_definition_.size() <= b) {
			return {};
		}

		std::set<llvm::StoreInst*> stores;
		for(auto i : interval_definition_.at(b)) {
			stores.insert(i);
		}
		return stores;
	}

	bool operator==(const RegisterDefinitions &other) const
	{
		return interval_definition_ == other.interval_definition_;
	}
	bool operator!=(const RegisterDefinitions &other) const
	{
		return !operator==(other);
	}


private:
	friend std::ostream &operator<<(std::ostream &str, const RegisterDefinitions &rd);

	// TODO: be a bit smarter with this
	std::vector<DefinitionList> interval_definition_;

	void AddByteDefinition(int byte, const DefinitionList &inst)
	{
		if(interval_definition_.size() <= byte+1) {
			interval_definition_.resize(byte+1);
		}
		interval_definition_.at(byte).insert(inst.begin(), inst.end());
	}

	void AddByteDefinition(int byte, llvm::StoreInst *inst)
	{
		if(interval_definition_.size() <= byte+1) {
			interval_definition_.resize(byte+1);
		}
		interval_definition_.at(byte).insert(inst);
	}

	void SetByteDefinition(int byte, llvm::StoreInst *inst)
	{
		if(interval_definition_.size() < byte+1) {
			interval_definition_.resize(byte+1);
		}
		interval_definition_.at(byte) = { inst };
	}
};

std::ostream &operator<<(std::ostream &str, const RegisterDefinitions &rd)
{
	for(int i = 0; i < rd.interval_definition_.size(); ++i) {
		str << i << " = (";

		for(auto j : rd.interval_definition_.at(i)) {
			str << j << " ";
		}

		str << ")";
	}

	return str;
}

bool IsDefinition(llvm::Instruction *inst)
{
	return isRegisterStore(inst);
}

bool AddDefinition(llvm::Instruction *store, RegisterDefinitions &definitions, LLVMRegisterOptimisationPass::DefinitionSet &all_definitions)
{
	PointerInformationProvider pip(store->getFunction());

	if(llvm::isa<llvm::StoreInst>(store)) {
		llvm::StoreInst *st = (llvm::StoreInst*)store;
		std::vector<int> info;
		bool success = pip.GetPointerInfo(st->getPointerOperand(), st->getPointerOperandType()->getPointerElementType()->getPrimitiveSizeInBits()/8, info);
		if(!success) {
			fprintf(stderr, "Failed to get pointer info for %s\n", FormatLLVMInst(st->getPointerOperand()).c_str());
			UNEXPECTED;
		}

		// If we have precise information about this store, then override any
		// definitions available for the detected interval. Otherwise, override
		// the entire register file.
		if(info.size() > 1) {
			definitions.SetDefinition({info.at(1), info.at(1) + info.at(2) - 1}, st);
		} else {
			UNIMPLEMENTED;
		}

		DEBUGLOG(" ** Definition added for %u - %u: %s\n", info.at(1), info.at(1) + info.at(2)-1, FormatLLVMInst(st).str().c_str());

		all_definitions.insert(st);

	} else {
		UNEXPECTED;
	}

	return true;
}

bool IsUse(llvm::Instruction *inst)
{
	return isRegisterLoad(inst) || isBreaker(inst) || llvm::isa<llvm::ReturnInst>(inst);
}

bool AddUse(llvm::Instruction *inst, const RegisterDefinitions &live_defs, LLVMRegisterOptimisationPass::DefinitionSet &live_definition_instructions)
{
	// two cases: if inst is a register load, then add live definitions for any store instructions which touch the loaded range.
	//            if inst is a call, then add live definitions for all live stores

	if(isRegisterLoad(inst)) {
		llvm::LoadInst *load = (llvm::LoadInst*)inst;

		PointerInformationProvider pip(inst->getFunction());
		std::vector<int> info;
		bool success = pip.GetPointerInfo(load->getPointerOperand(), load->getPointerOperandType()->getPointerElementType()->getPrimitiveSizeInBits()/8, info);

		DEBUGLOG(" *** %s is a load\n", FormatLLVMInst(inst).c_str());

		if(!success || info.at(0) != 0 || info.size() < 3) {
			UNEXPECTED;
		}

		for(int i = info.at(1); i < info.at(1) + info.at(2); ++i) {
			auto defs = live_defs.GetDefinitionsForByte(i);
			for(auto d : defs) {
				live_definition_instructions.insert(d);

				DEBUGLOG(" *** Use added for %u: %s\n", i, FormatLLVMInst(d).c_str());
			}
		}

	} else if(isBreaker(inst) || llvm::isa<llvm::ReturnInst>(inst)) {
		// all current reachable definitions become live
		DEBUGLOG(" *** %s is a breaker\n", FormatLLVMInst(inst).c_str());
		for(auto def : live_defs.GetDefinitions()) {
			live_definition_instructions.insert(def);
			DEBUGLOG(" *** Use added for %s\n", FormatLLVMInst(def).c_str());
		}
	} else {
		// some unhandled case
		UNEXPECTED;
	}

	return true;
}

bool ProcessInstructionRange(llvm::BasicBlock::iterator start, llvm::BasicBlock::iterator end, const RegisterDefinitions &incoming_definitions, RegisterDefinitions &outgoing_definitions, LLVMRegisterOptimisationPass::DefinitionSet &all_definitions, LLVMRegisterOptimisationPass::DefinitionSet &live_definitions)
{
	DEBUGLOG2(" *** %u incoming defs\n", incoming_definitions.GetDefinitions().size());
	DEBUGLOG2("%s\n", incoming_definitions.print().c_str());

	auto oldout = outgoing_definitions;
	DEBUGLOG2(" *** %u initial outgoing defs\n", oldout.GetDefinitions().size());
	DEBUGLOG2(" %s\n", oldout.print().c_str());

	outgoing_definitions = incoming_definitions;

	for(auto it = start; it != end; ++it) {
		llvm::Instruction *inst = &*it;

		if(IsDefinition(inst)) {
			// add store as a definition
			AddDefinition(inst, outgoing_definitions, all_definitions);
		}

		if(IsUse(inst)) {
			// process use into reachable definitions
			AddUse(inst, outgoing_definitions, live_definitions);
		}
	}

	DEBUGLOG2(" *** %u outgoing defs\n", outgoing_definitions.GetDefinitions().size());
	DEBUGLOG2(" %s\n", outgoing_definitions.print().c_str());

	if(oldout != outgoing_definitions) {
		DEBUGLOG(" *** Difference detected!\n");
	}
	return oldout != outgoing_definitions;
}

bool ProcessBlock(llvm::BasicBlock *block, const RegisterDefinitions &incoming_definitions, RegisterDefinitions &outgoing_definitions, LLVMRegisterOptimisationPass::DefinitionSet &all_definitions, LLVMRegisterOptimisationPass::DefinitionSet &live_definitions)
{
	DEBUGLOG(" *** Processing block %s\n", block->getName().str().c_str());
	return ProcessInstructionRange(block->begin(), block->end(), incoming_definitions, outgoing_definitions, all_definitions, live_definitions);
}

std::set<llvm::BasicBlock*> GetPredecessors(llvm::BasicBlock *block)
{
	std::set<llvm::BasicBlock*> preds;
	for(auto &b : *(block->getParent())) {
		for(int i = 0; i < b.getTerminator()->getNumSuccessors(); ++i) {
			auto p = b.getTerminator()->getSuccessor(i);
			if(p == block) {
				preds.insert(&b);
			}
		}
	}

	return preds;
}

bool LLVMRegisterOptimisationPass::ProcessFunction(llvm::Function &f, LLVMRegisterOptimisationPass::DefinitionSet &all_definitions, LLVMRegisterOptimisationPass::DefinitionSet &live_definitions)
{
	std::set<llvm::BasicBlock *> work_set;
	std::map<llvm::BasicBlock *, RegisterDefinitions> outgoing_defs;
	std::vector<RegisterDefinitions*> incoming_defs;

	for(auto &b : f) {
		work_set.insert(&b);
		outgoing_defs[&b];
	}

	while(!work_set.empty()) {
		auto *block = *work_set.begin();
		work_set.erase(block);

		incoming_defs.clear();
		for(auto pred : GetPredecessors(block)) {
			incoming_defs.push_back(&outgoing_defs.at(pred));
		}
		RegisterDefinitions incoming = RegisterDefinitions::MergeDefinitions(incoming_defs);

		if(ProcessBlock(block, incoming, outgoing_defs.at(block), all_definitions, live_definitions)) {
			auto *terminator = block->getTerminator();
			for(int i = 0; i < terminator->getNumSuccessors(); ++i) {
				work_set.insert(terminator->getSuccessor(i));
			}
		}
	}

	return true;
}


bool LLVMRegisterOptimisationPass::runOnFunction(llvm::Function &f)
{
	DEBUGLOG(" *** Starting to work on %s\n", f.getName().str().c_str());

	bool changed = false;

	DefinitionSet live_definitions, all_definitions, dead_definitions;
	ProcessFunction(f, all_definitions, live_definitions);

	for(auto i : all_definitions) {
		if(!live_definitions.count(i)) {
			dead_definitions.insert(i);
		}
	}

	DEBUGLOG(" *** Found %u/%u/%u all/live/dead defs\n", all_definitions.size(), live_definitions.size(), dead_definitions.size());
//	fprintf(stderr, " *** Found %u/%u/%u all/live/dead defs\n", all_definitions.size(), live_definitions.size(), dead_definitions.size());

	for(llvm::Instruction *dead_store : dead_definitions) {
		dead_store->removeFromParent();
		dead_store->deleteValue();
		changed = true;
	}

	return changed;
}

void archsim::translate::translate_llvm::LLVMRegisterOptimisationPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.setPreservesCFG();
}
