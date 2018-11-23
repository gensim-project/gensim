/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "core/arch/ArchDescriptor.h"
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

static bool isReturn(llvm::Instruction *inst)
{
	return llvm::isa<llvm::ReturnInst>(inst);
}

class BlockInformation
{
public:
	using AccessList = std::vector<RegisterAccess>;

	BlockInformation(llvm::BasicBlock *block)
	{
		ProcessBlock(block);
	}

	void ProcessBlock(llvm::BasicBlock *block)
	{
		for(auto &inst : *block) {
			auto *insn = &inst;
			if(isRegisterLoad(insn)) {
				ProcessLoad(insn);
			} else if(isRegisterStore(insn)) {
				ProcessStore(insn);
			} else if(isBreaker(insn)) {
				ProcessBreaker(insn);
			} else if(isReturn(insn)) {
				ProcessReturn(insn);
			}
		}
	}

	AccessList &GetAccesses()
	{
		return register_accesses_;
	}

private:
	void ProcessLoad(llvm::Instruction *insn)
	{
		PointerInformationProvider pip(insn->getFunction());
		ASSERT(llvm::isa<llvm::LoadInst>(insn));
		llvm::LoadInst* load = (llvm::LoadInst*)insn;

		PointerInformationProvider::PointerInfo info;
		pip.GetPointerInfo(load->getPointerOperand(), info);

		if(info.size() != 3 || info.at(0) != TAG_REG_ACCESS) {
			UNEXPECTED;
		}

		AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference({info.at(1), info.at(1) + info.at(2) - 1}, true)));
	}
	void ProcessStore(llvm::Instruction *insn)
	{
		PointerInformationProvider pip(insn->getFunction());
		ASSERT(llvm::isa<llvm::StoreInst>(insn));
		llvm::StoreInst* store = (llvm::StoreInst*)insn;

		PointerInformationProvider::PointerInfo info;
		pip.GetPointerInfo(store->getPointerOperand(), info);

		if(info.size() != 3 || info.at(0) != TAG_REG_ACCESS) {
			UNEXPECTED;
		}

		AddRegisterAccess(RegisterAccess::Store(insn, RegisterReference({info.at(1), info.at(1) + info.at(2) - 1}, true)));
	}
	void ProcessBreaker(llvm::Instruction *insn)
	{
		// load anything, then store anything

		AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference()));
		AddRegisterAccess(RegisterAccess::Store(insn, RegisterReference()));
	}
	void ProcessReturn(llvm::Instruction *insn)
	{
		// load everything
		AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference()));
	}

	void AddRegisterAccess(const RegisterAccess &access)
	{
		register_accesses_.push_back(access);
	}

	AccessList register_accesses_;
};

class RegisterDefinitions
{
public:

	RegisterDefinitions()
	{
		definitions_[RegisterReference::UnlimitedExtent()] = {};

		FailIfInvariantBroken();
	}

	void AddDefinition(RegisterAccess *access)
	{
		if(access->GetRegRef().IsPrecise()) {
			if(access->GetRegRef().IsUnlimited()) {
				// not sure what kind of access this would even be
				UNIMPLEMENTED;
			}

			EraseDefinitionsForRange(access->GetRegRef().GetExtents());
			SetDefinitionForRange(access->GetRegRef().GetExtents(), access);
		} else {
			if(access->GetRegRef().IsUnlimited()) {
				// add definition to all register ranges
				for(auto i : definitions_) {
					i.second.push_back(access);
				}

			} else {
				// need to add definition to each range touched by incoming
				// extent. This might involve splitting ranges.
				UNIMPLEMENTED;
			}
		}

		FailIfInvariantBroken();
	}

	std::set<RegisterAccess*> GetDefinitions(RegisterReference::Extents extents)
	{
		std::set<RegisterAccess*> output;
		for(auto &i : definitions_) {
			if(extents.first > i.first.second || extents.second < i.first.first) {
				continue;
			}
			output.insert(i.second.begin(), i.second.end());
		}

		return output;
	}

