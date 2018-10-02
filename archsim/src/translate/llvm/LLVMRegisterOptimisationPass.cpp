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

bool LLVMRegisterOptimisationPass::runOnRegion(const llvm::Region* region)
{
	return false;
}

bool LLVMRegisterOptimisationPass::runOnFunction(llvm::Function &f)
{
	const auto *tlregion = getAnalysis<llvm::RegionInfoPass>().getRegionInfo().getTopLevelRegion();

	std::vector<const llvm::Region*> candidate_regions;
	// recurse through regions, identifying those which do not contain loops
	getCandidateRegions(tlregion, candidate_regions);

	bool changed = false;
	for(auto i : candidate_regions) {
		changed |= runOnRegion(i);

// 		printregion(i);
// 		auto stores = getRegisterStores(&*f.arg_begin(), i);
// 		auto count = countRedundantRegisterStores(f.getParent()->getDataLayout(), stores);
// 		fprintf(stderr, "### %u redundant stores\n", count);
	}

	return changed;
}

void archsim::translate::translate_llvm::LLVMRegisterOptimisationPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.addRequired<llvm::RegionInfoPass>();
	au.setPreservesCFG();
}
