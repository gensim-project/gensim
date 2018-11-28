/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMGuestRegisterAccessEmitter.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include "util/LogContext.h"
#include <wutils/vbitset.h>

#include <llvm/IR/DerivedTypes.h>

DeclareLogContext(LogGRAE, "GRAE");
using namespace archsim::translate::translate_llvm;

LLVMGuestRegisterAccessEmitter::LLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : ctx_(ctx)
{

}

LLVMGuestRegisterAccessEmitter::~LLVMGuestRegisterAccessEmitter()
{

}

void LLVMGuestRegisterAccessEmitter::Finalize()
{

}

void LLVMGuestRegisterAccessEmitter::Reset()
{

}

void LLVMGuestRegisterAccessEmitter::AddAAAIMetadata(llvm::Value* target, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	if(llvm::isa<llvm::Instruction>(target)) {
		llvm::Instruction *insn = llvm::dyn_cast<llvm::Instruction>(target);

		insn->setMetadata("aaai", GetMetadataFor(reg, index));
	} else if(llvm::isa<llvm::ConstantExpr>(target)) {
//		llvm::ConstantExpr *insn = llvm::dyn_cast<llvm::ConstantExpr>(target);
//		insn->setMetadata("aaai", GetMetadataFor(reg, index));
	} else {
		// coukldn't figure out what AAAI to add
	}
}

llvm::MDNode *LLVMGuestRegisterAccessEmitter::GetMetadataFor(const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	std::vector<int> output {archsim::translate::translate_llvm::TAG_REG_ACCESS};

	std::vector<llvm::Metadata*> mddata;
	for(auto i : output) {
		mddata.push_back(llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(GetCtx().Types.i64, i)));
	}
	return llvm::MDNode::get(GetCtx().LLVMCtx, mddata);
}

BasicLLVMGuestRegisterAccessEmitter::BasicLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : LLVMGuestRegisterAccessEmitter(ctx)
{

}

BasicLLVMGuestRegisterAccessEmitter::~BasicLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* BasicLLVMGuestRegisterAccessEmitter::EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	auto ptr = GetRegisterPointer(builder, reg, index);
	return builder.CreateLoad(ptr, "register_entry_from_" + reg.GetName());
}

void BasicLLVMGuestRegisterAccessEmitter::EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	auto ptr = GetRegisterPointer(builder, reg, index);
	ptr = builder.CreatePointerCast(ptr, value->getType()->getPointerTo(0));
	builder.CreateStore(value, ptr);
}

llvm::Value* BasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	auto ptr = GetCtx().Values.reg_file_ptr;
	ptr = builder.CreatePtrToInt(ptr, GetCtx().Types.i64);
	if(reg.GetOffset() != 0) {
		ptr = builder.CreateAdd(ptr, llvm::ConstantInt::get(GetCtx().Types.i64, reg.GetOffset()), "ptr_to_" + reg.GetName());
	}
	if(index != nullptr) {
		index = builder.CreateZExt(index, GetCtx().Types.i64);
		ptr = builder.CreateAdd(ptr, builder.CreateMul(index, llvm::ConstantInt::get(GetCtx().Types.i64, reg.GetRegisterStride())), "ptr_to_" + reg.GetName() + "_entry");
	}
	ptr = builder.CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(GetCtx().LLVMCtx, reg.GetRegisterSize()*8));
	return ptr;
}

CachingBasicLLVMGuestRegisterAccessEmitter::CachingBasicLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : BasicLLVMGuestRegisterAccessEmitter(ctx)
{

}

CachingBasicLLVMGuestRegisterAccessEmitter::~CachingBasicLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* CachingBasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	bool index_is_constant = false;
	uint64_t constant_index = 0;
	if(index == nullptr) {
		index_is_constant = true;
	} else if(llvm::isa<llvm::ConstantInt>(index)) {
		constant_index = llvm::dyn_cast<llvm::ConstantInt>(index)->getZExtValue();
		index_is_constant = true;
	}

	if(index_is_constant) {
		if(!register_pointer_cache_.count({reg.GetName(), constant_index})) {
			llvm::IRBuilder<> entry_builder (GetCtx().LLVMCtx);
			entry_builder.SetInsertPoint(&builder.GetInsertBlock()->getParent()->getEntryBlock(), builder.GetInsertBlock()->getParent()->getEntryBlock().begin());

			register_pointer_cache_[ {reg.GetName(), constant_index}] = BasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(entry_builder, reg, index);
		}

		return register_pointer_cache_.at({reg.GetName(), constant_index});
	} else {
		register_pointer_cache_.clear();
		return BasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(builder, reg, index);
	}
}


