/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMRegisterOptimisationPass.h"
#include <llvm/Analysis/RegionInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/PassSupport.h>

using namespace archsim::translate::translate_llvm;

char LLVMRegisterOptimisationPass::ID = 0;

//static llvm::RegisterPass<LLVMRegisterOptimisationPass> X("register-opt", "Optimise register accesses", false, false);

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


LLVMRegisterOptimisationPass::LLVMRegisterOptimisationPass() : llvm::FunctionPass(ID)
{
	llvm::call_once(init_flag, initPass, *llvm::PassRegistry::getPassRegistry());
}

const char *indent(int count)
{
	static std::map<int, std::string> indentations;
	if(!indentations.count(count)) {
		indentations[count] = std::string(count, '\t');
	}

	return indentations.at(count).c_str();
}

void printregion(const llvm::Region *rgn, int depth = 0)
{
	fprintf(stderr, "%sRegion %p\n", indent(depth), rgn);
	for(const auto &i : rgn->blocks()) {
		fprintf(stderr, "%s - block %s\n", indent(depth), i->getName().str().c_str());
	}

	fprintf(stderr, "%s - subregions:\n", indent(depth));
	for(const auto &i : *rgn) {
		printregion(i.get(), depth+1);
	}
}

static bool isCandidateRegion(const llvm::Region *region)
{
	// a region is a candidate if it contains no loops.
	std::set<llvm::BasicBlock*> reached_blocks;

	std::list<llvm::BasicBlock*> work_list;
	work_list.push_back(region->getEntry());

	while(!work_list.empty()) {
		auto block = work_list.back();
		work_list.pop_back();

		if(!region->contains(block)) {
			continue;
		}


		// If we've already seen this block, we must have looped
		if(reached_blocks.count(block)) {
			return false;
		}

		reached_blocks.insert(block);

		for(auto i : block->getTerminator()->successors()) {
			work_list.push_back(i);
		}
	}

	return true;
}

static void getCandidateRegions(const llvm::Region *tlregion, std::vector<const llvm::Region *> &candidate_regions)
{
	if(isCandidateRegion(tlregion)) {
		candidate_regions.push_back(tlregion);
	}

	for(auto &i : *tlregion) {
		getCandidateRegions(i.get(), candidate_regions);
	}
}

static std::vector<llvm::StoreInst*> getRegisterStores(llvm::Value *register_base, const llvm::Region *rgn)
{
	std::vector<llvm::StoreInst*> register_stores;

	for(auto block : rgn->blocks()) {
		for(auto &insn : *block) {
			if(llvm::isa<llvm::StoreInst>(&insn)) {
				llvm::StoreInst *store = (llvm::StoreInst*)&insn;
				auto ptr = store->getPointerOperand();
				if(ptr->stripInBoundsConstantOffsets() == register_base) {
					register_stores.push_back(store);
				}
			}
		}
	}

	return register_stores;
}

static int countRedundantRegisterStores(const llvm::DataLayout &dl, std::vector<llvm::StoreInst*> stores)
{
	std::map<int, int> store_count;
	for(auto i : stores) {
		llvm::APInt offset(64, 0, false);
		i->getPointerOperand()->stripAndAccumulateInBoundsConstantOffsets(dl, offset);
		store_count[offset.getZExtValue()]++;
	}

	int redundants = 0;
	for(auto i : store_count) {
		redundants += i.second-1;
	}

	return redundants;
}

static bool isRegisterStore(llvm::Instruction *inst)
{
	if(llvm::isa<llvm::StoreInst>(inst)) {
		llvm::StoreInst *store = (llvm::StoreInst*)inst;

		if(store->getPointerOperand()->stripInBoundsConstantOffsets() == &*inst->getFunction()->arg_begin()) {
			return true;
		}
	}
	return false;
}

