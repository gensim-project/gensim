/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "core/arch/ArchDescriptor.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include "translate/llvm/LLVMRegisterOptimisationPass.h"
#include <llvm/Analysis/RegionInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/PassSupport.h>
#include <llvm/Support/raw_os_ostream.h>

#include <sstream>
#include <wutils/vset.h>

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

std::ostream &operator<<(std::ostream &str, const RegisterReference &ref)
{
	str << "(";
	if(ref.IsUnlimited()) {
		str << ref.GetExtents().first << ", infinity";
	} else {
		str << ref.GetExtents().first << ", " << ref.GetExtents().second;
	}
	str << ")";
	if(!ref.IsPrecise()) {
		str << "?";
	}

	return str;
}

std::ostream &operator<<(std::ostream &str, const RegisterAccess &access)
{
	llvm::raw_os_ostream llvm_str(str);

	llvm_str << *access.GetInstruction() << ": ";
	if(access.IsStore()) {
		llvm_str << " store ";
	} else {
		llvm_str << " load ";
	}
	str << access.GetRegRef();
	return str;
}

class BlockInformation
{
public:
	using AccessList = std::vector<RegisterAccess>;

	BlockInformation(llvm::BasicBlock *block, TagContext &tags) : tags_(tags)
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

	const AccessList &GetAccesses() const
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

		if(info.size() < 1 || info.at(0) != TAG_REG_ACCESS) {
			UNEXPECTED;
		}

		if(info.size() != 3) {
			// treat as an imprecise unlimited load
			// TODO: fix this
			AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference(), tags_.NextTag()));
		} else {
			AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference({info.at(1), info.at(1) + info.at(2) - 1}, true), tags_.NextTag()));
		}
	}
	void ProcessStore(llvm::Instruction *insn)
	{
		PointerInformationProvider pip(insn->getFunction());
		ASSERT(llvm::isa<llvm::StoreInst>(insn));
		llvm::StoreInst* store = (llvm::StoreInst*)insn;

		PointerInformationProvider::PointerInfo info;
		pip.GetPointerInfo(store->getPointerOperand(), info);

		if(info.size() < 1 || info.at(0) != TAG_REG_ACCESS) {
			UNEXPECTED;
		}

		if(info.size() != 3) {
			// treat as an imprecise unlimited load
			// TODO: fix this
			AddRegisterAccess(RegisterAccess::Store(insn, RegisterReference(), tags_.NextTag()));
		} else {
			AddRegisterAccess(RegisterAccess::Store(insn, RegisterReference({info.at(1), info.at(1) + info.at(2) - 1}, true), tags_.NextTag()));
		}
	}
	void ProcessBreaker(llvm::Instruction *insn)
	{
		// load anything, then store anything

		AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference(), tags_.NextTag()));
		AddRegisterAccess(RegisterAccess::Store(insn, RegisterReference(), tags_.NextTag()));
	}
	void ProcessReturn(llvm::Instruction *insn)
	{
		// load everything
		AddRegisterAccess(RegisterAccess::Load(insn, RegisterReference(), tags_.NextTag()));
	}

	void AddRegisterAccess(const RegisterAccess &access)
	{
		register_accesses_.push_back(access);
	}

	TagContext &tags_;
	AccessList register_accesses_;
};

std::ostream &operator<< (std::ostream &str, const BlockInformation &block)
{
	str << "block info {" << &block << "}:" << std::endl;
	for(auto &access : block.GetAccesses()) {
		str << " - " << access << std::endl;
	}
	return str;
}