GEPLLVMGuestRegisterAccessEmitter::GEPLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : LLVMGuestRegisterAccessEmitter(ctx)
{

}

GEPLLVMGuestRegisterAccessEmitter::~GEPLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* GEPLLVMGuestRegisterAccessEmitter::EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	return builder.CreateLoad(GetPointerToReg(builder, reg, index));
}

void GEPLLVMGuestRegisterAccessEmitter::EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	builder.CreateStore(value, GetPointerToReg(builder, reg, index));
}

llvm::Value* GEPLLVMGuestRegisterAccessEmitter::GetPointerToReg(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg_view, llvm::Value* index)
{
	if(index == nullptr) {
		index = llvm::ConstantInt::get(GetCtx().Types.i64, 0);
	}

	llvm::Value *ptr = nullptr;
	llvm::IRBuilder<> ptr_builder = builder;

	if(llvm::isa<llvm::ConstantInt>(index)) {
		ptr_builder.SetInsertPoint(&builder.GetInsertBlock()->getParent()->getEntryBlock(), builder.GetInsertBlock()->getParent()->getEntryBlock().begin());

		llvm::ConstantInt *c = (llvm::ConstantInt*)index;
		if(c->getZExtValue() >= reg_view.GetRegisterCount()) {
			throw std::logic_error("Register access out of bounds!");
		}

		if(!register_pointer_cache_.count({reg_view.GetName(), c->getZExtValue()})) {
			ptr = GetPointerToRegBank(ptr_builder, reg_view);
			ptr = ptr_builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, 0), index, llvm::ConstantInt::get(GetCtx().Types.i32, 0)});
			register_pointer_cache_[ {reg_view.GetName(), c->getZExtValue()}] = ptr;
		} else {
			ptr = register_pointer_cache_[ {reg_view.GetName(), c->getZExtValue()}];
		}
	} else {
		// Emit range check if debug is enabled
		if(archsim::options::Debug) {
			llvm::BasicBlock *range_ok = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());
			llvm::BasicBlock *range_bad = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

			auto in_range = ptr_builder.CreateICmpULT(index, llvm::ConstantInt::get(index->getType(), reg_view.GetRegisterCount()));
			ptr_builder.CreateCondBr(in_range, range_ok, range_bad);

			ptr_builder.SetInsertPoint(range_bad);
			ptr_builder.CreateCall(GetCtx().Functions.debug_trap);
			ptr_builder.CreateRet(llvm::ConstantInt::get(GetCtx().Types.i32, 1));

			ptr_builder.SetInsertPoint(range_ok);
			ptr = GetPointerToRegBank(ptr_builder, reg_view);
			ptr = ptr_builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, 0), index, llvm::ConstantInt::get(GetCtx().Types.i32, 0)});

			builder.SetInsertPoint(range_ok);
		} else {
			ptr = GetPointerToRegBank(ptr_builder, reg_view);
			ptr = ptr_builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, 0), index, llvm::ConstantInt::get(GetCtx().Types.i32, 0)});
		}
	}

	AddAAAIMetadata(ptr, reg_view, index);

	return ptr;
}

llvm::Value* GEPLLVMGuestRegisterAccessEmitter::GetPointerToRegBank(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg_view)
{
	llvm::Value *ptr = GetCtx().Values.reg_file_ptr;
	ptr = builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, reg_view.GetOffset())});
	ptr = builder.CreatePointerCast(ptr, GetTypeForRegView(reg_view)->getPointerTo(0));
	AddAAAIMetadata(ptr, reg_view, nullptr);
	return ptr;
}

llvm::Type* LLVMGuestRegisterAccessEmitter::GetTypeForRegView(const archsim::RegisterFileEntryDescriptor& reg_view)
{
	return llvm::ArrayType::get(GetTypeForRegViewEntry(reg_view), reg_view.GetRegisterCount());
}