static bool isRegisterLoad(llvm::Instruction *inst)
{
	if(llvm::isa<llvm::LoadInst>(inst)) {
		llvm::LoadInst *load = (llvm::LoadInst*)inst;

		if(load->getPointerOperand()->stripInBoundsConstantOffsets() == &*inst->getFunction()->arg_begin()) {
			return true;
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

		if(!call->getCalledFunction()->doesNotAccessMemory()) {
			return true;
		}
	}
	return false;
}

static std::pair<uint64_t, uint64_t> getRegisterStoreInfo(llvm::StoreInst *inst)
{
	llvm::APInt offset(64, 0, false);
	auto dl = inst->getModule()->getDataLayout();
	inst->getPointerOperand()->stripAndAccumulateInBoundsConstantOffsets(dl, offset);

	return {offset.getZExtValue(), dl.getTypeSizeInBits(inst->getValueOperand()->getType()) / 8};
}

static std::pair<uint64_t, uint64_t> getRegisterLoadInfo(llvm::LoadInst *inst)
{
	llvm::APInt offset(64, 0, false);
	auto dl = inst->getModule()->getDataLayout();
	inst->getPointerOperand()->stripAndAccumulateInBoundsConstantOffsets(dl, offset);

	return {offset.getZExtValue(), dl.getTypeSizeInBits(inst->getType()) / 8};
}

using register_info_t = std::pair<uint64_t, uint64_t>;
using block_register_out_t = std::map<register_info_t, llvm::StoreInst*>;

bool registerAccessAliases(const register_info_t &i1, const register_info_t &i2)
{
	if(i1.first >= i2.first + i2.second) {
		return false;
	}
	if(i2.first >= i1.first + i1.second) {
		return false;
	}
	return true;
}

void killRegisterStore(block_register_out_t &info, register_info_t reg_info)
{
	// first, remove anything from existing info which aliases new_store
	std::vector<register_info_t> aliasing_entries;

	for(auto i : info) {
		if(registerAccessAliases(i.first, reg_info)) {
			aliasing_entries.push_back(i.first);
		}
	}

	for(auto i : aliasing_entries) {
		info.erase(i);
	}
}

static block_register_out_t getLiveRegisterOuts(llvm::BasicBlock *block)
{
	block_register_out_t register_outs;

	for(auto &inst : block->getInstList()) {
		if(isRegisterStore(&inst)) {
			auto info = getRegisterStoreInfo((llvm::StoreInst*)&inst);
			killRegisterStore(register_outs, info);
			register_outs[info] = (llvm::StoreInst*)&inst;
		}

		if(isRegisterLoad(&inst)) {
			auto info = getRegisterLoadInfo((llvm::LoadInst*)&inst);
			killRegisterStore(register_outs, info);
		}

		if(isBreaker(&inst)) {
			register_outs.clear();
		}
	}

	return register_outs;
}

static block_register_out_t getLiveRegisterIns(llvm::BasicBlock *block)
{
	block_register_out_t register_ins;

	for(auto i = block->getInstList().rbegin(); i != block->getInstList().rend(); ++i) {
		auto &inst = *i;
		if(isRegisterStore(&inst)) {
			auto info = getRegisterStoreInfo((llvm::StoreInst*)&inst);
			killRegisterStore(register_ins, info);
			register_ins[info] = (llvm::StoreInst*)&inst;
		}

		if(isRegisterLoad(&inst)) {
			auto info = getRegisterLoadInfo((llvm::LoadInst*)&inst);
			killRegisterStore(register_ins, info);
		}

		if(isBreaker(&inst)) {
			register_ins.clear();
		}
	}

	return register_ins;
}

bool LLVMRegisterOptimisationPass::runOnRegion(llvm::Function &f, const llvm::Region* region)
{
	// Do a very basic DSE:
	// 1. For each block B, identify all writes W to registers R
	// 2. For each immediate successor S of B, determine if B writes to R before it is read
	// 3. If all successors write to R before it is read, delete the write W

	// In future this should be extended.

	bool changed = false;

	std::map<llvm::BasicBlock*, block_register_out_t> live_register_outs;

// 	fprintf(stderr, "In Function %s\n", f.getParent()->getName().str().c_str());

	for(auto &b : f.getBasicBlockList()) {
		auto B = &b;
// 		fprintf(stderr, "For block %p (%s)\n", B, B->getName().str().c_str());

		block_register_out_t live_outs = getLiveRegisterOuts(B);
		for(auto i : live_outs) {
// 			fprintf(stderr, " - %p wrote to %u:%u\n", i.second, i.first.first, i.first.second);
		}

		std::map<llvm::BasicBlock*, block_register_out_t> live_register_ins;
		for(auto S : B->getTerminator()->successors()) {
// 			fprintf(stderr, " - For successor %p (%s)\n", S, S->getName().str().c_str());

			auto live_ins = getLiveRegisterIns(S);
			live_register_ins[S] = live_ins;

			for(auto i : live_ins) {
// 				fprintf(stderr, " - - %u:%u is live in\n", i.first.first, i.first.second);
			}
		}

		for(auto R : live_outs) {
			// check each successor to see if R is live
			bool live = true;

			for(auto S : live_register_ins) {
				if(S.second.count(R.first) == 0) {
					live = false;
					break;
				}
			}

			if(live) {
// 				fprintf(stderr, " - Deleted %p (%u:%u)\n", R.second, R.first.first, R.first.second);
				// delete the write to R in B
				R.second->removeFromParent();
				R.second->dropAllReferences();
				delete R.second;
				changed = true;
			}
		}
	}

	return changed;
}

bool LLVMRegisterOptimisationPass::runOnFunction(llvm::Function &f)
{
	const auto *tlregion = getAnalysis<llvm::RegionInfoPass>().getRegionInfo().getTopLevelRegion();

	std::vector<const llvm::Region*> candidate_regions;
	// recurse through regions, identifying those which do not contain loops
	getCandidateRegions(tlregion, candidate_regions);

	bool changed = false;
	changed |= runOnRegion(f, nullptr);

	return changed;
}

void archsim::translate::translate_llvm::LLVMRegisterOptimisationPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.addRequired<llvm::RegionInfoPass>();
	au.setPreservesCFG();
}