	static RegisterDefinitions MergeDefinitions(std::vector<RegisterDefinitions*> &incoming_defs)
	{
		RegisterDefinitions output_defs;

		for(auto incoming : incoming_defs) {
			for(auto &def : incoming->definitions_) {
				auto &output_def_list = output_defs.definitions_[def.first];
				output_def_list.insert(output_def_list.begin(), def.second.begin(), def.second.end());
			}
		}

		// flatten output defs
		output_defs.Flatten();
		return output_defs;
	}

	bool operator==(const RegisterDefinitions &other) const
	{
		return definitions_ == other.definitions_;
	}
	bool operator!=(const RegisterDefinitions &other) const
	{
		return !operator==(other);
	}

private:
	void InsertDefinition(RegisterReference::Extents n_extent, std::vector<RegisterAccess*> &n_accesses, std::map<RegisterReference::Extents, std::vector<RegisterAccess*>> &output)
	{
		ASSERT(n_extent.second >= n_extent.first);

		for(auto e : output) {
			auto e_extent = e.first;

			// z
			if(e_extent == n_extent) {
				auto &p = output.at(e_extent);
				p.insert(p.begin(), n_accesses.begin(), n_accesses.end());
				return;
			}

			// (a)
			if(e_extent.first == n_extent.first  && n_extent.second > e_extent.second) {
				auto &p = output.at(e_extent);
				p.insert(p.begin(), n_accesses.begin(), n_accesses.end());
				InsertDefinition({e_extent.second+1, n_extent.second}, n_accesses, output);
				return;
			}

			// (b)
			if(n_extent.first > e_extent.first && n_extent.second < e_extent.second) {
				output.erase(e_extent);
				InsertDefinition({e_extent.first, n_extent.first-1}, e.second, output);
				InsertDefinition({n_extent.first, n_extent.second}, e.second, output);
				InsertDefinition({n_extent.first, n_extent.second}, n_accesses, output);
				InsertDefinition({n_extent.second+1, e_extent.second}, e.second, output);
				return;
			}

			// (c)
			if(n_extent.first > e_extent.first && n_extent.second == e_extent.second) {
				output.erase(e.first);
				InsertDefinition({e_extent.first, n_extent.first-1}, e.second, output);
				InsertDefinition({n_extent.first, n_extent.second}, e.second, output);
				InsertDefinition({n_extent.first, n_extent.second}, n_accesses, output);
				return;
			}

			// (d)
			if(e_extent.second >= n_extent.first && e_extent.second <= n_extent.second) {

				//       + [{E.start, N.start-1}] = E.accesses
				//       + [{N.start, E.end}] = E.accesses + N.accesses
				//       + [{E.end+1, N.end}] = N.accesses
				output.erase(e.first);
				InsertDefinition({e_extent.first, n_extent.first-1}, e.second, output);
				InsertDefinition({n_extent.first, e_extent.second}, e.second, output);
				InsertDefinition({n_extent.first, e_extent.second}, n_accesses, output);
				InsertDefinition({e_extent.second+1, n_extent.second}, n_accesses, output);
				return;
			}
		}

		// did not encounter any overlap: add directly to output
		output[n_extent] = n_accesses;
	}

	void Flatten()
	{
		// get a complete. sorted list of definitions to add
		std::map<RegisterReference::Extents, std::vector<RegisterAccess*>> all_extents = definitions_;

		// Add overlapping extents to new definitions one by one. The extents must be added in order.
		// This results in the following situations:
		// 1. The new extent N is after every existing extent
		//    + [{N.start, N.end}] = N.accesses
		// 2. The new extent overlaps an existing extent:
		//    z) it exactly overlaps an existing extent
		//       M [{E.start, E.end}] += N.accesses
		//    a) it starts at the same byte as an existing extent E, but is larger
		//       M [{E.start, E.end}] += N.accesses
		//       + [{E.end+1, N.end}] = N.accesses
		//    b) it starts after an existing extent E, but finishes before it
		//       - E
		//       + [{E.start, N.start-1}] = E.accesses
		//       + [{N.start, N.end}] = E.accesses + N.accesses
		//       + [{N.end+1, E.end}] = E.accesses
		//    c) it starts after an existing extent, but finishes at the same byte as it
		//       - E
		//       + [{E.start, N.start-1}] = E.accesses
		//       + [{N.start, N.end}] = E.accesses + N.accesses
		//    d) it starts after an existing extent, but finishes after it
		//       - E
		//       + [{E.start, N.start-1}] = E.accesses
		//       + [{N.start, E.end}] = E.accesses + N.accesses
		//       + [{E.end+1, N.end}] = N.accesses

		definitions_.clear();
		for(auto d : all_extents) {
			InsertDefinition(d.first, d.second, definitions_);
		}

		FailIfInvariantBroken();
	}