llvm::Type* LLVMGuestRegisterAccessEmitter::GetTypeForRegViewEntry(const archsim::RegisterFileEntryDescriptor& reg_view)
{
	llvm::Type *base_type = nullptr;
	if(reg_view.GetEntryCountPerRegister() > 1) {
		// vector register
		base_type = llvm::IntegerType::get(GetCtx().LLVMCtx, reg_view.GetEntrySize()*8);
		base_type = llvm::VectorType::get(base_type, reg_view.GetEntryCountPerRegister());
	} else {
		// scalar register
		base_type = llvm::IntegerType::get(GetCtx().LLVMCtx, reg_view.GetEntrySize()*8);
	}

	assert(reg_view.GetRegisterStride() >= reg_view.GetRegisterSize());
	int padding_bytes = reg_view.GetRegisterStride() - reg_view.GetRegisterSize();

	assert(padding_bytes >= 0);

	std::vector<llvm::Type*> entries;
	entries.push_back(base_type);

	if(padding_bytes) {
		llvm::ArrayType *padding_array = llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(GetCtx().LLVMCtx), padding_bytes);
		entries.push_back(padding_array);
	}

	return llvm::StructType::get(GetCtx().LLVMCtx, entries);
}

ShadowLLVMGuestRegisterAccessEmitter::ShadowLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : LLVMGuestRegisterAccessEmitter(ctx)
{

}

ShadowLLVMGuestRegisterAccessEmitter::~ShadowLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* ShadowLLVMGuestRegisterAccessEmitter::EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	if(index == nullptr) {
		LC_DEBUG1(LogGRAE) << "Emitting Register Read " << reg.GetName() << " (no index)";
	} else {
		LC_DEBUG1(LogGRAE) << "Emitting Register Read " << reg.GetName() << " " << index;
	}

	if(index == nullptr) {
		index = llvm::ConstantInt::get(GetCtx().Types.i32, 0);
	}

	auto ptr = GetUndefPtrFor(reg);
	ptr = builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i32, 0), llvm::ConstantInt::get(GetCtx().Types.i32, 0)});
	auto load = builder.CreateLoad(ptr);

	loads_[ {&reg, index}].push_back(load);

	return load;
}

void ShadowLLVMGuestRegisterAccessEmitter::EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	if(index == nullptr) {
		LC_DEBUG1(LogGRAE) << "Emitting Register Write " << reg.GetName() << " (no index)";
	} else {
		LC_DEBUG1(LogGRAE) << "Emitting Register Write " << reg.GetName() << " " << index;
	}

	if(index == nullptr) {
		index = llvm::ConstantInt::get(GetCtx().Types.i32, 0);
	}

	auto ptr = GetUndefPtrFor(reg);
	ptr = builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i32, 0), llvm::ConstantInt::get(GetCtx().Types.i32, 0)});
	auto store = builder.CreateStore(value, ptr);
	stores_[ {&reg, index}].push_back(store);
}

static bool intervals_overlap(int a1, int a2, int b1, int b2)
{
	return !((b1 > a2) || (a2 > b2));
}

void ShadowLLVMGuestRegisterAccessEmitter::Finalize()
{
	auto intervals = GetIntervals();
	auto allocas = CreateAllocas(intervals);

	// Fix loads and stores
	FixLoads(allocas);
	FixStores(allocas);

	// insert loads/saves of shadow registers
	// load registers on entry
	LoadShadowRegs(GetCtx().GetFunction()->getEntryBlock().getTerminator(), allocas);

	for(auto &block : *GetCtx().GetFunction()) {
		for(auto &inst : block) {
			if(llvm::isa<llvm::ReturnInst>(inst)) {
				SaveShadowRegs(&inst, allocas);
			}

			if(llvm::isa<llvm::CallInst>(inst)) {
				llvm::CallInst *call = (llvm::CallInst*)&inst;

				// if it's an indirect call, it must be a chain
				if(call->getCalledFunction() == nullptr) {
					SaveShadowRegs(&inst, allocas);
				} else if(!call->getCalledFunction()->doesNotAccessMemory()) {
					SaveShadowRegs(&inst, allocas);
					LoadShadowRegs(inst.getNextNode(), allocas);
				}
			}
		}
	}
}

void ShadowLLVMGuestRegisterAccessEmitter::Reset()
{
	stores_.clear();
	loads_.clear();
}

llvm::Value* ShadowLLVMGuestRegisterAccessEmitter::GetUndefPtrFor(const archsim::RegisterFileEntryDescriptor& reg)
{
	return llvm::UndefValue::get(GetTypeForRegViewEntry(reg)->getPointerTo(0));
}

std::pair<int, int> ShadowLLVMGuestRegisterAccessEmitter::GetInterval(const register_descriptor_t& reg)
{
	auto register_entry = reg.first;
	auto index = reg.second;

	// two situations:
	// 1. index is constant, so we return an interval which covers exactly one register
	// 2. index is non constant, so we return an interval which covers an entire register bank

	if(llvm::isa<llvm::ConstantInt>(index)) {
		uint64_t constant_index = llvm::dyn_cast<llvm::ConstantInt>(index)->getZExtValue();

		int lower_bound = register_entry->GetOffset() + (constant_index * register_entry->GetRegisterStride());
		int upper_bound = lower_bound + register_entry->GetRegisterSize();

		return {lower_bound, upper_bound};
	} else {
		int lower_bound = register_entry->GetOffset();
		int upper_bound = lower_bound + (register_entry->GetRegisterStride() * register_entry->GetRegisterCount());

		return {lower_bound, upper_bound};
	}
}