void RegisterDefinitions::AddDefinition(RegisterAccess* access)
{
	if (access->GetRegRef().IsPrecise()) {
		if (access->GetRegRef().IsUnlimited()) {
			// not sure what kind of access this would even be
			UNIMPLEMENTED;
		}

		EraseDefinitionsForRange(access->GetRegRef().GetExtents());
		SetDefinitionForRange(access->GetRegRef().GetExtents(), access);
	} else {
		if (access->GetRegRef().IsUnlimited()) {
			// add definition to all register ranges
			for (auto &i : definitions_) {
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

void RegisterDefinitions::SetDefinitionForRange(RegisterReference::Extents extents, RegisterAccess* access)
{
	definitions_.emplace(extents, std::vector<RegisterAccess*> {access});

	FailIfInvariantBroken();
}

RegisterDefinitions RegisterDefinitions::MergeDefinitions(const std::vector<RegisterDefinitions*>& incoming_defs)
{
	if(incoming_defs.size() == 1) {
		return *incoming_defs.at(0);
	}

	RegisterDefinitions output_defs;

	std::map<RegisterReference::Extents, wutils::vset<RegisterAccess*>> new_accesses;

	for (auto incoming : incoming_defs) {
		for (auto &def : incoming->definitions_) {
			auto &accesses = new_accesses[def.first];
			accesses.insert(def.second.begin(), def.second.end());
		}
	}
	for(auto &def : new_accesses) {
		auto &merged = def.second;
		auto &output_def = output_defs.definitions_[def.first];

		output_def.insert(output_def.end(), merged.begin(), merged.end());
	}

	// flatten output defs
	output_defs.Flatten();
	return output_defs;
}

RegisterReference::Extents RegisterReference::GetExtents() const
{
	return extents_;
}

wutils::vset<RegisterAccess*> RegisterDefinitions::GetDefinitions(RegisterReference::Extents extents) const
{
	wutils::vset<RegisterAccess*> output;
	for (auto &i : definitions_) {
		// definitions are ordered so if i.first > extents.second, then return
		if(i.first.first > extents.second) {
			break;
		}
		if (extents.first > i.first.second || extents.second < i.first.first) {
			continue;
		}
		output.insert(i.second.begin(), i.second.end());
	}

	return output;
}

void RegisterDefinitions::Flatten()
{
	// get a complete. sorted list of definitions to add
	std::map<RegisterReference::Extents, std::vector < RegisterAccess*>> new_definitions;

	// Pre populate definitions_ with the merged extent. First, we need to
	// figure out what those extents are though.
	// 1. Figure out the top most non infinite byte
	// 2. Create a vector of bools that length.
	//    This vector represents the edges in the output extent set.
	//    If an extent (x, y) exists, then edges (x, y+1) exist.
	// 3. Iterate over that vector, creating empty extents in the output
	//    extent set as we go.

	int top_most_byte = -1;
	for (auto extent : definitions_) {
		if (extent.first.second == INT_MAX) {
			continue;
		}
		if (extent.first.second > top_most_byte) {
			top_most_byte = extent.first.second;
		}
	}

	if (top_most_byte == -1) {
		// nothing mode to do, apparently. We have only one infinitely sized definition
		FailIfInvariantBroken();
		return;
	}

	std::vector<bool> edges(top_most_byte + 2, false);
	for (auto extent : definitions_) {
		edges.at(extent.first.first) = true;

		if (extent.first.second != INT_MAX) {
			edges.at(extent.first.second + 1) = true;
		}
	}

	ASSERT(edges.at(0)); // there must be an edge at byte 0

	int extent_start = 0;
	for (int b = 1; b < edges.size(); ++b) { // skip the edge at byte 0
		if (edges.at(b)) {
			new_definitions[ {extent_start, b - 1}] = {};
			extent_start = b;
		}
	}
	new_definitions[ {extent_start, INT_MAX}];

	for (auto &new_def : new_definitions) {
		auto new_extent = new_def.first;
		for (const auto &old_def : definitions_) {
			auto old_extent = old_def.first;

			if (old_extent.first > new_extent.second) {
				break;
			}

			if (new_extent.first > old_extent.second) {
				continue;
			}

			new_def.second.insert(new_def.second.end(), old_def.second.begin(), old_def.second.end());
		}
	}

	definitions_ = std::move(new_definitions);

	FailIfInvariantBroken();
}

void RegisterDefinitions::FailIfInvariantBroken() const
{
#ifndef NDEBUG
	// there should be precisely one extent for every value from 0 to INT_MAX

	// pairs are sorted by first, then second.
	int current_value = 0;
	for (auto &i : definitions_) {
		ASSERT(i.first.first == current_value);
		current_value = i.first.second + 1;
	}
#endif
}

void RegisterDefinitions::EraseDefinitionsForRange(RegisterReference::Extents erase)
{
	std::vector<RegisterReference::Extents> delete_extents;
	std::vector<std::pair<RegisterReference::Extents, std::vector <RegisterAccess*>>> new_extents;

	for (auto &current : definitions_) {
		auto curr_extent = current.first;
		// if the extents don't overlap, continue
		if (curr_extent.first > erase.second || erase.first > curr_extent.second) {
			continue;
		}

		delete_extents.push_back(curr_extent);

		// if some of the current extent is left at the start, add a new extent
		if (curr_extent.first < erase.first) {
			new_extents.push_back( {
				{curr_extent.first, erase.first - 1}, current.second
			});
		}

		// if some of the current extent is left at the end, add a new extent
		if (curr_extent.second > erase.second) {
			new_extents.push_back( {
				{erase.second + 1, curr_extent.second}, current.second
			});
		}
	}

	for (auto i : delete_extents) {
		definitions_.erase(i);
	}
	for (auto i : new_extents) {
		definitions_.emplace(i.first, i.second);
	}

	// This does not check the invariant, so should always be followed by
	// something which reinserts values for the deleted extent.
}


std::ostream &operator<<(std::ostream &str, const RegisterDefinitions &defs)
{
	str << "defs(";

	for(const auto &def : defs.GetDefinitions(RegisterReference::UnlimitedExtent())) {
		str << def->GetRegRef();
		if(def->IsStore()) {
			str << " <= ";
		} else {
			str << " => ";
		}
		str << def->GetInstruction();

		str << ", ";
	}

	str << ")";
	return str;
}

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

			live_accesses.insert(live_accesses.end(), touched_stores.begin(), touched_stores.end());
		}

		for(auto outgoing_def : outgoing_stored_regs_) {
			outgoing_defs.AddDefinition(outgoing_def);
		}

		return outgoing_defs;
	}

	const std::vector<RegisterAccess*> &GetLocalLive() const
	{
		return locally_live_stores_;
	}

	const std::vector<RegisterReference> &GetIncomingLoads() const
	{
		return incoming_loaded_regs_;
	}

	const std::vector<RegisterAccess*> &GetOutgoingStores() const
	{
		return outgoing_stored_regs_;
	}

private:
	std::vector<RegisterReference> ConvertToImpreciseReferences(const std::vector<bool> &bytes)
	{
		std::vector<RegisterReference> refs;

		int ref_start = -1;
		int ref_end = -1;
		for(int b = 0; b < bytes.size(); ++b) {
			if(bytes[b]) {
				if(ref_start == -1) {
					ref_start = b;
				}
				ref_end = b;
			} else {
				if(ref_start != -1) {
					refs.push_back(RegisterReference({ref_start, ref_end}, false));
					ref_start = -1;
					ref_end = -1;
				}
			}
		}
		if(ref_start != -1) {
			refs.push_back(RegisterReference({ref_start, ref_end}, false));
		}

		return refs;
	}

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

						for(int b = start; b <= end; ++b) {
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
					// if the access is not limited, we need to treat every
					// incoming byte as live, except for those which have been
					// precisely written before this load occurs.

					auto all_defs = live_definitions.GetDefinitions(RegisterReference::UnlimitedExtent());

					if(all_defs.empty()) {
						// no previous defs, so add a 0-unlimited load
						incoming_loaded_regs_.push_back(RegisterReference({0, INT_MAX}, false));
					} else {
						int max_byte = 0;

						for(auto store : all_defs) {
							if(store->GetRegRef().IsUnlimited()) {
								continue;
							}

							auto store_max = store->GetRegRef().GetExtents().second;
							if(store_max > max_byte) {
								max_byte = store_max;
							}
						}

						// todo: do this better
						std::vector<bool> incoming_bytes(max_byte+1, 1);
						for(auto store : all_defs) {
							if(store->GetRegRef().IsPrecise()) {
								for(int b = store->GetRegRef().GetExtents().first; b <= store->GetRegRef().GetExtents().second; ++b) {
									incoming_bytes.at(b) = false;
								}
							}
						}

						// now convert incoming bytes to extents
						auto refs = ConvertToImpreciseReferences(incoming_bytes);
						incoming_loaded_regs_.insert(incoming_loaded_regs_.end(), refs.begin(), refs.end());
						incoming_loaded_regs_.push_back(RegisterReference({max_byte+1, INT_MAX}, false));
					}
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

std::ostream &operator<<(std::ostream &str, const BlockDefinitions &defs)
{
	str << "Block Defs for {" << &defs << "}:" << std::endl;
	str << " - Incoming loads:" << std::endl;
	for(auto in : defs.GetIncomingLoads()) {
		str << " - - " << in << std::endl;
	}

	str << " - Locally live refs:" << std::endl;
	for(auto &local : defs.GetLocalLive()) {
		str << " - - " << *local << std::endl;
	}

	str << " - Outgoing stores:" << std::endl;
	for(auto &out : defs.GetOutgoingStores()) {
		str << " - - " << *out << std::endl;
	}
	return str;
}

std::vector<llvm::BasicBlock*> GetPredecessors(llvm::BasicBlock *block)
{
	std::vector<llvm::BasicBlock*> preds;
	for(auto &b : *(block->getParent())) {
		for(int i = 0; i < b.getTerminator()->getNumSuccessors(); ++i) {
			auto p = b.getTerminator()->getSuccessor(i);
			if(p == block) {
				preds.push_back(&b);
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
	TagContext tags;

	std::unordered_set<llvm::BasicBlock *> work_set;
	std::map<llvm::BasicBlock *, RegisterDefinitions> outgoing_defs;

	std::map<llvm::BasicBlock*, BlockInformation*> block_info;
	std::map<llvm::BasicBlock*, BlockDefinitions*> block_defs;
	std::map<llvm::BasicBlock*, std::vector<llvm::BasicBlock*>> preds;
	std::vector<RegisterDefinitions*> incoming_defs;

	for(auto &b : f) {
		work_set.insert(&b);
		outgoing_defs[&b];

		block_info.insert({&b, new BlockInformation(&b, tags)});
		block_defs.insert({&b, new BlockDefinitions(*block_info.at(&b))});

		for(auto i : block_defs.at(&b)->GetLocalLive()) {
			live_definitions.push_back(i);
		}
		GetDefinitions(*block_info.at(&b), all_definitions);

		preds[&b] = GetPredecessors(&b);
//		std::cout << *block_info.at(&b) << std::endl << *block_defs.at(&b) << std::endl;
	}

	while(!work_set.empty()) {
		auto *block = *work_set.begin();
		work_set.erase(block);

		incoming_defs.clear();
		for(auto pred : preds.at(block)) {
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

static void deep_delete(llvm::Instruction *insn)
{
	if(insn->hasNUsesOrMore(1)) {
		return;
	}

	for(int i = 0; i < insn->getNumOperands(); ++i) {
		auto operand = insn->getOperand(i);
		if(llvm::isa<llvm::Instruction>(operand)) {
			auto child = (llvm::Instruction*)operand;
			insn->setOperand(i, nullptr);
			deep_delete(child);
		}
	}

	insn->removeFromParent();
	insn->deleteValue();
}

bool LLVMRegisterOptimisationPass::runOnFunction(llvm::Function &f)
{
	DEBUGLOG(" *** Starting to work on %s\n", f.getName().str().c_str());

	bool changed = false;

//	fprintf(stderr, " *** Found %u/%u/%u all/live/dead defs\n", all_definitions.size(), live_definitions.size(), dead_definitions.size());
	auto dead_definitions = getDeadStores(f);

	for(auto dead_def : dead_definitions) {
		auto dead_store = dead_def->GetInstruction();
		deep_delete(dead_store);

		changed = true;
	}

	return changed;
}

std::vector<RegisterAccess*> LLVMRegisterOptimisationPass::getDeadStores(llvm::Function& f)
{
	std::vector<RegisterAccess*> all_definitions, dead_definitions, live_definitions;
	ProcessFunction(f, all_definitions, live_definitions);

	std::sort(all_definitions.begin(), all_definitions.end());
	std::sort(live_definitions.begin(), live_definitions.end());

	std::set_difference(all_definitions.begin(), all_definitions.end(), live_definitions.begin(), live_definitions.end(), std::inserter(dead_definitions, dead_definitions.begin()));
	auto last = std::unique(dead_definitions.begin(), dead_definitions.end());
	dead_definitions.erase(last, dead_definitions.end());

	DEBUGLOG(" *** Found %u/%u/%u all/live/dead defs\n", all_definitions.size(), live_definitions.size(), dead_definitions.size());

	return dead_definitions;
}


void archsim::translate::translate_llvm::LLVMRegisterOptimisationPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.setPreservesCFG();
}