	void SetDefinitionForRange(RegisterReference::Extents extents, RegisterAccess *access)
	{
		definitions_[extents] = {access};

		FailIfInvariantBroken();
	}
	void EraseDefinitionsForRange(RegisterReference::Extents erase)
	{
		std::vector<RegisterReference::Extents> delete_extents;
		std::vector<std::pair<RegisterReference::Extents, std::vector<RegisterAccess*>>> new_extents;

		for(auto current : definitions_) {
			auto curr_extent = current.first;
			// if the extents don't overlap, continue
			if(curr_extent.first > erase.second || erase.first > curr_extent.second) {
				continue;
			}

			delete_extents.push_back(curr_extent);

			// if some of the current extent is left at the start, add a new extent
			if(curr_extent.first < erase.first) {
				new_extents.push_back({{curr_extent.first, erase.first-1}, current.second});
			}

			// if some of the current extent is left at the end, add a new extent
			if(curr_extent.second > erase.second) {
				new_extents.push_back({{erase.second+1, curr_extent.second}, current.second});
			}
		}

		for(auto i : delete_extents) {
			definitions_.erase(i);
		}
		for(auto i : new_extents) {
			definitions_[i.first] = i.second;
		}

		// This does not check the invariant, so should always be followed by
		// something which reinserts values for the deleted extent.
	}
	void FailIfInvariantBroken()
	{
		// there should be precisely one extent for every value from 0 to INT_MAX

		// pairs are sorted by first, then second.
		int current_value = 0;
		for(auto i : definitions_) {
			ASSERT(i.first.first == current_value);
			current_value = i.first.second + 1;
		}
	}

	std::map<RegisterReference::Extents, std::vector<RegisterAccess*>> definitions_;
};

class BlockDefinitions
{
public:
	BlockDefinitions(BlockInformation &block)
	{
		ProcessBlock(block);
	}

	RegisterDefinitions PropagateDefinitions(RegisterDefinitions &incoming_defs, std::vector<RegisterAccess*> &live_accesses)
	{
		RegisterDefinitions outgoing_defs = incoming_defs;

		for(auto &ref : incoming_loaded_regs_) {
			auto touched_stores = incoming_defs.GetDefinitions(ref.GetExtents());
			live_accesses.insert(live_accesses.begin(), touched_stores.begin(), touched_stores.end());
		}

		for(auto outgoing_def : outgoing_stored_regs_) {
			outgoing_defs.AddDefinition(outgoing_def);
		}

		return outgoing_defs;
	}

	const std::vector<RegisterAccess*> &GetLocalLive()
	{
		return locally_live_stores_;
	}

private:
	void ProcessBlock(BlockInformation &block)
	{
		RegisterDefinitions live_definitions;

		for(auto &access : block.GetAccesses()) {
			if(access.IsStore()) {
				live_definitions.AddDefinition(&access);
			} else {
				// we're dealing with a load. Does it load any values
				// which have been stored in this block? If so, mark those
				// stores as locally live.

				auto touched_defs = live_definitions.GetDefinitions(access.GetRegRef().GetExtents());
				for(auto i : touched_defs) {
					locally_live_stores_.push_back(i);

				}

				if(!access.GetRegRef().IsUnlimited()) {
					// if this access is limited, figure out which bytes are
					// not written to before this point in the block: these are
					// live incoming bytes
					std::vector<bool> live (access.GetRegRef().GetSize(), true);
					for(auto i : touched_defs) {
						int start = std::max(access.GetRegRef().GetExtents().first, i->GetRegRef().GetExtents().first);
						int end = std::min(access.GetRegRef().GetExtents().second, i->GetRegRef().GetExtents().second);

						for(int b = start; b < end; ++b) {
							live.at(b - start) = false;
						}
					}

					for(int b = 0; b < live.size(); ++b) {
						if(live.at(b)) {
							auto byte = b + access.GetRegRef().GetExtents().first;
							incoming_loaded_regs_.push_back(RegisterReference({byte, byte}, 1));
						}
					}

				} else {
					// if the access is not limited, treat every incoming byte
					// as being live.

					incoming_loaded_regs_.push_back(RegisterReference());
				}

			}
		}

		// put every currently live access into outgoing_stored_regs.
		auto live_defs = live_definitions.GetDefinitions(RegisterReference::UnlimitedExtent());
		for(auto def : live_defs) {
			outgoing_stored_regs_.push_back(def);
		}
	}