static std::set<std::pair<int, int>> ConvertToIntervals(const wutils::vbitset<> &bits)
{
	ShadowLLVMGuestRegisterAccessEmitter::intervals_t intervals;

	int interval_start = 0;
	int interval_end = 0;
	bool in_interval = false;

	unsigned int index = 0;
	while(index < bits.size()) {

		bool bit = bits.get(index);
		if(in_interval) {
			if(bit) {
				// continue current interval
			} else {
				// end interval

				intervals.insert({interval_start, index});
				in_interval = false;
			}

		} else {
			if(bit) {
				// start a new interval
				interval_start = index;
				in_interval = true;
			}
		}

		index++;
	}

	return intervals;
}

ShadowLLVMGuestRegisterAccessEmitter::intervals_t ShadowLLVMGuestRegisterAccessEmitter::GetIntervals()
{
	wutils::vbitset<> bits (GetCtx().GetArch().GetRegisterFileDescriptor().GetSize() * 8);

	for(auto load : loads_) {
		auto loaded_interval = GetInterval(load.first);

		for(int i = loaded_interval.first; i < loaded_interval.second; ++i) {
			bits.set(i, true);
		}
	}
	for(auto store : stores_) {
		auto stored_interval = GetInterval(store.first);

		// merge into intervals
		for(int i = stored_interval.first; i < stored_interval.second; ++i) {
			bits.set(i, true);
		}
	}

	return ConvertToIntervals(bits);
}

ShadowLLVMGuestRegisterAccessEmitter::allocas_t ShadowLLVMGuestRegisterAccessEmitter::CreateAllocas(const intervals_t& intervals)
{
	// create an alloca for each interval
	allocas_t allocas;

	// insert allocas at the start of the entry block
	auto insertion_point = &GetCtx().GetFunction()->getEntryBlock().front();
	for(auto i : intervals) {
		auto type = GetIntervalType(i);
		auto alloca = new llvm::AllocaInst(type, 0, nullptr, "", insertion_point);

		allocas[i] = alloca;
	}

	return allocas;
}

void ShadowLLVMGuestRegisterAccessEmitter::FixLoads(const allocas_t& allocas)
{
	LC_DEBUG1(LogGRAE) << "Fixing Loads";

	for(auto load_list : loads_) {
		for(auto load : load_list.second) {
			llvm::IRBuilder<> builder (load);
			auto ptr = GetShadowRegPtr(builder, load_list.first, allocas);
			load->setOperand(0, ptr);
		}
	}
}

void ShadowLLVMGuestRegisterAccessEmitter::FixStores(const allocas_t& allocas)
{
	LC_DEBUG1(LogGRAE) << "Fixing Stores";

	for(auto store_list : stores_) {
		for(auto store : store_list.second) {
			llvm::IRBuilder<> builder (store);
			auto ptr = GetShadowRegPtr(builder, store_list.first, allocas);
			store->setOperand(1, builder.CreatePointerCast(ptr, store->getValueOperand()->getType()->getPointerTo(0)));
		}
	}
}

void ShadowLLVMGuestRegisterAccessEmitter::LoadShadowRegs(llvm::Instruction* insertbefore, const allocas_t& allocas)
{
	// copy each registered interval from the real alloca to the real register file
	llvm::IRBuilder<> builder(insertbefore);
	for(auto i : allocas) {
		// get register file pointer
		auto reg_ptr = GetRealRegPtr(builder, i.first);

		// load register file entry
		auto reg_val = builder.CreateLoad(reg_ptr);

		// save to alloca
		builder.CreateStore(reg_val, i.second);
	}
}

void ShadowLLVMGuestRegisterAccessEmitter::SaveShadowRegs(llvm::Instruction* insertbefore, const allocas_t& allocas)
{
	// copy each registered interval from the real register file to the appropriate alloca
	llvm::IRBuilder<> builder(insertbefore);
	for(auto i : allocas) {
		// get register file pointer
		auto reg_ptr = GetRealRegPtr(builder, i.first);

		// load appropriate alloca
		auto reg_val = builder.CreateLoad(i.second);

		// save to real reg
		builder.CreateStore(reg_val, reg_ptr);
	}
}

llvm::Type* ShadowLLVMGuestRegisterAccessEmitter::GetIntervalType(const interval_t& i)
{
	return llvm::IntegerType::get(GetCtx().LLVMCtx, 8*(i.second - i.first));
}

llvm::Value* ShadowLLVMGuestRegisterAccessEmitter::GetRealRegPtr(llvm::IRBuilder<> &builder, const interval_t& interval)
{
	if(real_interval_ptrs_.count(interval) == 0) {
		llvm::IRBuilder<> reg_builder (&GetCtx().GetFunction()->getEntryBlock().front());

		auto regfile_ptr = GetCtx().GetRegStatePtr();
		auto regfile_ptr_int = reg_builder.CreatePtrToInt(regfile_ptr, GetCtx().Types.i64);
		auto reg_ptr = reg_builder.CreateAdd(regfile_ptr_int, llvm::ConstantInt::get(GetCtx().Types.i64, interval.first));
		reg_ptr = reg_builder.CreateIntToPtr(reg_ptr, GetIntervalType(interval)->getPointerTo(0));

		real_interval_ptrs_[interval] = reg_ptr;
	}
	return real_interval_ptrs_.at(interval);
}

static bool interval_contains(const ShadowLLVMGuestRegisterAccessEmitter::interval_t &bigger, const ShadowLLVMGuestRegisterAccessEmitter::interval_t &smaller)
{
	if(smaller.first >= bigger.first && smaller.second <= bigger.second) {
		return true;
	} else {
		return false;
	}
}

llvm::Value* ShadowLLVMGuestRegisterAccessEmitter::GetShadowRegPtr(llvm::IRBuilder<> &builder, const interval_t& interval, const allocas_t& allocas)
{
	if(shadow_interval_ptrs_.count(interval) == 0) {
		LC_DEBUG1(LogGRAE) << "Getting reg ptr for interval " << interval.first << " -> " << interval.second;

		// find the first alloca'd interval which might contain the given interval (it must exist)
		llvm::AllocaInst *alloca = nullptr;
		interval_t alloca_interval;
		for(auto i : allocas) {
			if(interval_contains(i.first, interval)) {
				LC_DEBUG1(LogGRAE) << " - interval contained by " << i.first.first << " -> " << i.first.second;
				alloca = i.second;
				alloca_interval = i.first;
				break;
			}
		}

		if(alloca == nullptr) {
			throw std::logic_error("Failed to find alloca");
		}

		llvm::IRBuilder<> local_builder = builder;
		local_builder.SetInsertPoint(alloca->getNextNode());

		// index alloca by correct offset
		int offset = interval.first - alloca_interval.first;
		LC_DEBUG1(LogGRAE) << " - offset in alloca is " << offset;

		auto ptr = local_builder.CreatePtrToInt(alloca, GetCtx().Types.i64);
		ptr = local_builder.CreateAdd(ptr, llvm::ConstantInt::get(GetCtx().Types.i64, offset));
		ptr = local_builder.CreateIntToPtr(ptr, GetIntervalType(interval)->getPointerTo(0));

		shadow_interval_ptrs_[interval] = ptr;
	}

	return shadow_interval_ptrs_.at(interval);
}

llvm::Value *ShadowLLVMGuestRegisterAccessEmitter::GetShadowRegPtr(llvm::IRBuilder<> &builder, const register_descriptor_t &reg, const allocas_t &allocas)
{
	LC_DEBUG1(LogGRAE) << "Getting reg ptr for " << reg.first->GetName() << " " << reg.second;

	// first, convert given register descriptor to an interval
	auto interval = GetInterval(reg);
	LC_DEBUG1(LogGRAE) << " - Got interval " << interval.first << " -> " << interval.second;

	// get base of given interval
	auto base = GetShadowRegPtr(builder, interval, allocas);
	base = builder.CreatePtrToInt(base, GetCtx().Types.i64);
	base = builder.CreateSub(base, llvm::ConstantInt::get(GetCtx().Types.i64, interval.first));
	base = builder.CreateAdd(base, llvm::ConstantInt::get(GetCtx().Types.i64, reg.first->GetOffset()));
	base = builder.CreateAdd(base, builder.CreateMul(llvm::ConstantInt::get(GetCtx().Types.i64, reg.first->GetRegisterStride()), builder.CreateZExtOrTrunc(reg.second, GetCtx().Types.i64)));
	base = builder.CreateIntToPtr(base, GetIntervalType(interval)->getPointerTo(0));

	return base;
}