	std::vector<RegisterAccess *> locally_live_stores_;
	std::vector<RegisterReference> incoming_loaded_regs_;
	std::vector<RegisterAccess*> outgoing_stored_regs_;
};

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

static void GetDefinitions(BlockInformation &block, std::vector<RegisterAccess*> &all_defs)
{
	for(auto &i : block.GetAccesses()) {
		if(i.IsStore()) {
			all_defs.push_back(&i);
		}
	}
}

bool LLVMRegisterOptimisationPass::ProcessFunction(llvm::Function &f, std::vector<RegisterAccess*> &all_definitions, std::vector<RegisterAccess*> &live_definitions)
{
	std::set<llvm::BasicBlock *> work_set;
	std::map<llvm::BasicBlock *, RegisterDefinitions> outgoing_defs;

	std::map<llvm::BasicBlock*, BlockInformation*> block_info;
	std::map<llvm::BasicBlock*, BlockDefinitions*> block_defs;
	std::vector<RegisterDefinitions*> incoming_defs;

	for(auto &b : f) {
		work_set.insert(&b);
		outgoing_defs[&b];

		block_info.insert({&b, new BlockInformation(&b)});
		block_defs.insert({&b, new BlockDefinitions(*block_info.at(&b))});

		for(auto i : block_defs.at(&b)->GetLocalLive()) {
			live_definitions.push_back(i);
		}
		GetDefinitions(*block_info.at(&b), all_definitions);
	}

	while(!work_set.empty()) {
		auto *block = *work_set.begin();
		work_set.erase(block);

		incoming_defs.clear();
		for(auto pred : GetPredecessors(block)) {
			incoming_defs.push_back(&outgoing_defs.at(pred));
		}
		auto incoming_def_set = RegisterDefinitions::MergeDefinitions(incoming_defs);

		auto new_outgoing_defs = block_defs.at(block)->PropagateDefinitions(incoming_def_set, live_definitions);
		if(new_outgoing_defs != outgoing_defs.at(block)) {
			outgoing_defs[block] = new_outgoing_defs;

			auto terminator = block->getTerminator();
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

	std::vector<RegisterAccess*> all_definitions, live_definitions, dead_definitions;
	ProcessFunction(f, all_definitions, live_definitions);

	std::sort(all_definitions.begin(), all_definitions.end());
	std::sort(live_definitions.begin(), live_definitions.end());

	std::set_difference(all_definitions.begin(), all_definitions.end(), live_definitions.begin(), live_definitions.end(), std::inserter(dead_definitions, dead_definitions.begin()));
	auto last = std::unique(dead_definitions.begin(), dead_definitions.end());
	dead_definitions.erase(last, dead_definitions.end());

	DEBUGLOG(" *** Found %u/%u/%u all/live/dead defs\n", all_definitions.size(), live_definitions.size(), dead_definitions.size());
//	fprintf(stderr, " *** Found %u/%u/%u all/live/dead defs\n", all_definitions.size(), live_definitions.size(), dead_definitions.size());

	for(auto dead_def : dead_definitions) {
		auto dead_store = dead_def->GetInstruction();

		if(llvm::isa<llvm::StoreInst>(dead_store)) {
			dead_store->removeFromParent();
			dead_store->deleteValue();
			changed = true;
		}
	}

	return changed;
}

void archsim::translate::translate_llvm::LLVMRegisterOptimisationPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.setPreservesCFG();
}
